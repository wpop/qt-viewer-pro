#include "qtviewerpro/io/MetaImageVolumeLoader.h"

#include <QFileInfo>

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
  (void)path;
  return VolumeLoadResult::makeFailure(
      QStringLiteral("MetaImage loading is not implemented yet"));
}

} // namespace qvp
