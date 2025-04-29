add_library(test_generator SHARED ${CMAKE_CURRENT_SOURCE_DIR}/test_generator.cpp)
add_library(alert_generator SHARED ${CMAKE_CURRENT_SOURCE_DIR}/alert_generator.cpp)
add_library(command_generator SHARED ${CMAKE_CURRENT_SOURCE_DIR}/command_generator.cpp)
add_library(data_generator SHARED ${CMAKE_CURRENT_SOURCE_DIR}/data_generator.cpp)

INSTALL(TARGETS test_generator alert_generator command_generator data_generator LIBRARY DESTINATION ${GENERATORS_DIR})