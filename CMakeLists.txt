cmake_minimum_required(VERSION 3.22)
project(simple VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenCV REQUIRED)

add_executable(simple src/ArmorParam.cpp src/main.cpp src/ArmorDescriptor.cpp)

target_include_directories(simple PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(simple PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 添加海康SDK的头文件目录
#target_include_directories(simple PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/demo/hik/hikSDK/include)

# 添加海康SDK的库文件目录
#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/demo/hik/hikSDK/lib/amd64)

# 查找MvCameraControl库
#find_library(MV_CAMERA_CONTROL_LIB NAMES MvCameraControl PATHS ${CMAKE_CURRENT_SOURCE_DIR}/demo/hik/hikSDK/lib/amd64)

# 链接OpenCV库和海康SDK库
#target_link_libraries(simple ${OpenCV_LIBS} ${MV_CAMERA_CONTROL_LIB})
target_link_libraries(simple ${OpenCV_LIBS})