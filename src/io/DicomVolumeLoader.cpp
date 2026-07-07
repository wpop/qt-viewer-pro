#include "qtviewerpro/io/DicomVolumeLoader.h"

#include "qtviewerpro/io/ItkVolumeConverter.h"

#include <QDir>
#include <QFileInfo>

#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImageSeriesReader.h>

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

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

QString normalizedPath(const QString& path)
{
  const QFileInfo fileInfo(path);
  const QString canonicalPath = fileInfo.canonicalFilePath();
  if (!canonicalPath.isEmpty())
  {
    return QDir::cleanPath(canonicalPath);
  }

  return QDir::cleanPath(fileInfo.absoluteFilePath());
}

bool seriesContainsFile(const std::vector<std::string>& fileNames, const QString& selectedPath)
{
  return std::any_of(fileNames.begin(), fileNames.end(), [&selectedPath](const std::string& fileName) {
    return normalizedPath(QString::fromStdString(fileName)) == selectedPath;
  });
}

} // namespace

namespace qvp
{

namespace
{

bool hasDicomPatientOrientationTags(const std::string& fileName)
{
  const auto imageIO = itk::GDCMImageIO::New();
  imageIO->SetFileName(fileName);

  try
  {
    imageIO->ReadImageInformation();
  }
  catch (...)
  {
    return false;
  }

  std::string imageOrientationPatient;
  std::string imagePositionPatient;
  return imageIO->GetValueFromTag("0020|0037", imageOrientationPatient) &&
         !imageOrientationPatient.empty() &&
         imageIO->GetValueFromTag("0020|0032", imagePositionPatient) &&
         !imagePositionPatient.empty();
}

VolumeLoadResult loadSeriesFromDirectory(const QString& directoryPath,
                                         const QString* selectedFilePath)
{
  using ImageIOType = itk::GDCMImageIO;
  using NamesGeneratorType = itk::GDCMSeriesFileNames;
  using ReaderType = itk::ImageSeriesReader<ItkVolumeImage>;

  const QFileInfo directoryInfo(directoryPath);
  if (!directoryInfo.exists() || !directoryInfo.isDir())
  {
    return VolumeLoadResult::makeFailure(QStringLiteral("Selected DICOM directory does not exist"));
  }

  const auto namesGenerator = NamesGeneratorType::New();
  namesGenerator->SetUseSeriesDetails(true);
  namesGenerator->SetDirectory(directoryPath.toStdString());

  const auto& seriesIds = namesGenerator->GetSeriesUIDs();
  if (seriesIds.empty())
  {
    return VolumeLoadResult::makeFailure(QStringLiteral("No DICOM series found in directory"));
  }

  std::vector<std::string> fileNames;
  if (selectedFilePath != nullptr)
  {
    for (const auto& seriesId : seriesIds)
    {
      const auto seriesFileNames = namesGenerator->GetFileNames(seriesId);
      if (seriesContainsFile(seriesFileNames, *selectedFilePath))
      {
        fileNames = seriesFileNames;
        break;
      }
    }
  }

  if (fileNames.empty())
  {
    fileNames = namesGenerator->GetFileNames(seriesIds.front());
  }

  if (fileNames.empty())
  {
    return VolumeLoadResult::makeFailure(QStringLiteral("No DICOM series found in directory"));
  }

  try
  {
    const auto imageIO = ImageIOType::New();
    const auto reader = ReaderType::New();
    reader->SetImageIO(imageIO);
    reader->SetFileNames(fileNames);
    reader->Update();

    const std::string metadataFileName =
        selectedFilePath != nullptr ? selectedFilePath->toStdString() : fileNames.front();
    const ItkSpatialGeometryPolicy geometryPolicy{
        hasDicomPatientOrientationTags(metadataFileName),
        VolumeData::CoordinateSystem::LPS};
    const VolumeData volume = convertItkImageToVolume(reader->GetOutput(),
                                                      geometryPolicy,
                                                      "DICOM");
    if (!volume.isValid())
    {
      return VolumeLoadResult::makeFailure(QStringLiteral("Loaded DICOM volume is invalid"));
    }

    return VolumeLoadResult::makeSuccess(volume);
  }
  catch (const itk::ExceptionObject& exception)
  {
    return VolumeLoadResult::makeFailure(
        QStringLiteral("Failed to load DICOM series: ") + exception.GetDescription());
  }
  catch (const std::exception& exception)
  {
    return VolumeLoadResult::makeFailure(
        QStringLiteral("Failed to load DICOM series: ") + QString::fromUtf8(exception.what()));
  }
}

} // namespace

bool DicomVolumeLoader::canLoad(const QString& path) const
{
  return hasDicomExtension(path);
}

VolumeLoadResult DicomVolumeLoader::load(const QString& path) const
{
  const QFileInfo selectedFileInfo(path);
  if (!selectedFileInfo.exists() || !selectedFileInfo.isFile())
  {
    return VolumeLoadResult::makeFailure(QStringLiteral("Selected DICOM file does not exist"));
  }

  const QString selectedFilePath = normalizedPath(path);
  return loadSeriesFromDirectory(selectedFileInfo.absoluteDir().absolutePath(), &selectedFilePath);
}

VolumeLoadResult DicomVolumeLoader::loadSeriesDirectory(const QString& directoryPath) const
{
  return loadSeriesFromDirectory(directoryPath, nullptr);
}

} // namespace qvp
