cmake_minimum_required(VERSION 3.10)
project(connector_manager)
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread ")
find_package(CURL REQUIRED) 
include_directories(${CURL_INCLUDE_DIR})
include_directories("include")

add_library(${PROJECT_NAME} SHARED curl_wrapper.cpp connector_manager.cpp)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  add_executable(${PROJECT_NAME}_exe main.cpp)
  target_link_libraries(${PROJECT_NAME}_exe -lm -pthread ${CURL_LIBRARIES} ${PROJECT_NAME})
else()
set_target_properties(${PROJECT_NAME} PROPERTIES
LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
  message("This is a submodule.")
endif()
target_link_libraries(${PROJECT_NAME} -lm -pthread ${CURL_LIBRARIES} ${PROJECT_NAME})