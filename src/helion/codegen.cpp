// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/ast.h>
#include <helion/core.h>
#include <iostream>
#include <unordered_map>

using namespace helion;




// the global llvm context
llvm::LLVMContext helion::llvm_ctx;

llvm::TargetMachine *target_machine = nullptr;

static llvm::DataLayout data_layout("");
ojit_ee *helion::execution_engine = nullptr;


static std::unique_ptr<cg_scope> global_scope;

llvm::Module *global_module;



llvm::Function *allocate_function = nullptr;
llvm::Function *deallocate_function = nullptr;




namespace helion {
  /**
   * Represents the context for a single method compilation
   */
  class cg_ctx {
   public:
    llvm::IRBuilder<> builder;
    llvm::Function *func = nullptr;
    helion::module *module = nullptr;
    // what method instance is this compiling?
    method_instance *linfo;
    std::string func_name;
    std::vector<cgval> args;
    cg_ctx(llvm::LLVMContext &llvmctx) : builder(llvmctx) {}
  };

  struct cg_binding {
    std::string name;
    datatype *type;
    llvm::Value *val;
  };

  class cg_scope {
   protected:
    std::unordered_map<std::string, datatype *> m_types;
    std::unordered_map<std::string, std::unique_ptr<cg_binding>> m_bindings;
    cg_scope *m_parent;

    std::vector<std::unique_ptr<cg_scope>> children;

   public:
    cg_scope *spawn() {
      auto p = std::make_unique<cg_scope>();
      cg_scope *ptr = p.get();
      ptr->m_parent = this;
      children.push_back(std::move(p));
      return ptr;
    }

    cg_binding *find_binding(std::string &name) {
      auto *sc = this;
      while (sc != nullptr) {
        if (sc->m_bindings.count(name) != 0) {
          return sc->m_bindings[name].get();
        }
        sc = sc->m_parent;
      }
      return nullptr;
    }

    void set_binding(std::string name, std::unique_ptr<cg_binding> binding) {
      m_bindings[name] = std::move(binding);
    }

    // type lookups
    datatype *find_type(std::string name) {
      auto *sc = this;
      while (sc != nullptr) {
        if (sc->m_types.count(name) != 0) {
          return sc->m_types[name];
        }
        sc = sc->m_parent;
      }
      return nullptr;
    }

    void set_type(std::string name, datatype *T) { m_types[name] = T; }
  };

  class cg_options {};

}  // namespace helion



static void init_llvm_env(llvm::Module *);


static void setup_module(llvm::Module *m) {
  m->setDataLayout(data_layout);
  m->setTargetTriple(target_machine->getTargetTriple().str());
}


static llvm::Module *init_llvm(void) {
  // init the LLVM global internal state
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetDisassembler();



  llvm::Module *m, *engine_module;
  engine_module = new llvm::Module("helion", llvm_ctx);
  m = new llvm::Module("helion", llvm_ctx);
  llvm::TargetOptions options = llvm::TargetOptions();

  options.PrintMachineCode = true;

  llvm::EngineBuilder eb((std::unique_ptr<llvm::Module>(engine_module)));
  std::string err_str;
  eb.setEngineKind(llvm::EngineKind::JIT)
      .setTargetOptions(options)
      // Generate simpler code for JIT
      .setRelocationModel(llvm::Reloc::Static)
      .setOptLevel(llvm::CodeGenOpt::Aggressive);

  // the target triple for the current machine
  llvm::Triple the_triple(llvm::sys::getProcessTriple());
  target_machine = eb.selectTarget();

  execution_engine = new ojit_ee(*target_machine);

  data_layout = execution_engine->getDataLayout();

  setup_module(engine_module);
  setup_module(m);
  return m;
}




void helion::init_codegen(void) {
  llvm::Module *m = init_llvm();
  init_llvm_env(m);

  global_module = m;

  global_scope = std::make_unique<cg_scope>();

  // Fill out the builtin types into the global scope
  global_scope->set_type("Any", any_type);
  global_scope->set_type("Int", int32_type);
  global_scope->set_type("Float", float32_type);
}




