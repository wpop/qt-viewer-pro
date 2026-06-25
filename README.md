# Qt Viewer Pro

[![CI](https://github.com/wpop/qt-viewer-pro/actions/workflows/ci.yml/badge.svg)](https://github.com/wpop/qt-viewer-pro/actions/workflows/ci.yml)

Qt Viewer Pro is a lightweight C++20 image viewer built with Qt Widgets, CMake, and OpenCV.

This project is a professional extension of the original Qt Viewer project. The current version focuses on a clean desktop image viewer architecture. Future versions will add OpenGL rendering, medical volume visualization, overlays, and optional medical imaging preprocessing support.

## Screenshot

![Qt Viewer grayscale demo](docs/images/qt_viewer_grayscale.png)

## Current Features

- Load and display images
- Zoom in and out
- Pan images
- Fit image to window
- Actual size view
- Basic image preprocessing support
- Clean C++/Qt project structure

## Planned Features

- OpenGL-based image rendering
- Slice viewer for volume data
- Window/level controls
- Segmentation mask overlays
- Medical image preprocessing bridge
- Optional CUDA experiments later

## Technologies

- C++20
- Qt 6
- CMake
- OpenCV
- OpenGL planned
- SimpleITK preprocessing planned

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

- `core` contains reusable data and slice logic, including `VolumeData`, `SliceOrientation`, and `SliceExtractor`. It is built as the separate `qt-viewer-pro-core` CMake library target.
- `io` handles image loading.
- `processing` contains image processing utilities.
- `ui` contains the desktop interface, including `MainWindow` and `ImageViewer2D`.

Unit tests currently cover core data structures and slice extraction.

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/qt-viewer-pro
```

## RAW Volume Demo

Generate local sample RAW volume files:

```bash
python3 scripts/create_raw_volume_sample.py
```

Build and run the app:

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/qt-viewer-pro
```

In the app, choose `Demo -> Open RAW Volume...`, then select:

```text
data/samples/raw_volume/volume.json
data/samples/raw_volume/volume.raw
```

Use `Z-` / `Z+` toolbar buttons or `PageUp` / `PageDown` to navigate slices.
The generated data files are local test files and are not committed.

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
