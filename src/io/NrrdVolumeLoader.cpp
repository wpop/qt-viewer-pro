#include "qtviewerpro/io/NrrdVolumeLoader.h"

#include <QFileInfo>

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
  (void)path;
  return VolumeLoadResult::makeFailure(QStringLiteral("NRRD loading is not implemented yet"));
}

} // namespace qvp
