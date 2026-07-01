# Qt Viewer Pro

[![CI](https://github.com/wpop/qt-viewer-pro/actions/workflows/ci.yml/badge.svg)](https://github.com/wpop/qt-viewer-pro/actions/workflows/ci.yml)

Qt Viewer Pro is a C++20 / Qt 6 desktop image and medical volume viewer built with Qt Widgets, OpenCV, OpenGL, ITK, and CMake.

This project extends the original Qt Viewer into a unified desktop application for 2D image viewing and medical volume exploration.

## Screenshot

![Qt Viewer Pro demo](docs/images/qt_viewer_grayscale.png)

## Current Features

- 2D image loading for PNG, JPG, JPEG, and BMP
- Zoom, pan, fit to window, and actual size controls
- 2D image processing actions, including rotate, flip, and OpenCV-backed grayscale conversion
- Embedded Medical Volume Viewer page
- OpenGL slice display
- NIfTI loading for `.nii` and `.nii.gz`
- MetaImage loading for `.mhd` and `.mha`
- DICOM series folder loading
- NRRD loading for `.nrrd` and `.nhdr`
- Custom float32 RAW loading with JSON metadata
- Axial, coronal, and sagittal slice navigation
- Window and level controls with CT presets
- Mask overlay loading
- Mask opacity control
- Crosshair voxel readout with base voxel value and optional mask value
- Medical volume metadata panel

## Technologies

- C++20
- Qt 6 Widgets for the GUI
- OpenCV for standard 2D image loading and image processing/color conversion
- OpenGL for medical slice rendering
- ITK for medical volume loading
- CMake
- Ninja
- CTest

## Project Structure

```text
qt-viewer-pro/
├── cmake/
├── docs/
├── include/
├── resources/
├── samples/
├── scripts/
├── src/
└── tests/
```

## Project Architecture

- `core` contains reusable data and slice logic, including `VolumeData`, `SliceData`, `SliceExtractor`, and `SliceOrientation`.
- `io` contains `ImageLoader`, `MedicalVolumeLoaderRegistry`, and the NIfTI, MetaImage, DICOM, NRRD, and RAW loaders.
- `processing` contains image processing utilities.
- `render` contains `OpenGLSliceViewer`.
- `ui` contains the desktop interface, including `MainWindow`, `ImageViewer2D`, and `OpenGLVolumeViewerWidget`.

Unit tests currently cover the core data structures, slice extraction, and the volume loader stack.

## Local Data Policy

- `data/` is ignored by Git.
- Medical datasets and generated samples are kept local only.
- Screenshots can be committed under `docs/images/`.

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/qt-viewer-pro
```

## Workflows

### Open Medical Volume

Use `File -> Open Medical Volume...` and choose any supported medical volume file:

```text
.nii
.nii.gz
.mhd
.mha
.dcm
.nrrd
.nhdr
```

### Open DICOM Series

Use `File -> Open DICOM Series Folder...` to load a directory of DICOM slices.

### Open Mask Overlay

Use `File -> Open Mask Overlay...` to load a mask volume for the active medical volume.

### Load Custom RAW Test Volume

Use `Tools -> Load Custom RAW Test Volume...`, then select:

```text
data/samples/raw/custom_test_volume/metadata.json
data/samples/raw/custom_test_volume/volume.raw
```

This workflow expects a JSON metadata object plus a separate float32 RAW file. MetaImage / LUNA16 `.raw` files should be opened via their `.mhd` file using `File -> Open Medical Volume...`.

## Testing

Configure and build the project first:

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Run unit tests with CTest:

```bash
ctest --test-dir build --output-on-failure
```

## Formatting

This project uses `clang-format` for C++ source formatting.

Format all C++ headers and source files:

```bash
find include src tests \( -name "*.h" -o -name "*.cpp" \) -print0 | xargs -0 clang-format -i
```

## License

MIT License