// In this function we setup the enviroment functions and types that are needed
// in the runtime of the JIT. This means creating linkage to an allocation
// function, a free function and many other kinds of needed runtime functions.
// We also create the builtin types, like ints and other types
static void init_llvm_env(llvm::Module *m) {
  {
    // create a linkage to the allocation function.
    // Sig = i8* allocate(i32);
    std::vector<llvm::Type *> args;
    args.push_back(int32_type->to_llvm());
    auto return_type = llvm::Type::getInt8PtrTy(llvm_ctx, 0);
    auto typ = llvm::FunctionType::get(return_type, args, false);
    allocate_function = llvm::Function::Create(
        typ, llvm::Function::ExternalLinkage, 0, "allocate", global_module);
  }


  {
    // create a linkage to the deallocate function
    // Sig = void deallocate(i8*);
    std::vector<llvm::Type *> args = {llvm::Type::getInt8PtrTy(llvm_ctx, 0)};
    auto typ =
        llvm::FunctionType::get(llvm::Type::getVoidTy(llvm_ctx), args, false);
    deallocate_function = llvm::Function::Create(
        typ, llvm::Function::ExternalLinkage, 0, "deallocate", global_module);
  }
}


std::unordered_map<std::string, llvm::Module *> module_for_fname;


// this takes ownership of a module after code emission is complete
// and will add it to the execution engine when required (by
// jl_finalize_function)
/*
static void finalize_module(llvm::Module *m, bool shadow) {
  // record the function names that are part of this Module
  // so it can be added to the JIT when needed
  for (llvm::Module::iterator I = m->begin(), E = m->end(); I != E; ++I) {
    llvm::Function *F = &*I;
    if (!F->isDeclaration()) {
      module_for_fname[F->getName()] = m;
    }
  }
  // in the newer JITs, the shadow module is separate from the execution module
  // if (shadow) jl_add_to_shadow(m);
}
*/


static datatype *declare_type(std::shared_ptr<ast::typedef_node>, cg_scope *);

void helion::compile_module(std::unique_ptr<ast::module> m) {
  // the very first thing we have to do is declare the types
  for (auto t : m->typedefs) declare_type(t, global_scope.get());


  try {
    auto dt = global_scope->find_type("Node");
    datatype *spec = specialize(dt, {int32_type}, global_scope.get());

    auto pmt = ast::parse_type("some V{V}");
    pattern_match(pmt, spec, global_scope.get());
    puts("V =", global_scope->find_type("V")->str());
  } catch (pattern_match_error &e) {
    die(e.what());
  }

  // global_module->print(llvm::errs(), nullptr);
}

/*
static std::unique_ptr<llvm::Module> emit_function(method_instance *lam) {
  std::unique_ptr<llvm::Module> m;

  // step 1. Build code context for the compilation of this method
  cg_ctx ctx(llvm_ctx);
  ctx.linfo = lam;

  ctx.func_name = lam->of->name;
  return m;
}
*/




llvm::Value *ast::number::codegen(cg_ctx &ctx, cg_scope *sc, cg_options *opt) {
  return nullptr;
}



llvm::Value *ast::binary_op::codegen(cg_ctx &ctx, cg_scope *sc,
                                     cg_options *opt) {
  return nullptr;
}

llvm::Value *ast::dot::codegen(cg_ctx &ctx, cg_scope *sc, cg_options *opt) {
  return nullptr;
}


llvm::Value *ast::subscript::codegen(cg_ctx &ctx, cg_scope *sc,
                                     cg_options *opt) {
  return nullptr;
}

llvm::Value *ast::call::codegen(cg_ctx &ctx, cg_scope *sc, cg_options *opt) {
  return nullptr;
}



llvm::Value *ast::tuple::codegen(cg_ctx &ctx, cg_scope *sc, cg_options *opt) {
  return nullptr;
}


llvm::Value *ast::string::codegen(cg_ctx &ctx, cg_scope *sc, cg_options *opt) {
  return nullptr;
}

