# Qt Viewer Pro Architecture

Qt Viewer Pro is a C++20 / Qt 6 desktop application with two main workflows:

- 2D image viewing
- medical volume viewing

## High-Level Structure

```text
MainWindow
  QStackedWidget
    ImageViewer2D
    OpenGLVolumeViewerWidget

OpenGLVolumeViewerWidget
  OpenGLSliceViewer
  VolumeData
  SliceExtractor
  SliceImageConverter
  mask overlay
  crosshair readout
  window/level controls

MedicalVolumeLoaderRegistry
  NiftiVolumeLoader
  MetaImageVolumeLoader
  DicomVolumeLoader
  NrrdVolumeLoader

RawVolumeLoader
  custom float32 RAW + JSON metadata workflow
```

## Modules

- `core` owns reusable volume and slice structures such as `VolumeData`, `SliceData`, `SliceExtractor`, and `SliceOrientation`.
- `io` owns file and volume loading, including `ImageLoader`, `MedicalVolumeLoaderRegistry`, the format-specific medical volume loaders, and the standalone RAW workflow loader.
- `processing` owns image processing utilities used by the 2D workflow.
- `render` owns the OpenGL display layer, centered on `OpenGLSliceViewer`.
- `ui` owns the desktop interface, including `MainWindow`, `ImageViewer2D`, and `OpenGLVolumeViewerWidget`.

## Data Flow

### 2D Images

```text
Image file -> ImageLoader / OpenCV -> QImage -> ImageViewer2D
```

### Medical Volumes

```text
Medical file -> MedicalVolumeLoaderRegistry -> VolumeData -> SliceExtractor -> SliceImageConverter -> OpenGLSliceViewer
```

### DICOM Folder Loading

```text
DICOM folder -> DicomVolumeLoader::loadSeriesDirectory() -> VolumeData
```

### Mask Overlay

```text
Mask volume -> VolumeData -> SliceExtractor -> applyMaskOverlay() -> OpenGLSliceViewer
```

## Responsibilities

- `MainWindow` routes actions and switches pages.
- `OpenGLVolumeViewerWidget` owns the medical viewer state and controls.
- Loaders own format-specific loading.
- `core` owns reusable volume and slice structures.
- `render` owns OpenGL display.
