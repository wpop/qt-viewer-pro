#include "qtviewerpro/io/NrrdVolumeLoader.h"

#include "qtviewerpro/io/ItkVolumeConverter.h"

#include <QFileInfo>

#include <itkImageFileReader.h>
#include <itkMetaDataObject.h>
#include <itkNrrdImageIO.h>

#include <exception>
#include <string>

namespace
{

bool hasNrrdExtension(const QString& path)
{
  const QString lowerFileName = QFileInfo(path).fileName().toLower();
  return lowerFileName.endsWith(QStringLiteral(".nrrd")) ||
         lowerFileName.endsWith(QStringLiteral(".nhdr"));
}

bool hasTrustedNrrdOrientation(const itk::MetaDataDictionary& dictionary)
{
  std::string space;
  if (!itk::ExposeMetaData<std::string>(dictionary, "NRRD_space", space))
  {
    return false;
  }

  return space == "left-posterior-superior";
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
    const auto nrrdImageIO = itk::NrrdImageIO::New();
    const auto reader = ReaderType::New();
    reader->SetFileName(path.toStdString());
    reader->SetImageIO(nrrdImageIO);
    reader->Update();

    const auto image = reader->GetOutput();
    if (!image)
    {
      return VolumeLoadResult::makeFailure(QStringLiteral("NRRD image buffer is null"));
    }

    ItkSpatialGeometryPolicy policy;
    policy.hasTrustedOrientation =
        hasTrustedNrrdOrientation(nrrdImageIO->GetMetaDataDictionary());
    policy.coordinateSystem = VolumeData::CoordinateSystem::LPS;

    const VolumeData volume =
        convertItkImageToVolume(image, policy, "NRRD");
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
