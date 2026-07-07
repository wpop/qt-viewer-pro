#include "qtviewerpro/processing/VolumeResampler.h"

#include <itkImage.h>
#include <itkIdentityTransform.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkResampleImageFilter.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <QDebug>
#include <QString>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
using ImageType = itk::Image<float, 3>;
using Clock = std::chrono::steady_clock;

double durationMilliseconds(const Clock::duration& duration)
{
  return std::chrono::duration<double, std::milli>(duration).count();
}

std::size_t checkedMultiply(std::size_t lhs, std::size_t rhs)
{
  if (lhs != 0 && rhs > std::numeric_limits<std::size_t>::max() / lhs)
  {
    throw std::overflow_error("Volume dimensions exceed size limits");
  }

  return lhs * rhs;
}

std::size_t resampledDimension(std::size_t inputSize, float inputSpacing, float targetSpacing)
{
  if (inputSize == 0)
  {
    throw std::invalid_argument("Volume dimensions must be positive");
  }

  if (!std::isfinite(inputSpacing) || inputSpacing <= 0.0F || !std::isfinite(targetSpacing) ||
      targetSpacing <= 0.0F)
  {
    throw std::invalid_argument("Volume spacing must be positive and finite");
  }

  const double physicalExtent =
      (static_cast<double>(inputSize) - 1.0) * static_cast<double>(inputSpacing);
  const double outputExtent = physicalExtent / static_cast<double>(targetSpacing);
  const auto outputSize = static_cast<long long>(std::llround(outputExtent)) + 1LL;

  return std::max<std::size_t>(1, static_cast<std::size_t>(outputSize));
}

ImageType::Pointer volumeToItkImage(const qvp::VolumeData& volume)
{
  if (!volume.isValid())
  {
    throw std::invalid_argument("Volume data is invalid");
  }

  const std::size_t width = volume.width();
  const std::size_t height = volume.height();
  const std::size_t depth = volume.depth();
  const std::size_t voxelCount = checkedMultiply(checkedMultiply(width, height), depth);

  auto image = ImageType::New();

  ImageType::RegionType region;
  ImageType::IndexType start;
  start.Fill(0);
  ImageType::SizeType size;
  size[0] = static_cast<ImageType::SizeType::SizeValueType>(width);
  size[1] = static_cast<ImageType::SizeType::SizeValueType>(height);
  size[2] = static_cast<ImageType::SizeType::SizeValueType>(depth);
  region.SetIndex(start);
  region.SetSize(size);

  ImageType::SpacingType spacing;
  spacing[0] = volume.spacingX();
  spacing[1] = volume.spacingY();
  spacing[2] = volume.spacingZ();

  const auto& sourceGeometry = volume.spatialGeometry();
  ImageType::PointType origin;
  origin[0] = sourceGeometry.origin[0];
  origin[1] = sourceGeometry.origin[1];
  origin[2] = sourceGeometry.origin[2];

  ImageType::DirectionType direction;
  direction[0][0] = sourceGeometry.direction[0];
  direction[0][1] = sourceGeometry.direction[1];
  direction[0][2] = sourceGeometry.direction[2];
  direction[1][0] = sourceGeometry.direction[3];
  direction[1][1] = sourceGeometry.direction[4];
  direction[1][2] = sourceGeometry.direction[5];
  direction[2][0] = sourceGeometry.direction[6];
  direction[2][1] = sourceGeometry.direction[7];
  direction[2][2] = sourceGeometry.direction[8];

  image->SetRegions(region);
  image->SetSpacing(spacing);
  image->SetOrigin(origin);
  image->SetDirection(direction);
  image->Allocate();

  const auto& voxels = volume.voxels();
  if (voxels.size() != voxelCount)
  {
    throw std::invalid_argument("Volume voxel count does not match dimensions");
  }

  std::copy(voxels.begin(), voxels.end(), image->GetBufferPointer());
  return image;
}

