#include "qtviewerpro/io/MedicalVolumeLoaderRegistry.h"

#include "qtviewerpro/io/DicomVolumeLoader.h"
#include "qtviewerpro/io/MetaImageVolumeLoader.h"
#include "qtviewerpro/io/NiftiVolumeLoader.h"
#include "qtviewerpro/io/NrrdVolumeLoader.h"
#include "qtviewerpro/io/RawVolumeLoader.h"

#include <QFileInfo>

namespace qvp
{

VolumeLoadResult loadMedicalVolume(const QString& path)
{
  if (QFileInfo(path).suffix().compare(QStringLiteral("json"), Qt::CaseInsensitive) == 0)
  {
    try
    {
      return VolumeLoadResult::makeSuccess(RawVolumeLoader::load(path));
    }
    catch (const std::exception& error)
    {
      return VolumeLoadResult::makeFailure(QString::fromUtf8(error.what()));
    }
  }

  const NiftiVolumeLoader niftiLoader;
  if (niftiLoader.canLoad(path))
  {
    return niftiLoader.load(path);
  }

  const MetaImageVolumeLoader metaImageLoader;
  if (metaImageLoader.canLoad(path))
  {
    return metaImageLoader.load(path);
  }

  const DicomVolumeLoader dicomLoader;
  if (dicomLoader.canLoad(path))
  {
    return dicomLoader.load(path);
  }

  const NrrdVolumeLoader nrrdLoader;
  if (nrrdLoader.canLoad(path))
  {
    return nrrdLoader.load(path);
  }

  return VolumeLoadResult::makeFailure(QStringLiteral("Unsupported medical volume format"));
}

} // namespace qvp
