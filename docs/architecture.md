# Qt Viewer Pro Architecture

Qt Viewer Pro is a C++20 desktop image viewer built with Qt 6 Widgets and
OpenCV. The current codebase is intentionally small and organized around three
modules under the `qvp` namespace.

## Current Modules

### ui

The `ui` module owns the Qt Widgets user interface.

- `qvp::MainWindow` is the top-level application window. It creates menus,
  toolbar actions, status updates, recent-file handling, and coordinates image
  loading, processing, and display.
- `qvp::ImageViewer2D` is the 2D image display widget. It handles presentation
  operations such as fit-to-window, actual size, zoom, rotate, flip, and
  drag-and-drop image file input.

### io

The `io` module owns file input responsibilities.

- `qvp::ImageLoader` loads image files from disk and converts them into `QImage`
  instances for the rest of the application.

### processing

The `processing` module owns image-processing operations that are independent of
the UI.

- `qvp::ImageProcessor` currently provides grayscale conversion for `QImage`
  instances.

## Namespace

Project classes live in the `qvp` namespace to keep application types separate
from Qt, OpenCV, and future library integrations.

## Data Flow

The current image workflow is:

1. The user opens or drops an image.
2. `qvp::MainWindow` receives the request.
3. `qvp::MainWindow` uses `qvp::ImageLoader` to load the file into a `QImage`.
4. Optional actions use `qvp::ImageProcessor` to create a processed image.
5. `qvp::ImageViewer2D` displays the current image and handles view operations.

## Future Planned Modules

The following areas are planned as future architecture boundaries and are not
implemented yet:

- `core`: shared application models, state, and non-UI abstractions.
- `rendering`: rendering interfaces and common rendering infrastructure.
- medical preprocessing bridge: an integration layer for future medical-image
  preprocessing workflows.
- OpenGL renderer: a future rendering backend for GPU-accelerated display.
- volume data: data structures and services for future 3D or volumetric image
  workflows.

The current project does not implement DICOM, ITK, CUDA, SimpleITK, OpenGL
volume rendering, or volume-data support.
