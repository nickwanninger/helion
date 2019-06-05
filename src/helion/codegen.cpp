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




static llvm::IntegerType *T_int32;
static llvm::IntegerType *T_int64;
static llvm::Type *T_float32;
static llvm::Type *T_float64;
static llvm::Type *T_void;




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
    datatype *find_type(std::string &name) {
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


  global_scope = std::make_unique<cg_scope>();

  // Fill out the builtin types into the global scope
  global_scope->set_type("Any", any_type);
  global_scope->set_type("Int", int32_type);
  global_scope->set_type("Float", float32_type);
}




static void init_llvm_env(llvm::Module *m) {
  T_int32 = llvm::Type::getInt32Ty(llvm_ctx);
  T_int64 = llvm::Type::getInt64Ty(llvm_ctx);
  T_float32 = llvm::Type::getFloatTy(llvm_ctx);
  T_float64 = llvm::Type::getDoubleTy(llvm_ctx);
  T_void = llvm::Type::getVoidTy(llvm_ctx);
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
static datatype *specialize_type(std::shared_ptr<ast::type_node> &, cg_scope *);

static datatype *specialize_type(datatype *, std::vector<datatype *>,
                                 cg_scope *);



// convert a function ast node into a more abstract method
static method *func_to_method(std::shared_ptr<ast::func>, cg_scope *);


void helion::compile_module(std::unique_ptr<ast::module> m) {
  // the very first thing we have to do is declare the types
  for (auto t : m->typedefs) {
    auto *dt = declare_type(t, global_scope.get());

    // try to specialize with no arguments
    datatype *spec = specialize_type(dt, {&datatype::create_integer("Int512", 512)}, global_scope.get());

    auto llt = spec->to_llvm();
    llt->print(llvm::errs());
  }
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


static datatype *specialize_type(std::shared_ptr<ast::type_node> &tn,
                                 cg_scope *s) {
  std::string name = tn->name;
  datatype *t = s->find_type(name);

  std::vector<datatype *> p;

  for (auto &param : tn->params) {
    p.push_back(specialize_type(param, s));
  }
  return specialize_type(t, p, s);
}

static datatype *specialize_type(datatype *t, std::vector<datatype *> params,
                                 cg_scope *scp) {
  if (t->ti->style == typeinfo::type_style::FLOATING || t->ti->style == typeinfo::type_style::INTEGER)
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
  // if (t->specialized) return t;

  // grab a lock on the typeinfo
  std::unique_lock lock(t->ti->lock);

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
  spec->ti = t->ti;  // fill in the type info
  auto node = t->ti->node;

  for (auto &f : node->fields) {
    auto *ft = specialize_type(f.type, ns);
    spec->add_field(f.name, ft);
    puts(f.name, ft->str());
  }

  return spec;
}



static method *func_to_method(std::shared_ptr<ast::func>, cg_scope *) {
  return nullptr;
}
