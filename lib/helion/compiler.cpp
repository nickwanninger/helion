// [License]
// MIT - See LICENSE.md file in the package.

#include <dlfcn.h>
#include <helion/ast.h>
#include <helion/core.h>
#include <helion/gc.h>
#include <helion/iir.h>
#include <helion/infer.h>
#include <fstream>
#include <iostream>
#include <unordered_map>


using namespace helion;

// the global llvm context
static llvm::DataLayout data_layout("");
static std::unique_ptr<cg_scope> global_scope;

llvm::LLVMContext helion::llvm_ctx;
llvm::TargetMachine *target_machine = nullptr;
ojit_ee *helion::execution_engine = nullptr;
llvm::Function *allocate_function = nullptr;
llvm::Function *deallocate_function = nullptr;


static void init_llvm_env();




cg_scope *helion::cg_scope::spawn() {
  auto p = std::make_unique<cg_scope>();
  cg_scope *ptr = p.get();
  ptr->m_parent = this;
  ptr->mod = mod;
  children.push_back(std::move(p));
  return ptr;
}


llvm::Value *cg_scope::find_binding(std::string &name) {
  auto *sc = this;
  while (sc != nullptr) {
    if (sc->m_bindings.count(name) != 0) {
      return sc->m_bindings[name];
    }
    sc = sc->m_parent;
  }
  return nullptr;
}



// type lookups
datatype *cg_scope::find_type(std::string name) {
  auto *sc = this;
  while (sc != nullptr) {
    if (sc->m_types.count(name) != 0) {
      return sc->m_types[name];
    }
    sc = sc->m_parent;
  }
  return nullptr;
}



datatype *cg_scope::find_val_type(llvm::Value *v) {
  auto *sc = this;
  while (sc != nullptr) {
    if (sc->m_val_types.count(v) != 0) {
      return sc->m_val_types[v];
    }
    sc = sc->m_parent;
  }
  return nullptr;
}

/**
 * Functions can only reference functions in their own module, so we have to
 * add declaration so the dyld linker can sort out the symbol linkages when
 * we add the module to the execution_enviroment
 */
static llvm::Function *copy_function_declaration(llvm::Function &from,
                                                 llvm::Module *to) {
  auto fn = llvm::Function::Create(from.getFunctionType(),
                                   llvm::Function::ExternalLinkage, 0,
                                   from.getName(), to);
  return fn;
}




static void setup_module(llvm::Module *m) {
  m->setDataLayout(data_layout);
  m->setTargetTriple(target_machine->getTargetTriple().str());
}

static std::unique_ptr<llvm::Module> create_module(std::string name) {
  auto mod = std::make_unique<llvm::Module>(name, llvm_ctx);
  setup_module(mod.get());
  return mod;
}


/**
 * Initialize LLVM state
 */
static void init_llvm(void) {
  // init the LLVM global internal state
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetDisassembler();



  llvm::Module *engine_module;
  engine_module = new llvm::Module("helion", llvm_ctx);
  llvm::TargetOptions options = llvm::TargetOptions();

  // options.PrintMachineCode = true;

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
}




void helion::init_codegen(void) {
  init_llvm();
  init_llvm_env();


  global_scope = std::make_unique<cg_scope>();


  global_scope->set_type(any_type);


  global_scope->set_type(bool_type);
  global_scope->set_type(int8_type);
  global_scope->set_type(int16_type);
  global_scope->set_type(int32_type);
  global_scope->set_type(int64_type);
  global_scope->set_type(integer_type);
  // Float type
  global_scope->set_type(float32_type);
  // Double Type
  global_scope->set_type(float64_type);
  global_scope->set_type(datatype_type);
}