llvm::Value *ast::keyword::codegen(cg_ctx &ctx, cg_scope *sc, cg_options *opt) {
  return nullptr;
}

llvm::Value *ast::nil::codegen(cg_ctx &ctx, cg_scope *sc, cg_options *opt) {
  return nullptr;
}

llvm::Value *ast::do_block::codegen(cg_ctx &ctx, cg_scope *sc,
                                    cg_options *opt) {
  return nullptr;
}

llvm::Value *ast::return_node::codegen(cg_ctx &ctx, cg_scope *sc,
                                       cg_options *opt) {
  return nullptr;
}


llvm::Value *ast::type_node::codegen(cg_ctx &ctx, cg_scope *sc,
                                     cg_options *opt) {
  return nullptr;
}

llvm::Value *ast::var_decl::codegen(cg_ctx &ctx, cg_scope *sc,
                                    cg_options *opt) {
  return nullptr;
}


llvm::Value *ast::var::codegen(cg_ctx &ctx, cg_scope *sc, cg_options *opt) {
  return nullptr;
}

llvm::Value *ast::prototype::codegen(cg_ctx &ctx, cg_scope *sc,
                                     cg_options *opt) {
  return nullptr;
}


llvm::Value *ast::func::codegen(cg_ctx &ctx, cg_scope *sc, cg_options *opt) {
  return nullptr;
}


llvm::Value *ast::def::codegen(cg_ctx &ctx, cg_scope *sc, cg_options *opt) {
  return nullptr;
}

llvm::Value *ast::if_node::codegen(cg_ctx &ctx, cg_scope *sc, cg_options *opt) {
  return nullptr;
}


llvm::Value *ast::typedef_node::codegen(cg_ctx &ctx, cg_scope *sc,
                                        cg_options *opt) {
  return nullptr;
}

llvm::Value *ast::typeassert::codegen(cg_ctx &ctx, cg_scope *sc,
                                      cg_options *opt) {
  return nullptr;
}


static datatype *declare_type(std::shared_ptr<ast::typedef_node> n,
                              cg_scope *scp) {
  auto type = n->type;
  std::string name = type->name;

  std::vector<std::string> params;
  for (auto &p : type->params) {
    params.push_back(p->name);
    if (p->params.size() != 0)
      throw std::logic_error("type definition parameters must be simple names");
  }


  auto *t = &datatype::create(type->name, *any_type, params);

  // simply store the ast node in the type for now. Fields are sorted out
  // at specialization and when needed
  t->ti->node = n;
  // store the type in the scope under it's name
  scp->set_type(name, t);

  return t;
}

datatype *helion::specialize(std::shared_ptr<ast::type_node> &tn, cg_scope *s) {
  std::string name = tn->name;
  datatype *t = s->find_type(name);

  std::vector<datatype *> p;

  for (auto &param : tn->params) {
    p.push_back(specialize(param, s));
  }
  return specialize(t, p, s);
}


datatype *helion::specialize(datatype *t, cg_scope *scp) {
  return specialize(t, {}, scp);
}

datatype *helion::specialize(datatype *t, std::vector<datatype *> params,
                             cg_scope *scp) {
  if (t->ti->style == type_style::FLOATING ||
      t->ti->style == type_style::INTEGER)
    return t;


  // Step 1. Check that the parameter count is the same as is expected
  if (params.size() != t->ti->param_names.size()) {
    std::string err;
    err += "Unable to specialize type ";
    err += t->ti->name;
    err += " with invalid number of parameters. Expected ";
    err += std::to_string(t->ti->param_names.size());
    err += ". Got ";
    err += std::to_string(params.size());
    throw std::logic_error(err.c_str());
  }

  // Step 2. Check if the current type is specialized or now. If it is,
  // short circuit and return it
  if (t->specialized) return t;


  // Step 3. Search through the specializations in the typeinfo and
  //         attempt to find an existing specialization
  for (auto &s : t->ti->specializations) {
    if (s->param_types == params) return s.get();
  }

  // Step 4. Create a new specialization, this is one of the more complicated
  //         parts of the specialization lookup

  // spawn a scope that the specialization will be built in
  auto *ns = scp->spawn();

  // loop over and fill in the new scope
  for (size_t i = 0; i < t->ti->param_names.size(); i++) {
    ns->set_type(t->ti->param_names[i], params[i]);
  }

  // allocate a new instance of the datatype
  auto spec = t->spawn_spec();
  spec->param_types = params;
  spec->ti = t->ti;  // fill in the type info
  auto node = t->ti->node;

  for (auto &f : node->fields) {
    auto *ft = specialize(f.type, ns);
    spec->add_field(f.name, ft);
  }

  return spec;
}



