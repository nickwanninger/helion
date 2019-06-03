// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/core.h>
#include <iostream>
#include <unordered_map>

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

// what the internal representation of a method shall be
// static llvm::StructType *T_method_type;


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
static void finalize_module(llvm::Module *m, bool shadow) {
  // record the function names that are part of this Module
  // so it can be added to the JIT when needed
  // for (llvm::Function *F : m) {}


  for (llvm::Module::iterator I = m->begin(), E = m->end(); I != E; ++I) {
    llvm::Function *F = &*I;
    if (!F->isDeclaration()) {
      /*
      bool known = incomplete_fname.erase(F->getName());
      (void)known;  // TODO: assert(known); // llvmcall gets this wrong
      */
      module_for_fname[F->getName()] = m;
    }
  }
  // in the newer JITs, the shadow module is separate from the execution module
  // if (shadow) jl_add_to_shadow(m);
}




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




// main entry point to the compiler. All code must be generated into a function, so
// this function must compile a method instance.
static std::unique_ptr<llvm::Module> emit_function(method_instance *lam) {
  std::unique_ptr<llvm::Module> m;



  // step 1. Build code context for the compilation of this method
  cg_ctx ctx(llvm_ctx);
  ctx.linfo = lam;

  ctx.func_name = lam->of->name;

  return m;
}
