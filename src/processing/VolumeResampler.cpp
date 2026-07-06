#include "qtviewerpro/processing/VolumeResampler.h"

#include <itkImage.h>
#include <itkIdentityTransform.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkResampleImageFilter.h>

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
using ImageType = itk::Image<float, 3>;

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

  ImageType::PointType origin;
  origin.Fill(0.0);

  ImageType::DirectionType direction;
  direction.SetIdentity();

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

qvp::VolumeData itkImageToVolume(const ImageType::Pointer& image)
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

  const auto* buffer = image->GetBufferPointer();
  if (buffer == nullptr)
  {
    throw std::runtime_error("Resampled image buffer is null");
  }

  std::vector<float> voxels(voxelCount);
  std::copy(buffer, buffer + voxelCount, voxels.begin());

  return qvp::VolumeData(width, height, depth, spacingX, spacingY, spacingZ, std::move(voxels));
}

} // namespace

namespace qvp
{

VolumeData VolumeResampler::resampleToIsotropicSpacing(const VolumeData& volume)
{
  const auto inputImage = volumeToItkImage(volume);

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

  resampleFilter->SetInput(inputImage);
  resampleFilter->SetTransform(transform);
  resampleFilter->SetInterpolator(interpolator);
  resampleFilter->SetSize(outputSize);
  resampleFilter->SetOutputSpacing(outputSpacing);
  resampleFilter->SetOutputOrigin(outputOrigin);
  resampleFilter->SetOutputDirection(outputDirection);
  resampleFilter->SetDefaultPixelValue(0.0F);
  resampleFilter->UpdateLargestPossibleRegion();

  return itkImageToVolume(resampleFilter->GetOutput());
}

} // namespace qvp
