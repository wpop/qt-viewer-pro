#include "qtviewerpro/io/DicomVolumeLoader.h"

#include <QFileInfo>

namespace
{

bool hasDicomExtension(const QString& path)
{
  const QFileInfo fileInfo(path);
  if (fileInfo.isDir())
  {
    return false;
  }

  return fileInfo.fileName().toLower().endsWith(QStringLiteral(".dcm"));
}

} // namespace

namespace qvp
{

bool DicomVolumeLoader::canLoad(const QString& path) const
{
  return hasDicomExtension(path);
}

VolumeLoadResult DicomVolumeLoader::load(const QString& path) const
{
  (void)path;
  return VolumeLoadResult::makeFailure(QStringLiteral("DICOM loading is not implemented yet"));
}

} // namespace qvp
