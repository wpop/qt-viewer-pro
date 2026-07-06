#include "qtviewerpro/core/VolumeData.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace qvp
{

VolumeData::VolumeData(std::size_t width, std::size_t height, std::size_t depth, float spacingX,
                       float spacingY, float spacingZ, std::vector<float> voxels)
    : width_(width), height_(height), depth_(depth), spacingX_(spacingX), spacingY_(spacingY),
      spacingZ_(spacingZ), voxels_(std::move(voxels))
{
  if (!voxels_.empty())
  {
    const auto [minIt, maxIt] = std::minmax_element(voxels_.begin(), voxels_.end());
    intensityMinimum_ = *minIt;
    intensityMaximum_ = *maxIt;
    hasIntensityRange_ = true;
  }
}

std::size_t VolumeData::width() const
{
  return width_;
}

std::size_t VolumeData::height() const
{
  return height_;
}

std::size_t VolumeData::depth() const
{
  return depth_;
}

float VolumeData::spacingX() const
{
  return spacingX_;
}

float VolumeData::spacingY() const
{
  return spacingY_;
}

float VolumeData::spacingZ() const
{
  return spacingZ_;
}

const std::vector<float>& VolumeData::voxels() const
{
  return voxels_;
}

bool VolumeData::hasIntensityRange() const
{
  return hasIntensityRange_;
}

float VolumeData::intensityMinimum() const
{
  return intensityMinimum_;
}

float VolumeData::intensityMaximum() const
{
  return intensityMaximum_;
}

bool VolumeData::isEmpty() const
{
  return width_ == 0 || height_ == 0 || depth_ == 0 || voxels_.empty();
}

bool VolumeData::isValid() const
{
  if (isEmpty())
  {
    return false;
  }

  try
  {
    return voxelCount() == voxels_.size();
  }
  catch (const std::overflow_error&)
  {
    return false;
  }
}

std::size_t VolumeData::voxelCount() const
{
  if (width_ == 0 || height_ == 0 || depth_ == 0)
  {
    return 0;
  }

  if (height_ > std::numeric_limits<std::size_t>::max() / width_)
  {
    throw std::overflow_error("Volume width and height exceed size limits");
  }

  const std::size_t planeVoxelCount = width_ * height_;
  if (depth_ > std::numeric_limits<std::size_t>::max() / planeVoxelCount)
  {
    throw std::overflow_error("Volume voxel count exceeds size limits");
  }

  return planeVoxelCount * depth_;
}

} // namespace qvp
