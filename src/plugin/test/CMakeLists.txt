include_directories(inc)
aux_source_directory(src SRC_LIST)

if (DEFINED NEED_BMCBOX)
    bmcbox_aux_source()
endif()

if (DEFINED NEED_PLUGIN)
    add_library(test SHARED ${SRC_LIST})
    target_compile_definitions(test PRIVATE NEED_PLUGIN=1)
    INSTALL(TARGETS test LIBRARY DESTINATION ${COMMANDS_DIR})
endif()

if (DEFINED NEED_SPLIT)
    add_executable(bmcbox_test ${SRC_LIST})
    target_compile_definitions(bmcbox_test PRIVATE NEED_SPLIT=1)
    INSTALL(TARGETS bmcbox_test DESTINATION ${APP_DIR})
endif()