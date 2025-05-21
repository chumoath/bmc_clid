get_property(FINAL_BMCBOX_SRC GLOBAL PROPERTY GLOBAL_BMCBOX_SRC)
get_property(FINAL_BMCBOX_INC GLOBAL PROPERTY GLOBAL_BMCBOX_INC)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} FINAL_BMCBOX_SRC)
include_directories(${FINAL_BMCBOX_INC})

set(FINAL_BMCBOX_SRC ${FINAL_BMCBOX_SRC} ${CMAKE_SOURCE_DIR}/src/commandline/Command.cpp)

add_executable(bmcbox ${FINAL_BMCBOX_SRC})
target_link_libraries(bmcbox bmclid_common)
target_compile_definitions(bmcbox PRIVATE NEED_BMCBOX=1)

INSTALL(TARGETS bmcbox DESTINATION ${APP_DIR})

message(STATUS "FINAL_BMCBOX_INC=${FINAL_BMCBOX_INC}")
message(STATUS "FINAL_BMCBOX_SRC=${FINAL_BMCBOX_SRC}")