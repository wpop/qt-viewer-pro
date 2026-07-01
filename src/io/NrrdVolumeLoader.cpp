#include "qtviewerpro/io/NrrdVolumeLoader.h"

#include "qtviewerpro/core/VolumeData.h"

#include <QFileInfo>

#include <itkImage.h>
#include <itkImageFileReader.h>

#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{

bool hasNrrdExtension(const QString& path)
{
  const QString lowerFileName = QFileInfo(path).fileName().toLower();
  return lowerFileName.endsWith(QStringLiteral(".nrrd")) ||
         lowerFileName.endsWith(QStringLiteral(".nhdr"));
}

std::size_t checkedMultiply(std::size_t lhs, std::size_t rhs)
{
  if (lhs != 0 && rhs > std::numeric_limits<std::size_t>::max() / lhs)
  {
    throw std::overflow_error("NRRD volume dimensions exceed size limits");
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
    throw std::runtime_error("NRRD volume has zero dimensions");
  }

  const std::size_t voxelCount = checkedMultiply(checkedMultiply(width, height), depth);

  const auto spacing = image->GetSpacing();
  const float spacingX = static_cast<float>(spacing[0]);
  const float spacingY = static_cast<float>(spacing[1]);
  const float spacingZ = static_cast<float>(spacing[2]);

  const auto* buffer = image->GetBufferPointer();
  if (buffer == nullptr)
  {
    throw std::runtime_error("NRRD image buffer is null");
  }

  std::vector<float> voxels(voxelCount);
  std::copy(buffer, buffer + voxelCount, voxels.begin());

  const qvp::VolumeData volume(width, height, depth, spacingX, spacingY, spacingZ, std::move(voxels));
  if (!volume.isValid())
  {
    throw std::runtime_error("Loaded NRRD volume is invalid");
  }

  return volume;
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
  using ImageType = itk::Image<float, 3>;
  using ReaderType = itk::ImageFileReader<ImageType>;

  try
  {
    const auto reader = ReaderType::New();
    reader->SetFileName(path.toStdString());
    reader->Update();

    const auto image = reader->GetOutput();
    if (!image)
    {
      return VolumeLoadResult::makeFailure(QStringLiteral("NRRD image buffer is null"));
    }

    const VolumeData volume = convertImageToVolume(image);
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
