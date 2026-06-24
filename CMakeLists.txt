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

# Core library
set(QTVIEWERPRO_CORE_SOURCES
    src/core/SliceData.cpp
    src/core/SliceExtractor.cpp
    src/core/VolumeData.cpp
)

set(QTVIEWERPRO_CORE_HEADERS
    include/qtviewerpro/core/SliceData.h
    include/qtviewerpro/core/SliceExtractor.h
    include/qtviewerpro/core/SliceOrientation.h
    include/qtviewerpro/core/VolumeData.h
)

add_library(qt-viewer-pro-core
    ${QTVIEWERPRO_CORE_SOURCES}
    ${QTVIEWERPRO_CORE_HEADERS}
)

target_include_directories(qt-viewer-pro-core
    PUBLIC
        include
)

# Application target
set(QTVIEWERPRO_APP_SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/ImageViewer2D.cpp
    src/io/ImageLoader.cpp
    src/processing/ImageProcessor.cpp
    src/processing/WindowLevelProcessor.cpp
)

set(QTVIEWERPRO_APP_HEADERS
    include/qtviewerpro/ui/MainWindow.h
    include/qtviewerpro/ui/ImageViewer2D.h
    include/qtviewerpro/io/ImageLoader.h
    include/qtviewerpro/processing/ImageProcessor.h
    include/qtviewerpro/processing/WindowLevelProcessor.h
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
        qt-viewer-pro-core
        Qt6::Widgets
        ${OpenCV_LIBS}
)

# Tests
qt_add_executable(test-volume-data
    tests/TestVolumeData.cpp
)

target_link_libraries(test-volume-data
    PRIVATE
        qt-viewer-pro-core
        Qt6::Test
)

add_test(NAME test-volume-data COMMAND test-volume-data)

qt_add_executable(test-slice-data
    tests/TestSliceData.cpp
)

target_link_libraries(test-slice-data
    PRIVATE
        qt-viewer-pro-core
        Qt6::Test
)

add_test(NAME test-slice-data COMMAND test-slice-data)

qt_add_executable(test-slice-extractor
    tests/TestSliceExtractor.cpp
)

target_link_libraries(test-slice-extractor
    PRIVATE
        qt-viewer-pro-core
        Qt6::Test
)

add_test(NAME test-slice-extractor COMMAND test-slice-extractor)

qt_add_executable(test-window-level-processor
    tests/TestWindowLevelProcessor.cpp
    src/processing/WindowLevelProcessor.cpp
)

target_include_directories(test-window-level-processor
    PRIVATE
        include
)

target_link_libraries(test-window-level-processor
    PRIVATE
        Qt6::Test
)

add_test(NAME test-window-level-processor COMMAND test-window-level-processor)
