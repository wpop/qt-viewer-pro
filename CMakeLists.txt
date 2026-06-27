cmake_minimum_required(VERSION 3.20)

# Project setup
project(QtViewerPro VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGLWidgets Test)
find_package(OpenCV REQUIRED)

qt_standard_project_setup()
enable_testing()

# Core library
set(QTVIEWERPRO_CORE_SOURCES
    src/core/SliceData.cpp
    src/core/SliceExtractor.cpp
    src/core/VolumeData.cpp
    src/core/VolumeMetadata.cpp
)

set(QTVIEWERPRO_CORE_HEADERS
    include/qtviewerpro/core/SliceData.h
    include/qtviewerpro/core/SliceExtractor.h
    include/qtviewerpro/core/SliceOrientation.h
    include/qtviewerpro/core/VolumeData.h
    include/qtviewerpro/core/VolumeMetadata.h
)

add_library(qt-viewer-pro-core
    ${QTVIEWERPRO_CORE_SOURCES}
    ${QTVIEWERPRO_CORE_HEADERS}
)

target_include_directories(qt-viewer-pro-core
    PUBLIC
        include
)

# Processing library
set(QTVIEWERPRO_PROCESSING_SOURCES
    src/processing/ImageProcessor.cpp
    src/processing/SliceImageConverter.cpp
    src/processing/WindowLevelProcessor.cpp
)

set(QTVIEWERPRO_PROCESSING_HEADERS
    include/qtviewerpro/processing/ImageProcessor.h
    include/qtviewerpro/processing/SliceImageConverter.h
    include/qtviewerpro/processing/WindowLevelProcessor.h
)

add_library(qt-viewer-pro-processing
    ${QTVIEWERPRO_PROCESSING_SOURCES}
    ${QTVIEWERPRO_PROCESSING_HEADERS}
)

target_include_directories(qt-viewer-pro-processing
    PUBLIC
        include
    PRIVATE
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(qt-viewer-pro-processing
    PUBLIC
        qt-viewer-pro-core
    PRIVATE
        Qt6::Widgets
        ${OpenCV_LIBS}
)

# IO library
set(QTVIEWERPRO_IO_SOURCES
    src/io/RawVolumeLoader.cpp
)

set(QTVIEWERPRO_IO_HEADERS
    include/qtviewerpro/io/RawVolumeLoader.h
)

add_library(qt-viewer-pro-io
    ${QTVIEWERPRO_IO_SOURCES}
    ${QTVIEWERPRO_IO_HEADERS}
)

target_include_directories(qt-viewer-pro-io
    PUBLIC
        include
)

target_link_libraries(qt-viewer-pro-io
    PUBLIC
        qt-viewer-pro-core
    PRIVATE
        Qt6::Core
)

# Render library
set(QTVIEWERPRO_RENDER_SOURCES
    src/render/OpenGLSliceViewer.cpp
)

set(QTVIEWERPRO_RENDER_HEADERS
    include/qtviewerpro/render/OpenGLSliceViewer.h
)

add_library(qt-viewer-pro-render
    ${QTVIEWERPRO_RENDER_SOURCES}
    ${QTVIEWERPRO_RENDER_HEADERS}
)

target_include_directories(qt-viewer-pro-render
    PUBLIC
        include
)

target_link_libraries(qt-viewer-pro-render
    PUBLIC
        Qt6::OpenGLWidgets
)

# Application target
set(QTVIEWERPRO_APP_SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/ImageViewer2D.cpp
    src/io/ImageLoader.cpp
)

set(QTVIEWERPRO_APP_HEADERS
    include/qtviewerpro/ui/MainWindow.h
    include/qtviewerpro/ui/ImageViewer2D.h
    include/qtviewerpro/io/ImageLoader.h
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
        qt-viewer-pro-processing
        qt-viewer-pro-io
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

qt_add_executable(test-volume-metadata
    tests/TestVolumeMetadata.cpp
)

target_link_libraries(test-volume-metadata
    PRIVATE
        qt-viewer-pro-core
        Qt6::Test
)

add_test(NAME test-volume-metadata COMMAND test-volume-metadata)

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
)

target_link_libraries(test-window-level-processor
    PRIVATE
        qt-viewer-pro-processing
        Qt6::Test
)

add_test(NAME test-window-level-processor COMMAND test-window-level-processor)

qt_add_executable(test-slice-image-converter
    tests/TestSliceImageConverter.cpp
)

target_link_libraries(test-slice-image-converter
    PRIVATE
        qt-viewer-pro-processing
        Qt6::Widgets
        Qt6::Test
)

add_test(NAME test-slice-image-converter COMMAND test-slice-image-converter)

qt_add_executable(test-raw-volume-loader
    tests/TestRawVolumeLoader.cpp
)

target_link_libraries(test-raw-volume-loader
    PRIVATE
        qt-viewer-pro-io
        Qt6::Test
)

add_test(NAME test-raw-volume-loader COMMAND test-raw-volume-loader)