static std::vector<std::unique_ptr<method>> method_table;

// create a method from a global def. Simply a named func creation
// in the global_scope
method *method::create(std::shared_ptr<ast::def> &n) {
  method *m = method::create(n->fn, global_scope.get());
  m->name = n->name;
  return m;
}



method *method::create(std::shared_ptr<ast::func> &fn, cg_scope *scp) {
  auto m = std::make_unique<method>();
  auto mptr = m.get();




  method_table.push_back(std::move(m));
  return mptr;
}


/**
 * attempt to pattern match the parameters of the two types.
 * This basically just requires that the two types have the same
 * number of parameters, and all of the parameters pattern match
 * successfully. Will throw
 */
static void pattern_match_params(ast::type_node *n, datatype *on, cg_scope *s) {
  if (n->params.size() != on->param_types.size()) {
    throw pattern_match_error(*n, *on, "Parameter count mismatch");
  }

  for (size_t i = 0; i < n->params.size(); i++) {
    pattern_match(n->params[i], on->param_types[i], s);
  }
}



/**
 * Pattern match a simple type name. This ends up just being
 * any type_node who's type is type_style::OBJECT. It then
 * pattern matches on the parameters.
 */
static void pattern_match_name(ast::type_node *n, datatype *on, cg_scope *s) {
  if (n->parameter) {
    // if the name is a parameter, we need to assign it in the scope if there
    // already is not a type under that name
    if (auto bound = s->find_type(n->name); bound != nullptr) {
      throw pattern_match_error(*n, *on, "Parameter already bound");
    }

    // set the type in the scope
    s->set_type(n->name, on);
  } else {
    auto bound = s->find_type(n->name);
    if (bound != on) {
      std::string err;
      err += n->name;
      err += " is bound to ";
      err += bound->str();
      throw pattern_match_error(*n, *on, err);
    }
  }

  pattern_match_params(n, on, s);
}



/**
 * attempt to pattern match a slice type. Basically just pattern
 * matches on the parameters.
 */
static void pattern_match_slice(ast::type_node *n, datatype *on, cg_scope *s) {
  // the other type should be a slice as well
  if (on->ti->style != type_style::SLICE)
    throw pattern_match_error(
        *n, *on, "Cannot pattern match slice against non-slice type");

  // since slices store their types in the parameters, simply pattern match the
  // parameters
  pattern_match_params(n, on, s);
}

/**
 * attempt to pattern match two types. Simply an entry point into
 * multiple other places.
 */
void helion::pattern_match(std::shared_ptr<ast::type_node> &n, datatype *on,
                           cg_scope *s) {
  // the type we are pattern matching on must be specialized
  assert(on->specialized);


  // Determine the type of the node. If it is a plain type reference, pattern
  // match on it and it's parameters.
  if (n->style == type_style::OBJECT) {
    pattern_match_name(n.get(), on, s);
  }

  if (n->style == type_style::SLICE) {
    pattern_match_slice(n.get(), on, s);
  }
}


helion::pattern_match_error::pattern_match_error(ast::type_node &n,
                                                 datatype &with,
                                                 std::string msg) {
  _msg += "Failed to pattern match ";
  _msg += n.str();
  _msg += " with ";
  _msg += with.str();
  _msg += ": ";
  _msg += msg;
}
