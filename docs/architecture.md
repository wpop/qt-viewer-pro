# Qt Viewer Pro Architecture

## High-Level Architecture

Qt Viewer Pro is a Qt Widgets desktop application with four coordinated viewing surfaces:

- `ImageViewer2D` for general 2D images
- `OpenGLVolumeViewerWidget` for interactive 2D medical slice viewing
- `MprViewerWidget` for synchronized axial / sagittal / coronal MPR
- `Volume3DViewerWidget` for interactive OpenGL 3D volume rendering

`MainWindow` owns the top-level `QStackedWidget`, menus, actions, recent-file state, and the signal wiring that keeps the medical viewers synchronized without duplicating canonical state.

## Core Data Model

The core layer centers on immutable or value-type data models and pure transformation helpers:

- `VolumeData` stores voxel data, dimensions, spacing, cached intensity range, and spatial geometry
- `VolumeData::SpatialGeometry` stores origin, row-major direction, coordinate system, and orientation trust metadata
- `SliceData` represents extracted 2D voxel planes
- `SliceOrientation` identifies axial, sagittal, and coronal slice conventions
- `VolumeInformation` provides a read-only summary model for UI display
- `VolumePhysicalCoordinateMapper` converts voxel indices into physical patient-space coordinates
- `MprCoordinateMapper` and `MprOrientationLabelMapper` provide pure MPR mapping and orientation-label logic

Medical viewers consume shared `VolumeData` instances through `std::shared_ptr<const VolumeData>` so the same loaded volume can be displayed in multiple views without copying voxel buffers.

## Medical Volume Loading

Medical loading is organized behind `MedicalVolumeLoaderRegistry`, which selects the appropriate format-specific loader for:

- DICOM series selected from one DICOM file, with the full series discovered from the same directory
- NIfTI (`.nii`, `.nii.gz`)
- MetaImage (`.mhd`, `.mha`)
- NRRD (`.nrrd`, `.nhdr`)
- custom float32 RAW workflow driven by JSON metadata

Each loader produces a `VolumeData` object with voxel data, spacing, intensity-range cache, and whatever spatial metadata is safe to trust for that format. `MainWindow` owns the currently loaded medical volume and passes shared ownership to the relevant viewing surfaces.

## MPR Pipeline

`MprViewerWidget` is the canonical owner of synchronized slice state for MPR. Its core responsibilities are:

- storing the shared current voxel position
- clamping and updating slice indices
- extracting axial, sagittal, and coronal slices from the shared volume
- converting slices into display images
- updating pane-specific crosshairs and voxel-coordinate labels
- emitting navigation updates for external UI synchronization

The effective pipeline is:

```text
VolumeData
  -> SliceExtractor
  -> SliceImageConverter
  -> ImageViewer2D pane
  -> crosshair / coordinate overlay updates
```

Clicks in any MPR pane map back through `MprCoordinateMapper`, update the single shared voxel position once, refresh all panes, and emit navigation state so external controls stay synchronized.

## Coordinate and Orientation Handling

Spatial metadata is carried in `VolumeData::SpatialGeometry`:

- `origin`
- `direction` in row-major order
- `coordinateSystem` (`LPS`, `RAS`, or `Unknown`)
- `hasOrientation`

When geometry is trusted, physical coordinates are computed as:

```text
physical = origin + direction * (index * spacing)
```

Trusted orientation and coordinate-system data also drive:

- anatomical edge labels in MPR
- patient-coordinate display in MPR
- volume-information presentation

When geometry is not trusted, voxel coordinates remain available while anatomical/patient-space presentation is withheld or marked untrusted rather than inferred.

## 2D Medical Slice Rendering

`OpenGLVolumeViewerWidget` owns the single-slice medical viewing workflow outside MPR. It is responsible for:

- orientation selection
- slice index control
- window/level control and CT presets
- optional mask overlay loading and blending
- readout and status updates for the active slice

It consumes the shared `VolumeData`, extracts the current slice, converts it to a 2D image, and pushes the result into the OpenGL-backed slice viewer path. This widget owns slice-view UI state, but it does not own application-wide medical-volume lifetime.

## 3D Volume Rendering

`Volume3DViewerWidget` is a thin QWidget wrapper around `OpenGLVolumeRendererWidget`. The canonical 3D rendering state lives in `OpenGLVolumeRendererWidget`, which owns:

- the active `VolumeData` reference
- GPU texture lifecycle
- source intensity range cache
- camera and interaction state
- render preset / manual transfer-function state

The renderer uploads voxel data into a 3D OpenGL texture when the loaded volume changes, then performs interactive rendering updates through state changes and uniform updates rather than re-uploading the volume for every control adjustment. Physical-spacing-aware model scaling keeps anisotropic datasets visually proportionate in 3D.

## Transfer-Function State

The 3D transfer-function state is represented by `VolumeTransferFunctionState`, which currently tracks:

- active render mode / preset
- global opacity
- manual intensity minimum
- manual intensity maximum

Named presets (`Default`, `CT Bone`, `CT Lung`) keep their preset-driven behavior. Manual intensity or opacity edits switch the renderer into `Custom` mode. `OpenGLVolumeRendererWidget` is the only canonical owner of that state; `Volume3DViewerWidget` only forwards renderer APIs, and the UI reflects renderer state rather than shadowing it.

## Volume Tools

`VolumeToolsWindow` is a floating, non-modal control window. It does not own canonical medical or render state. Instead, it:

- displays read-only volume information derived from `VolumeInformation`
- exposes MPR navigation controls
- exposes 3D transfer-function controls
- emits user requests back to `MainWindow`

`MainWindow` creates it lazily, connects its signals to the existing viewer owners, and pushes current navigation and 3D renderer state back into the window when needed.

## Resampling

`VolumeResampler` provides isotropic resampling for medical volumes. It converts `VolumeData` into an ITK image, runs resampling with preserved origin and direction, and converts the result back into a new `VolumeData`. This work is launched asynchronously from `MainWindow` so the UI stays responsive while resampling is in progress.

## Ownership and State Boundaries

The release architecture relies on a few explicit ownership rules:

- `VolumeData` is shared across medical viewers using shared ownership.
- `MainWindow` owns application-level page switching, menus, and cross-component wiring.
- `OpenGLVolumeViewerWidget` owns 2D medical slice-view UI state.
- `MprViewerWidget` owns synchronized slice/current-position state for MPR.
- `OpenGLVolumeRendererWidget` owns canonical 3D render and transfer-function state.
- `VolumeToolsWindow` does not own canonical medical, navigation, or render state.

These boundaries keep the viewer synchronized without introducing duplicate authoritative state for voxel position, render preset, or transfer-function parameters.

## Testing

The automated test suite currently covers 17 areas across pure logic and loader behavior, including:

- volume data and metadata
- anatomical orientation formatting
- physical coordinate mapping
- volume information modeling
- transfer-function state helpers
- slice extraction and slice image conversion
- MPR coordinate mapping and orientation labels
- window/level processing
- isotropic resampling
- RAW, NIfTI, MetaImage, and NRRD loader behavior
