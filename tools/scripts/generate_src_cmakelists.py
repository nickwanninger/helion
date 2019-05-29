import glob
import itertools as itools

header = """
# generated by tools/scripts/generate_src_cmakelists.py. DO NOT MODIFY


find_package(LLVM 8 CONFIG)
# llvm_map_components_to_libnames(LLVM_LIBS core support demangle mcjit native)

llvm_map_components_to_libnames(LLVM_LIBS core support orcjit native passes)



message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")
message(STATUS "LLVM_LIBS: ${LLVM_LIBS}")

set(LLVM_LINK_LLVM_DYLIB on)

"""

footer = """


target_include_directories(helion PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(helion uv_a ${CMAKE_DL_LIBS} ${LLVM_LIBS} -lgc -lgccpp -pthread -lboost_system)

set_target_properties(helion PROPERTIES OUTPUT_NAME helion)

"""



def glb(path, recursive = True):
    return glob.iglob(path, recursive=recursive)



with open('src/helion/CMakeLists.txt', 'w') as f:
    f.write(header)

    f.write('add_executable(helion\n')
    for filename in itools.chain(glb('src/helion/**/*.cpp'),
            glb('src/helion/**/*.c'), glb('src/helion/**/*.s')):
        f.write('\t%s\n' % (filename))
    f.write(")\n")
    f.write("\n");
    f.write(footer)
