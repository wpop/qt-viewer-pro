cmake_minimum_required(VERSION 3.20)

project(QtViewerPro VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

qt_standard_project_setup()

qt_add_executable(qt-viewer-pro
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/ImageViewer2D.cpp
    src/io/ImageLoader.cpp
    src/processing/ImageProcessor.cpp
    src/core/VolumeData.cpp

    include/qtviewerpro/ui/MainWindow.h
    include/qtviewerpro/ui/ImageViewer2D.h
    include/qtviewerpro/io/ImageLoader.h
    include/qtviewerpro/processing/ImageProcessor.h
    include/qtviewerpro/core/VolumeData.h
)

target_include_directories(qt-viewer-pro
    PRIVATE
        include
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(qt-viewer-pro
    PRIVATE
        Qt6::Widgets
        ${OpenCV_LIBS}
)
