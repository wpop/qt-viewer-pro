#include "qtviewerpro/io/MedicalVolumeLoaderRegistry.h"

#include "qtviewerpro/io/DicomVolumeLoader.h"
#include "qtviewerpro/io/MetaImageVolumeLoader.h"
#include "qtviewerpro/io/NiftiVolumeLoader.h"
#include "qtviewerpro/io/NrrdVolumeLoader.h"

namespace qvp
{

VolumeLoadResult loadMedicalVolume(const QString& path)
{
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
