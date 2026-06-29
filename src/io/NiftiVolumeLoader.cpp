#include "qtviewerpro/io/NiftiVolumeLoader.h"

#include <QFileInfo>

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
  (void)path;
  return VolumeLoadResult::makeFailure(QStringLiteral("NIfTI loading is not implemented yet"));
}

} // namespace qvp
