# cmake -DCMAKE_TOOLCHAIN_FILE=../build/aarch64-linux-gnu.cmake -DCMAKE_INSTALL_PREFIX=./target/tmp_root ..
# cmake -DCMAKE_INSTALL_PREFIX=/usr/ ..
# make install

cmake_minimum_required(VERSION 3.14)

project(BMC_CLID)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/global.cmake)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-E")

set(COMMANDS_DIR "lib/bmc_clid/plugin")
set(GENERATORS_DIR "lib/bmc_clid/generator")
set(APP_DIR "bin")
set(LIBS_DIR "lib")
#set(CMAKE_INSTALL_PREFIX "./target/tmp_root")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(NEED_COMMANDLINE 1)
set(NEED_COMPLETION 1)
set(NEED_PLUGIN 1)
#set(NEED_SERVER 1)
set(NEED_BMCBOX 1)
set(NEED_SPLIT 1)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

set_property(GLOBAL PROPERTY GLOBAL_BMCBOX_SRC "")
set_property(GLOBAL PROPERTY GLOBAL_BMCBOX_INC "")

add_subdirectory(src/libs)

if (DEFINED NEED_COMMANDLINE)
    aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src CLID_SRC)
    aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/commandline CLID_SRC)

    if (DEFINED NEED_SERVER)
        aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/server CLID_SRC)
    endif()

    message(STATUS "CLID_SRC=${CLID_SRC}")
    add_executable(bmc_clid ${CLID_SRC})
    target_link_libraries(bmc_clid readline bmclid_common sdbusplus systemd)

    if (DEFINED NEED_SERVER)
        target_compile_definitions(bmc_clid PRIVATE NEED_SERVER=1 CLID_PORT=8888)
    endif()

    INSTALL(TARGETS bmc_clid DESTINATION ${APP_DIR})
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/plugin)

if (DEFINED NEED_COMPLETION)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/generator)
endif()

if (DEFINED NEED_BMCBOX)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/bmcbox)
endif()
