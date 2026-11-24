cmake_minimum_required(VERSION 3.16)

project(vision_project LANGUAGES CXX)

find_package(OpenCV REQUIRED COMPONENTS core imgcodecs highgui videoio imgproc viz)

add_executable(vision_project src/main.cpp src/panel.cpp src/grabado.cpp src/medicion.cpp)

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

# Medición reproducible de filtros, fuera de la aplicación de cámara.
add_executable(benchmark_grabado EXCLUDE_FROM_ALL tools/benchmark_grabado.cpp src/grabado.cpp)
target_include_directories(benchmark_grabado PRIVATE src)
target_link_libraries(benchmark_grabado PRIVATE opencv_core opencv_imgproc)
set_target_properties(benchmark_grabado PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