qvp::VolumeData itkImageToVolume(const ImageType::Pointer& image,
                                 const qvp::VolumeData::SpatialGeometry& sourceGeometry)
{
  if (!image)
  {
    throw std::runtime_error("Resampled image is null");
  }

  const auto region = image->GetLargestPossibleRegion();
  const auto size = region.GetSize();

  const std::size_t width = static_cast<std::size_t>(size[0]);
  const std::size_t height = static_cast<std::size_t>(size[1]);
  const std::size_t depth = static_cast<std::size_t>(size[2]);
  if (width == 0 || height == 0 || depth == 0)
  {
    throw std::runtime_error("Resampled volume has zero dimensions");
  }

  const std::size_t voxelCount = checkedMultiply(checkedMultiply(width, height), depth);

  const auto spacing = image->GetSpacing();
  const float spacingX = static_cast<float>(spacing[0]);
  const float spacingY = static_cast<float>(spacing[1]);
  const float spacingZ = static_cast<float>(spacing[2]);
  const auto origin = image->GetOrigin();
  const auto direction = image->GetDirection();

  const auto* buffer = image->GetBufferPointer();
  if (buffer == nullptr)
  {
    throw std::runtime_error("Resampled image buffer is null");
  }

  std::vector<float> voxels(voxelCount);
  std::copy(buffer, buffer + voxelCount, voxels.begin());

  qvp::VolumeData::SpatialGeometry outputGeometry = sourceGeometry;
  outputGeometry.origin = {origin[0], origin[1], origin[2]};
  outputGeometry.direction = {direction[0][0], direction[0][1], direction[0][2],
                              direction[1][0], direction[1][1], direction[1][2],
                              direction[2][0], direction[2][1], direction[2][2]};

  return qvp::VolumeData(
      width, height, depth, spacingX, spacingY, spacingZ, std::move(voxels), outputGeometry);
}

} // namespace

namespace qvp
{

VolumeData VolumeResampler::resampleToIsotropicSpacing(const VolumeData& volume)
{
  const auto totalStart = Clock::now();

  const auto conversionStart = Clock::now();
  const auto inputImage = volumeToItkImage(volume);
  const auto conversionEnd = Clock::now();

  constexpr float kTargetSpacing = 1.0F;

  const auto inputRegion = inputImage->GetLargestPossibleRegion();
  const auto inputSize = inputRegion.GetSize();

  ImageType::SizeType outputSize;
  outputSize[0] = static_cast<std::size_t>(
      resampledDimension(static_cast<std::size_t>(inputSize[0]), volume.spacingX(), kTargetSpacing));
  outputSize[1] = static_cast<std::size_t>(
      resampledDimension(static_cast<std::size_t>(inputSize[1]), volume.spacingY(), kTargetSpacing));
  outputSize[2] = static_cast<std::size_t>(
      resampledDimension(static_cast<std::size_t>(inputSize[2]), volume.spacingZ(), kTargetSpacing));

  ImageType::SpacingType outputSpacing;
  outputSpacing.Fill(kTargetSpacing);

  const auto outputOrigin = inputImage->GetOrigin();
  const auto outputDirection = inputImage->GetDirection();

  using TransformType = itk::IdentityTransform<double, 3>;
  using InterpolatorType = itk::LinearInterpolateImageFunction<ImageType, double>;
  using ResampleFilterType = itk::ResampleImageFilter<ImageType, ImageType>;

  const auto transform = TransformType::New();
  const auto interpolator = InterpolatorType::New();
  const auto resampleFilter = ResampleFilterType::New();

  const auto resampleStart = Clock::now();
  resampleFilter->SetInput(inputImage);
  resampleFilter->SetTransform(transform);
  resampleFilter->SetInterpolator(interpolator);
  resampleFilter->SetSize(outputSize);
  resampleFilter->SetOutputSpacing(outputSpacing);
  resampleFilter->SetOutputOrigin(outputOrigin);
  resampleFilter->SetOutputDirection(outputDirection);
  resampleFilter->SetDefaultPixelValue(0.0F);
  resampleFilter->UpdateLargestPossibleRegion();
  const auto resampleEnd = Clock::now();

  const auto reconversionStart = Clock::now();
  const VolumeData outputVolume =
      itkImageToVolume(resampleFilter->GetOutput(), volume.spatialGeometry());
  const auto reconversionEnd = Clock::now();

  const auto totalEnd = Clock::now();

  qDebug().noquote()
      << QStringLiteral("Volume resampling timings:\n"
                        "  VolumeData -> ITK: %1 ms\n"
                        "  ITK resample:      %2 ms\n"
                        "  ITK -> VolumeData: %3 ms\n"
                        "  Total:             %4 ms")
             .arg(QString::number(durationMilliseconds(conversionEnd - conversionStart), 'f', 1))
             .arg(QString::number(durationMilliseconds(resampleEnd - resampleStart), 'f', 1))
             .arg(QString::number(durationMilliseconds(reconversionEnd - reconversionStart), 'f', 1))
             .arg(QString::number(durationMilliseconds(totalEnd - totalStart), 'f', 1));

  return outputVolume;
}

} // namespace qvp
