cmake_minimum_required(VERSION 3.20)

# Project setup
project(QtViewerPro VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
find_package(OpenCV REQUIRED)

qt_standard_project_setup()
enable_testing()

# Application target
set(QTVIEWERPRO_APP_SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/ImageViewer2D.cpp
    src/io/ImageLoader.cpp
    src/processing/ImageProcessor.cpp
    src/core/SliceExtractor.cpp
    src/core/VolumeData.cpp
)

set(QTVIEWERPRO_APP_HEADERS
    include/qtviewerpro/ui/MainWindow.h
    include/qtviewerpro/ui/ImageViewer2D.h
    include/qtviewerpro/io/ImageLoader.h
    include/qtviewerpro/processing/ImageProcessor.h
    include/qtviewerpro/core/SliceExtractor.h
    include/qtviewerpro/core/SliceOrientation.h
    include/qtviewerpro/core/VolumeData.h
)

qt_add_executable(qt-viewer-pro
    ${QTVIEWERPRO_APP_SOURCES}
    ${QTVIEWERPRO_APP_HEADERS}
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

# Tests
qt_add_executable(test-volume-data
    tests/TestVolumeData.cpp
    src/core/VolumeData.cpp
)

target_include_directories(test-volume-data
    PRIVATE
        include
)

target_link_libraries(test-volume-data
    PRIVATE
        Qt6::Test
)

add_test(NAME test-volume-data COMMAND test-volume-data)

qt_add_executable(test-slice-extractor
    tests/TestSliceExtractor.cpp
    src/core/SliceExtractor.cpp
    src/core/VolumeData.cpp
)

target_include_directories(test-slice-extractor
    PRIVATE
        include
)

target_link_libraries(test-slice-extractor
    PRIVATE
        Qt6::Test
)

add_test(NAME test-slice-extractor COMMAND test-slice-extractor)
