include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src COMMON_SRC)
add_library(bmclid_common SHARED ${COMMON_SRC})
INSTALL(TARGETS bmclid_common LIBRARY  DESTINATION ${LIBS_DIR})