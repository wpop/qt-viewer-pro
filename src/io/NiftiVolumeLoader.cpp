#include "qtviewerpro/io/NiftiVolumeLoader.h"

#include "qtviewerpro/io/ItkVolumeConverter.h"

#include <QFileInfo>

#include <itkImageFileReader.h>

#include <exception>

namespace
{

bool hasNiftiExtension(const QString& path)
{
  const QString lowerFileName = QFileInfo(path).fileName().toLower();
  return lowerFileName.endsWith(QStringLiteral(".nii")) ||
         lowerFileName.endsWith(QStringLiteral(".nii.gz"));
}

} // namespace

namespace qvp
{

bool NiftiVolumeLoader::canLoad(const QString& path) const
{
  return hasNiftiExtension(path);
}

VolumeLoadResult NiftiVolumeLoader::load(const QString& path) const
{
  using ReaderType = itk::ImageFileReader<ItkVolumeImage>;

  try
  {
    const auto reader = ReaderType::New();
    reader->SetFileName(path.toStdString());
    reader->Update();

    const VolumeData volume =
        convertItkImageToVolume(reader->GetOutput(), ItkSpatialGeometryPolicy{}, "NIfTI");
    if (!volume.isValid())
    {
      return VolumeLoadResult::makeFailure(QStringLiteral("Loaded NIfTI volume is invalid"));
    }

    return VolumeLoadResult::makeSuccess(volume);
  }
  catch (const itk::ExceptionObject& exception)
  {
    return VolumeLoadResult::makeFailure(
        QStringLiteral("Failed to load NIfTI volume: ") + exception.GetDescription());
  }
  catch (const std::exception& exception)
  {
    return VolumeLoadResult::makeFailure(
        QStringLiteral("Failed to load NIfTI volume: ") + QString::fromUtf8(exception.what()));
  }
}

} // namespace qvp
