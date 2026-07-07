#include "qtviewerpro/io/MetaImageVolumeLoader.h"

#include "qtviewerpro/io/ItkVolumeConverter.h"

#include <QFileInfo>

#include <itkImageFileReader.h>

#include <exception>

namespace
{

bool hasMetaImageExtension(const QString& path)
{
  const QString lowerFileName = QFileInfo(path).fileName().toLower();
  return lowerFileName.endsWith(QStringLiteral(".mhd")) ||
         lowerFileName.endsWith(QStringLiteral(".mha"));
}

} // namespace

namespace qvp
{

bool MetaImageVolumeLoader::canLoad(const QString& path) const
{
  return hasMetaImageExtension(path);
}

VolumeLoadResult MetaImageVolumeLoader::load(const QString& path) const
{
  using ReaderType = itk::ImageFileReader<ItkVolumeImage>;

  try
  {
    const auto reader = ReaderType::New();
    reader->SetFileName(path.toStdString());
    reader->Update();

    const VolumeData volume =
        convertItkImageToVolume(reader->GetOutput(), ItkSpatialGeometryPolicy{}, "MetaImage");
    if (!volume.isValid())
    {
      return VolumeLoadResult::makeFailure(QStringLiteral("Loaded MetaImage volume is invalid"));
    }

    return VolumeLoadResult::makeSuccess(volume);
  }
  catch (const itk::ExceptionObject& exception)
  {
    return VolumeLoadResult::makeFailure(
        QStringLiteral("Failed to load MetaImage volume: ") + exception.GetDescription());
  }
  catch (const std::exception& exception)
  {
    return VolumeLoadResult::makeFailure(
        QStringLiteral("Failed to load MetaImage volume: ") + QString::fromUtf8(exception.what()));
  }
}

} // namespace qvp