// In this function we setup the enviroment functions and types that are needed
// in the runtime of the JIT. This means creating linkage to an allocation
// function, a free function and many other kinds of needed runtime functions.
// We also create the builtin types, like ints and other types
static void init_llvm_env() {
  auto mem_mod = create_module("memory_management");


  {
    // create a linkage to the allocation function.
    // Sig = i8* allocate(i32);
    std::vector<llvm::Type *> args;
    args.push_back(int32_type->to_llvm());
    auto return_type = llvm::Type::getInt8PtrTy(llvm_ctx, 0);
    auto typ = llvm::FunctionType::get(return_type, args, false);
    allocate_function =
        llvm::Function::Create(typ, llvm::Function::ExternalLinkage, 0,
                               "helion_allocate", mem_mod.get());
  }

  {
    // create a linkage to the deallocate function
    // Sig = void deallocate(i8*);
    std::vector<llvm::Type *> args = {llvm::Type::getInt8PtrTy(llvm_ctx, 0)};
    auto typ =
        llvm::FunctionType::get(llvm::Type::getVoidTy(llvm_ctx), args, false);
    deallocate_function =
        llvm::Function::Create(typ, llvm::Function::ExternalLinkage, 0,
                               "helion_deallocate", mem_mod.get());
  }

  execution_engine->add_module(std::move(mem_mod));
}


static datatype *declare_type(std::shared_ptr<ast::typedef_node>, cg_scope *);
static method *declare_func_def(std::shared_ptr<ast::def>, cg_scope *);




static std::vector<std::unique_ptr<module>> modules;



static ska::flat_hash_map<text, datatype *> datatypes_from_names;

/**
 * takes an ast::module and turns it into a module that contains
 * executable code and all the state needed for execution
 */
module *helion::compile_module(std::unique_ptr<ast::module> m) {
  auto mod = std::make_unique<module>();

  /*
  ptr_set<iir::type *> types;
  while (!std::cin.eof()) {
    puts("Enter two types:");
    std::cout << "  t1> ";
    std::string src1;
    std::getline(std::cin, src1);

    std::cout << "  t2> ";
    std::string src2;
    std::getline(std::cin, src2);

    try {
      puts();
      auto t1 = iir::convert_type(src1);
      auto t2 = iir::convert_type(src2);

      infer::unify(t1, t2);
      puts("unification result:");
      puts("  t1 =", t1->str());
      puts("  t2 =", t2->str());
      puts("-------------------");
    } catch (std::exception &e) {
      puts(e.what());
    }
  }
  die();
  */

  // imod is a module in the intermediate representation
  iir::module imod("some_module");

  imod.create_intrinsic("add_sim", ast::parse_type("a -> a -> a"));


  // create a function that will be the 'init' function of this module
  auto *fn = gc::make_collected<iir::func>(imod);

  iir::builder b(*fn);

  auto bb = fn->new_block();
  fn->set_type(*iir::convert_type("Void -> Void", imod.spawn()));
  fn->add_block(bb);
  b.set_target(bb);

  for (auto &glob : m->globals) {
    glob->to_iir(b, &imod);
  }

  for (auto &e : m->stmts) {
    e->to_iir(b, &imod);
  }

  puts("before type inference:");
  fn->print(std::cout);
  std::cout << std::endl;



  try {
    infer::context gamma;
    fn->deduce(gamma);
  } catch (infer::analyze_failure &e) {
    puts("failed to analyze IIR:");
    e.val->print(std::cerr);
    die();
  } catch (std::exception &e) {
    die("Fatally uncaught exception:", e.what());
  }

  puts("After type inference");
  fn->print(std::cout);
  std::cout << std::endl;

  return nullptr;

  std::ofstream myfile;
  myfile.open("example.ssa");
  myfile << std::endl;
  myfile.close();


  return nullptr;
}




static datatype *declare_type(std::shared_ptr<ast::typedef_node> n,
                              cg_scope *scp) {
  auto type = n->type;
  std::string name = type->name;

  slice<text> params;
  for (auto p : type->params) {
    params.push_back(p->name);
    if (p->params.size() != 0)
      throw std::logic_error("type definition parameters must be simple names");
  }
  // search for the type, error if it is found
  auto *t = scp->find_type(type->name);

  // error if the type was already defined in the scope
  if (t != nullptr) throw std::logic_error("type already defined");

  t = datatype::create(type->name, *any_type, params);

  // simply store the ast node in the type for now. Fields are sorted out
  // at specialization and when needed. Copy it into the garbage collector
  t->ti->def = gc::make_collected<ast::typedef_node>(*n);
  // store the type in the scope under it's name
  scp->set_type(name, t);
  return t;
}
