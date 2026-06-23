cmake_minimum_required(VERSION 3.20)

project(qt-viewer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

qt_standard_project_setup()

qt_add_executable(qt-viewer
    src/main.cpp
    src/MainWindow.cpp
    src/ImageViewer.cpp
    src/ImageLoader.cpp
    src/ImageProcessor.cpp

    include/MainWindow.h
    include/ImageViewer.h
    include/ImageLoader.h
    include/ImageProcessor.h
)

target_include_directories(qt-viewer
    PRIVATE
        include
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(qt-viewer
    PRIVATE
        Qt6::Widgets
        ${OpenCV_LIBS}
)
