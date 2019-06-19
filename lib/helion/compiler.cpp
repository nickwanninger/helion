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

llvm::LLVMContext helion::llvm_ctx;
llvm::TargetMachine *target_machine = nullptr;
ojit_ee *helion::execution_engine = nullptr;
llvm::Function *allocate_function = nullptr;
llvm::Function *deallocate_function = nullptr;


static void init_llvm_env();



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
}

static void init_llvm_env() {}


/**
 * takes an ast::module and turns it into a module that contains
 * executable code and all the state needed for execution
 */
iir::module *helion::compile_module(std::unique_ptr<ast::module> m) {
  auto mod = std::make_unique<iir::module>("some module");

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
  iir::module &imod = *mod;

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

