cmake_minimum_required(VERSION 3.16)

project(vision_project LANGUAGES CXX)

find_package(OpenCV REQUIRED COMPONENTS core imgcodecs highgui videoio imgproc viz)

add_executable(vision_project src/main.cpp src/grabado.cpp)

target_link_libraries(vision_project PRIVATE ${OpenCV_LIBS})

if(APPLE)
    enable_language(OBJCXX)
    target_sources(vision_project PRIVATE src/escape_macos.mm)
    target_link_libraries(vision_project PRIVATE "-framework Cocoa")
endif()

set_target_properties(vision_project PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
