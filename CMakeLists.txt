cmake_minimum_required(VERSION 3.20)

# Project setup
project(QtViewerPro VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGLWidgets Test Concurrent)
find_package(OpenCV REQUIRED COMPONENTS imgcodecs imgproc)

# The Ubuntu ITK package may try to load ITKVtkGlue, which pulls VTK/MPI.
# qt-viewer-pro does not use ITK-VTK glue, so mark it as already loaded.
set(ITKVtkGlue_LOADED TRUE)
find_package(ITK REQUIRED COMPONENTS
    ITKCommon
    ITKIOGDCM
    ITKIOImageBase
    ITKImageFunction
    ITKImageGrid
    ITKIOMeta
    ITKIONIFTI
    ITKIONRRD
    ITKTransform
)
include(${ITK_USE_FILE})

# Qt 6.2 on Ubuntu 22.04 does not provide qt_standard_project_setup().
# This project only needs AUTOMOC for Q_OBJECT-based sources.
set(CMAKE_AUTOMOC ON)
enable_testing()

# Core library
set(QTVIEWERPRO_CORE_SOURCES
    src/core/AnatomicalOrientation.cpp
    src/core/SliceData.cpp
    src/core/SliceExtractor.cpp
    src/core/VolumeData.cpp
    src/core/VolumeInformation.cpp
    src/core/VolumePhysicalCoordinateMapper.cpp
    src/core/VolumeMetadata.cpp
)

set(QTVIEWERPRO_CORE_HEADERS
    include/qtviewerpro/core/AnatomicalOrientation.h
    include/qtviewerpro/core/SliceData.h
    include/qtviewerpro/core/SliceExtractor.h
    include/qtviewerpro/core/SliceOrientation.h
    include/qtviewerpro/core/VolumeData.h
    include/qtviewerpro/core/VolumeInformation.h
    include/qtviewerpro/core/VolumePhysicalCoordinateMapper.h
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
    src/processing/VolumeResampler.cpp
    src/processing/WindowLevelProcessor.cpp
)

set(QTVIEWERPRO_PROCESSING_HEADERS
    include/qtviewerpro/processing/ImageProcessor.h
    include/qtviewerpro/processing/SliceImageConverter.h
    include/qtviewerpro/processing/VolumeResampler.h
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
        Qt6::Concurrent
        ${OpenCV_LIBS}
        ${ITK_LIBRARIES}
)

# IO library
set(QTVIEWERPRO_IO_SOURCES
    src/io/DicomVolumeLoader.cpp
    src/io/ItkVolumeConverter.cpp
    src/io/MedicalVolumeLoaderRegistry.cpp
    src/io/MetaImageVolumeLoader.cpp
    src/io/NiftiVolumeLoader.cpp
    src/io/NrrdVolumeLoader.cpp
    src/io/RawVolumeLoader.cpp
)

set(QTVIEWERPRO_IO_HEADERS
    include/qtviewerpro/io/DicomVolumeLoader.h
    include/qtviewerpro/io/ItkVolumeConverter.h
    include/qtviewerpro/io/MedicalVolumeLoader.h
    include/qtviewerpro/io/MedicalVolumeLoaderRegistry.h
    include/qtviewerpro/io/MetaImageVolumeLoader.h
    include/qtviewerpro/io/NiftiVolumeLoader.h
    include/qtviewerpro/io/NrrdVolumeLoader.h
    include/qtviewerpro/io/RawVolumeLoader.h
    include/qtviewerpro/io/VolumeLoadResult.h
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
        ${ITK_LIBRARIES}
)

# Render library
set(QTVIEWERPRO_RENDER_SOURCES
    src/render/OpenGLSliceViewer.cpp
    src/render/OpenGLVolumeRendererWidget.cpp
)

set(QTVIEWERPRO_RENDER_HEADERS
    include/qtviewerpro/render/OpenGLSliceViewer.h
    include/qtviewerpro/render/OpenGLVolumeRendererWidget.h
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
    src/ui/MprCoordinateMapper.cpp
    src/ui/MprOrientationLabelMapper.cpp
    src/ui/MprViewerWidget.cpp
    src/ui/OpenGLVolumeViewerWidget.cpp
    src/ui/Volume3DViewerWidget.cpp
    src/ui/ImageViewer2D.cpp
    src/io/ImageLoader.cpp
)

set(QTVIEWERPRO_APP_HEADERS
    include/qtviewerpro/ui/MainWindow.h
    include/qtviewerpro/ui/MprCoordinateMapper.h
    include/qtviewerpro/ui/MprOrientationLabelMapper.h
    include/qtviewerpro/ui/MprViewerWidget.h
    include/qtviewerpro/ui/OpenGLVolumeViewerWidget.h
    include/qtviewerpro/ui/Volume3DViewerWidget.h
    include/qtviewerpro/ui/ImageViewer2D.h
    include/qtviewerpro/io/ImageLoader.h
)

qt6_add_executable(qt-viewer-pro
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
        qt-viewer-pro-render
        Qt6::Widgets
        ${OpenCV_LIBS}
)

# Tests
qt6_add_executable(test-volume-data
    tests/TestVolumeData.cpp
)

target_link_libraries(test-volume-data
    PRIVATE
        qt-viewer-pro-core
        Qt6::Test
)

add_test(NAME test-volume-data COMMAND test-volume-data)

