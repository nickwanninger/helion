

# helion requires LLVM 8
find_package(LLVM 8 CONFIG)
llvm_map_components_to_libnames(LLVM_LIBS core support orcjit native passes)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")
message(STATUS "LLVM_LIBS: ${LLVM_LIBS}")




add_library(helion-obj OBJECT
	lib/helion/text.cpp
	lib/helion/ast.cpp
	lib/helion/compiler.cpp
	lib/helion/tokenizer.cpp
	lib/helion/codegen.cpp
	lib/helion/ojit_ee.cpp
	lib/helion/gc.cpp
	lib/helion/parser.cpp
	lib/helion/iir.cpp
	lib/helion/iirbuilder.cpp
	lib/helion/iirgen.cpp
	lib/helion/typesystem.cpp
	lib/helion/infer.cpp
)

target_include_directories(helion-obj PRIVATE ${LLVM_INCLUDE_DIRS})


# add_library(helion-lib SHARED $<TARGET_OBJECTS:helion-obj>)
# add_library(helion-a   STATIC $<TARGET_OBJECTS:helion-obj>)
# set_target_properties(helion-lib PROPERTIES OUTPUT_NAME helion)
# target_link_libraries(helion-lib ${LLVM_LIBS} ${CMAKE_DL_LIBS} -lgc -pthread -lboost_system)
# set_target_properties(helion-a PROPERTIES OUTPUT_NAME helion)
# target_link_libraries(helion-a ${LLVM_LIBS} ${CMAKE_DL_LIBS} -lgc -pthread -lboost_system)

add_executable(helion
	lib/main.cpp
	$<TARGET_OBJECTS:helion-obj>
)


target_include_directories(helion PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(helion ${LLVM_LIBS} ${CMAKE_DL_LIBS} -lgc -pthread -lboost_system)

# target_link_libraries(helion helion-obj)

