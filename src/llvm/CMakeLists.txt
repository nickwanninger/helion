find_package(LLVM 8 CONFIG)
# llvm_map_components_to_libnames(LLVM_LIBS core support demangle mcjit native)

llvm_map_components_to_libnames(LLVM_LIBS core support orcjit native passes)



message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")
message(STATUS "LLVM_LIBS: ${LLVM_LIBS}")


add_library(helion_llvm SHARED
	src/llvm/llvm.cpp
)

target_link_libraries(helion_llvm ${LLVM_LIBS})
