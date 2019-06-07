// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/core.h>
#include <helion/util.h>
#include <iostream>

using namespace helion;


ojit_ee::ojit_ee(llvm::TargetMachine& TM)
    : TM(TM),
      DL(TM.createDataLayout()),
      exec_session(),
      symbol_resolver(createLegacyLookupResolver(
          exec_session,
          [this](const std::string& name) {
            return obj_layer.findSymbol(name, true);
          },
          [](llvm::Error err) {
            cantFail(std::move(err), "lookupFlags failed");
          })),
      obj_layer(exec_session,
                [this](llvm::orc::VModuleKey) {
                  return ObjLayer::Resources{
                      std::make_shared<llvm::SectionMemoryManager>(),
                      symbol_resolver};
                }),
      compile_layer(obj_layer, llvm::orc::SimpleCompiler(TM)),
      opt_layer(compile_layer, [this](std::unique_ptr<llvm::Module> M) {
        return opt_module(std::move(M));
      }) {
  llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
}




const llvm::DataLayout& ojit_ee::getDataLayout() const { return DL; }

const llvm::Triple& ojit_ee::getTargetTriple() const {
  return TM.getTargetTriple();
}


void ojit_ee::add_global_mapping(llvm::StringRef name, uint64_t addr) {
  bool successful =
      GlobalSymbolTable.insert(std::make_pair(name, (void*)addr)).second;
  (void)successful;
  assert(successful);
}

llvm::JITSymbol ojit_ee::find_mangled_symbol(const std::string& name,
                                             bool exp_only) {
  const bool ExportedSymbolsOnly = exp_only;

  // Search modules in reverse order: from last added to first added.
  // This is the opposite of the usual search order for dlsym, but makes more
  // sense in a REPL where we want to bind to the newest available definition.
  for (auto H : llvm::make_range(module_keys.rbegin(), module_keys.rend())) {
    puts(name);
    auto sym = compile_layer.findSymbolIn(H, name, ExportedSymbolsOnly);
    if (sym) {
      return sym;
    }
  }

  // If we can't find the symbol in the JIT, try looking in the host process.
  if (auto SymAddr = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name))
    return llvm::JITSymbol(SymAddr, llvm::JITSymbolFlags::Exported);

#ifdef _WIN32
  // For Windows retry without "_" at beginning, as RTDyldMemoryManager uses
  // GetProcAddress and standard libraries like msvcrt.dll use names
  // with and without "_" (for example "_itoa" but "sin").
  if (name.length() > 2 && Name[0] == '_')
    if (auto SymAddr = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(
            Name.substr(1)))
      return llvm::JITSymbol(SymAddr, llvm::JITSymbolFlags::Exported);
#endif

  return nullptr;
}



ojit_ee::ModuleHandle ojit_ee::add_module(std::unique_ptr<llvm::Module> m) {
  auto K = exec_session.allocateVModule();
  cantFail(opt_layer.addModule(K, std::move(m)));
  module_keys.push_back(K);
  return K;
}



std::unique_ptr<llvm::Module> ojit_ee::opt_module(std::unique_ptr<llvm::Module> M) {
  // Create a function pass manager.
  auto FPM = llvm::make_unique<llvm::legacy::FunctionPassManager>(M.get());

  // Add some optimizations.
  FPM->add(llvm::createInstructionCombiningPass());
  FPM->add(llvm::createReassociatePass());
  FPM->add(llvm::createGVNPass());
  FPM->add(llvm::createCFGSimplificationPass());
  FPM->doInitialization();

  // Run the optimizations over all functions in the module being added to
  // the JIT.
  for (auto& F : *M) FPM->run(F);

  return M;
}
