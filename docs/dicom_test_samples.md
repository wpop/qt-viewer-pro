# DICOM Test Samples

## Why samples are local-only

DICOM sample data used for local testing must not be committed to the repository. The project already ignores the `data/` directory in `.gitignore`, so local sample series can be stored there without affecting Git status.

## Recommended local folder

Recommended local test location:

```text
data/samples/dicom/lung_series_3/
```

## Create a small local sample series

Create the local folder:

```bash
mkdir -p data/samples/dicom/lung_series_3
```

Copy a few DICOM slices from a local external dataset using placeholder source paths:

```bash
cp /path/to/source/series/000000.dcm data/samples/dicom/lung_series_3/
cp /path/to/source/series/000001.dcm data/samples/dicom/lung_series_3/
cp /path/to/source/series/000002.dcm data/samples/dicom/lung_series_3/
```

Example with an environment variable instead of a hard-coded path:

```bash
SOURCE_SERIES_DIR=/path/to/source/series
cp "$SOURCE_SERIES_DIR"/000000.dcm data/samples/dicom/lung_series_3/
cp "$SOURCE_SERIES_DIR"/000001.dcm data/samples/dicom/lung_series_3/
cp "$SOURCE_SERIES_DIR"/000002.dcm data/samples/dicom/lung_series_3/
```

## Test in the application

Recommended test flow:

1. Start the application.
2. Use `File -> Open DICOM Series Folder...`.
3. Select `data/samples/dicom/lung_series_3`.

Alternative test flow:

1. Use `File -> Open Medical Volume...`.
2. Select any one `.dcm` file from the same series folder.

Both flows should load the full series through the existing DICOM loader path.

## Notes about incomplete DICOM series

If only a few slices are copied, ITK may print a warning such as:

```text
Non uniform sampling or missing slices detected
```

This is expected when testing with an incomplete series. For better visual testing, copy a larger consecutive subset of slices from the same series.
