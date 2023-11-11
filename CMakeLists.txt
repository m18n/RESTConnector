cmake_minimum_required(VERSION 3.10)
project(connector_manager)
set(NAME_OUT_FILE ${PROJECT_NAME})
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread ")
find_package(CURL REQUIRED) 
include_directories(${CURL_INCLUDE_DIR})

add_library(${NAME_OUT_FILE} SHARED curl_wrapper.cpp connector_manager.cpp)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(NAME_OUT_FILE ${PROJECT_NAME}_exe)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  add_executable(${NAME_OUT_FILE}_exe main.cpp)
else()
set_target_properties(${NAME_OUT_FILE} PROPERTIES
LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
  message("This is a submodule.")
endif()
target_link_libraries(${NAME_OUT_FILE} -lm -pthread ${CURL_LIBRARIES} )