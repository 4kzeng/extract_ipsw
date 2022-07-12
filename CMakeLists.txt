cmake_minimum_required(VERSION 2.8)

project(extract_ipsw)
add_executable(${PROJECT_NAME} "main.cpp" "utils.h" "utils.cpp")


if(WIN32)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../includes")

else()
#--------------mac------------------#
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/includes")
target_link_libraries(${PROJECT_NAME}  -L${CMAKE_CURRENT_SOURCE_DIR}/libs)
target_link_libraries(${PROJECT_NAME}  libplist-2.0.a)
target_link_libraries(${PROJECT_NAME}  libzip.a)
target_link_libraries(${PROJECT_NAME}  libz.dylib)



endif()
