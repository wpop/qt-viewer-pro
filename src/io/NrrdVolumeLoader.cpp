#include "qtviewerpro/io/NrrdVolumeLoader.h"

#include "qtviewerpro/io/ItkVolumeConverter.h"

#include <QFileInfo>

#include <itkImageFileReader.h>

#include <exception>

namespace
{

bool hasNrrdExtension(const QString& path)
{
  const QString lowerFileName = QFileInfo(path).fileName().toLower();
  return lowerFileName.endsWith(QStringLiteral(".nrrd")) ||
         lowerFileName.endsWith(QStringLiteral(".nhdr"));
}

} // namespace

namespace qvp
{

bool NrrdVolumeLoader::canLoad(const QString& path) const
{
  return hasNrrdExtension(path);
}

VolumeLoadResult NrrdVolumeLoader::load(const QString& path) const
{
  using ReaderType = itk::ImageFileReader<ItkVolumeImage>;

  try
  {
    const auto reader = ReaderType::New();
    reader->SetFileName(path.toStdString());
    reader->Update();

    const auto image = reader->GetOutput();
    if (!image)
    {
      return VolumeLoadResult::makeFailure(QStringLiteral("NRRD image buffer is null"));
    }

    const VolumeData volume =
        convertItkImageToVolume(image, ItkSpatialGeometryPolicy{}, "NRRD");
    if (!volume.isValid())
    {
      return VolumeLoadResult::makeFailure(QStringLiteral("Loaded NRRD volume is invalid"));
    }
    return VolumeLoadResult::makeSuccess(volume);
  }
  catch (const itk::ExceptionObject& exception)
  {
    return VolumeLoadResult::makeFailure(
        QStringLiteral("Failed to load NRRD volume: ") + exception.GetDescription());
  }
  catch (const std::exception& exception)
  {
    return VolumeLoadResult::makeFailure(
        QStringLiteral("Failed to load NRRD volume: ") + QString::fromUtf8(exception.what()));
  }
}

} // namespace qvp
