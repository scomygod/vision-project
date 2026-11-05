cmake_minimum_required(VERSION 3.16)

project(vision_project LANGUAGES CXX)

find_package(OpenCV REQUIRED COMPONENTS core imgcodecs highgui)

add_executable(vision_project src/main.cpp)

target_link_libraries(vision_project PRIVATE ${OpenCV_LIBS})

set_target_properties(vision_project PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
