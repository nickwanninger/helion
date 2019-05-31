// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/core.h>
#include <iostream>

using namespace helion;




// the global llvm context
llvm::LLVMContext helion::llvm_ctx;

llvm::TargetMachine *target_machine = nullptr;

static llvm::DataLayout data_layout("");
ojit_ee *helion::execution_engine = nullptr;




static llvm::IntegerType *T_int32;
static llvm::IntegerType *T_int64;
static llvm::Type *T_float32;
static llvm::Type *T_float64;
static llvm::Type *T_void;



struct cgval {
  llvm::Value *v;
  datatype *typ;

  cgval(llvm::Value *v, datatype *typ) : v(v), typ(typ) {}

  cgval()
      :  // undef / unreachable / default constructor
        v(llvm::UndefValue::get(T_void)),
        typ(nullptr) {}

  //
  inline operator llvm::Value *() const { return v; }
  inline operator datatype *() const { return typ; }
};




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

  options.PrintMachineCode =
      true;  // Print machine code produced during JIT compiling


  llvm::EngineBuilder eb((std::unique_ptr<llvm::Module>(engine_module)));
  std::string err_str;
  eb.setEngineKind(llvm::EngineKind::JIT)
      .setTargetOptions(options)
      // Generate simpler code for JIT
      .setRelocationModel(llvm::Reloc::Static);


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

  cgval v;
  v.v->print(llvm::errs());
}




static void init_llvm_env(llvm::Module *m) {
  T_int32 = llvm::Type::getInt32Ty(llvm_ctx);
  T_int64 = llvm::Type::getInt64Ty(llvm_ctx);
  T_float32 = llvm::Type::getFloatTy(llvm_ctx);
  T_float64 = llvm::Type::getDoubleTy(llvm_ctx);
  T_void = llvm::Type::getVoidTy(llvm_ctx);
}