qt6_add_executable(test-volume-metadata
    tests/TestVolumeMetadata.cpp
)

target_link_libraries(test-volume-metadata
    PRIVATE
        qt-viewer-pro-core
        Qt6::Test
)

add_test(NAME test-volume-metadata COMMAND test-volume-metadata)

qt6_add_executable(test-anatomical-orientation
    tests/TestAnatomicalOrientation.cpp
)

target_link_libraries(test-anatomical-orientation
    PRIVATE
        qt-viewer-pro-core
        Qt6::Test
)

add_test(NAME test-anatomical-orientation COMMAND test-anatomical-orientation)

qt6_add_executable(test-volume-physical-coordinate-mapper
    tests/TestVolumePhysicalCoordinateMapper.cpp
)

target_link_libraries(test-volume-physical-coordinate-mapper
    PRIVATE
        qt-viewer-pro-core
        Qt6::Test
)

add_test(NAME test-volume-physical-coordinate-mapper COMMAND test-volume-physical-coordinate-mapper)

qt6_add_executable(test-volume-information
    tests/TestVolumeInformation.cpp
)

target_link_libraries(test-volume-information
    PRIVATE
        qt-viewer-pro-core
        Qt6::Test
)

add_test(NAME test-volume-information COMMAND test-volume-information)

qt6_add_executable(test-slice-data
    tests/TestSliceData.cpp
)

target_link_libraries(test-slice-data
    PRIVATE
        qt-viewer-pro-core
        Qt6::Test
)

add_test(NAME test-slice-data COMMAND test-slice-data)

qt6_add_executable(test-slice-extractor
    tests/TestSliceExtractor.cpp
)

target_link_libraries(test-slice-extractor
    PRIVATE
        qt-viewer-pro-core
        Qt6::Test
)

add_test(NAME test-slice-extractor COMMAND test-slice-extractor)

qt6_add_executable(test-mpr-coordinate-mapper
    tests/TestMprCoordinateMapper.cpp
    src/ui/MprCoordinateMapper.cpp
)

target_include_directories(test-mpr-coordinate-mapper
    PRIVATE
        include
)

target_link_libraries(test-mpr-coordinate-mapper
    PRIVATE
        qt-viewer-pro-core
        Qt6::Test
)

add_test(NAME test-mpr-coordinate-mapper COMMAND test-mpr-coordinate-mapper)

qt6_add_executable(test-mpr-orientation-label-mapper
    tests/TestMprOrientationLabelMapper.cpp
    src/ui/MprOrientationLabelMapper.cpp
)

target_include_directories(test-mpr-orientation-label-mapper
    PRIVATE
        include
)

target_link_libraries(test-mpr-orientation-label-mapper
    PRIVATE
        qt-viewer-pro-core
        Qt6::Test
)

add_test(NAME test-mpr-orientation-label-mapper COMMAND test-mpr-orientation-label-mapper)

qt6_add_executable(test-window-level-processor
    tests/TestWindowLevelProcessor.cpp
)

target_link_libraries(test-window-level-processor
    PRIVATE
        qt-viewer-pro-processing
        Qt6::Test
)

add_test(NAME test-window-level-processor COMMAND test-window-level-processor)

qt6_add_executable(test-volume-resampler
    tests/TestVolumeResampler.cpp
)

target_link_libraries(test-volume-resampler
    PRIVATE
        qt-viewer-pro-processing
        Qt6::Test
        ${ITK_LIBRARIES}
)

add_test(NAME test-volume-resampler COMMAND test-volume-resampler)

qt6_add_executable(test-slice-image-converter
    tests/TestSliceImageConverter.cpp
)

target_link_libraries(test-slice-image-converter
    PRIVATE
        qt-viewer-pro-processing
        Qt6::Widgets
        Qt6::Test
)

add_test(NAME test-slice-image-converter COMMAND test-slice-image-converter)

qt6_add_executable(test-raw-volume-loader
    tests/TestRawVolumeLoader.cpp
)

target_link_libraries(test-raw-volume-loader
    PRIVATE
        qt-viewer-pro-io
        Qt6::Test
)

add_test(NAME test-raw-volume-loader COMMAND test-raw-volume-loader)

qt6_add_executable(test-nifti-volume-loader
    tests/TestNiftiVolumeLoader.cpp
)

target_link_libraries(test-nifti-volume-loader
    PRIVATE
        qt-viewer-pro-io
        Qt6::Test
        ${ITK_LIBRARIES}
)

add_test(NAME test-nifti-volume-loader COMMAND test-nifti-volume-loader)

qt6_add_executable(test-metaimage-volume-loader
    tests/TestMetaImageVolumeLoader.cpp
)

target_link_libraries(test-metaimage-volume-loader
    PRIVATE
        qt-viewer-pro-io
        Qt6::Test
        ${ITK_LIBRARIES}
)

add_test(NAME test-metaimage-volume-loader COMMAND test-metaimage-volume-loader)

qt6_add_executable(test-nrrd-volume-loader
    tests/TestNrrdVolumeLoader.cpp
)

target_link_libraries(test-nrrd-volume-loader
    PRIVATE
        qt-viewer-pro-io
        Qt6::Test
        ${ITK_LIBRARIES}
)

add_test(NAME test-nrrd-volume-loader COMMAND test-nrrd-volume-loader)
