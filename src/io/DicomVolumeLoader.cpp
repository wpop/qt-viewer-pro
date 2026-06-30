#include "qtviewerpro/io/DicomVolumeLoader.h"

#include "qtviewerpro/core/VolumeData.h"

#include <QDir>
#include <QFileInfo>

#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImage.h>
#include <itkImageSeriesReader.h>

#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>
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

std::size_t checkedMultiply(std::size_t lhs, std::size_t rhs)
{
  if (lhs != 0 && rhs > std::numeric_limits<std::size_t>::max() / lhs)
  {
    throw std::overflow_error("DICOM volume dimensions exceed size limits");
  }

  return lhs * rhs;
}

qvp::VolumeData convertImageToVolume(const itk::Image<float, 3>::Pointer& image)
{
  const auto region = image->GetLargestPossibleRegion();
  const auto size = region.GetSize();

  const std::size_t width = static_cast<std::size_t>(size[0]);
  const std::size_t height = static_cast<std::size_t>(size[1]);
  const std::size_t depth = static_cast<std::size_t>(size[2]);
  if (width == 0 || height == 0 || depth == 0)
  {
    throw std::runtime_error("DICOM volume has zero dimensions");
  }

  const std::size_t voxelCount = checkedMultiply(checkedMultiply(width, height), depth);

  const auto spacing = image->GetSpacing();
  const float spacingX = static_cast<float>(spacing[0]);
  const float spacingY = static_cast<float>(spacing[1]);
  const float spacingZ = static_cast<float>(spacing[2]);

  const auto* buffer = image->GetBufferPointer();
  if (buffer == nullptr)
  {
    throw std::runtime_error("DICOM image buffer is null");
  }

  std::vector<float> voxels(voxelCount);
  std::copy(buffer, buffer + voxelCount, voxels.begin());

  return qvp::VolumeData(width, height, depth, spacingX, spacingY, spacingZ, std::move(voxels));
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

bool DicomVolumeLoader::canLoad(const QString& path) const
{
  return hasDicomExtension(path);
}

VolumeLoadResult DicomVolumeLoader::load(const QString& path) const
{
  using ImageType = itk::Image<float, 3>;
  using ImageIOType = itk::GDCMImageIO;
  using NamesGeneratorType = itk::GDCMSeriesFileNames;
  using ReaderType = itk::ImageSeriesReader<ImageType>;

  try
  {
    const QFileInfo selectedFileInfo(path);
    if (!selectedFileInfo.exists() || !selectedFileInfo.isFile())
    {
      return VolumeLoadResult::makeFailure(QStringLiteral("Selected DICOM file does not exist"));
    }

    const QString selectedFilePath = normalizedPath(path);
    const QString directoryPath = selectedFileInfo.absoluteDir().absolutePath();

    const auto namesGenerator = NamesGeneratorType::New();
    namesGenerator->SetUseSeriesDetails(true);
    namesGenerator->SetDirectory(directoryPath.toStdString());

    const auto& seriesIds = namesGenerator->GetSeriesUIDs();
    if (seriesIds.empty())
    {
      return VolumeLoadResult::makeFailure(QStringLiteral("No DICOM series found in directory"));
    }

    std::vector<std::string> fileNames;
    for (const auto& seriesId : seriesIds)
    {
      const auto seriesFileNames = namesGenerator->GetFileNames(seriesId);
      if (seriesContainsFile(seriesFileNames, selectedFilePath))
      {
        fileNames = seriesFileNames;
        break;
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

    const auto imageIO = ImageIOType::New();
    const auto reader = ReaderType::New();
    reader->SetImageIO(imageIO);
    reader->SetFileNames(fileNames);
    reader->Update();

    const VolumeData volume = convertImageToVolume(reader->GetOutput());
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

} // namespace qvp
