# Qt Viewer Pro

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

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/qt-viewer-pro
```

## License

MIT License
