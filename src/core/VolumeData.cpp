#include "qtviewerpro/core/VolumeData.h"

#include <utility>

namespace qvp
{

VolumeData::VolumeData(std::size_t width,
                       std::size_t height,
                       std::size_t depth,
                       float spacingX,
                       float spacingY,
                       float spacingZ,
                       std::vector<float> voxels)
    : width_(width),
      height_(height),
      depth_(depth),
      spacingX_(spacingX),
      spacingY_(spacingY),
      spacingZ_(spacingZ),
      voxels_(std::move(voxels))
{
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

} // namespace qvp
