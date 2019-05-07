import glob
import itertools as itools

header = """
# generated by tools/scripts/generate_src_cmakelists.py. DO NOT MODIFY

add_library(gc_static STATIC IMPORTED)
set_property(TARGET gc_static PROPERTY IMPORTED_LOCATION
    ${PROJECT_SOURCE_DIR}/src/bdwgc/.libs/libgc.a
)
set_property(TARGET gc_static PROPERTY IMPORTED_LOCATION
    ${PROJECT_SOURCE_DIR}/src/bdwgc/.libs/libgccpp.a
    )

"""

footer = """


# add_library(helion-lib SHARED $<TARGET_OBJECTS:helion-obj>)
# add_library(helion-a   STATIC $<TARGET_OBJECTS:helion-obj>)
#
# target_link_libraries(helion-lib asmjit ${CMAKE_DL_LIBS} -pthread -lboost_system)
# set_target_properties(helion-lib PROPERTIES OUTPUT_NAME helion)
#
#
# target_link_libraries(helion-a asmjit ${CMAKE_DL_LIBS} -pthread -lboost_system)
# set_target_properties(helion-a PROPERTIES OUTPUT_NAME helion)

target_link_libraries(helion replxx ${CMAKE_DL_LIBS} -pthread -lboost_system)
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
