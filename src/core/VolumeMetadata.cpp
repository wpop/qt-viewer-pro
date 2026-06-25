#include "qtviewerpro/core/VolumeMetadata.h"

namespace qvp
{

VolumeMetadata::VolumeMetadata(std::size_t width, std::size_t height, std::size_t depth,
                               float spacingX, float spacingY, float spacingZ)
    : width_(width), height_(height), depth_(depth), spacingX_(spacingX), spacingY_(spacingY),
      spacingZ_(spacingZ)
{
}

std::size_t VolumeMetadata::width() const
{
  return width_;
}

std::size_t VolumeMetadata::height() const
{
  return height_;
}

std::size_t VolumeMetadata::depth() const
{
  return depth_;
}

float VolumeMetadata::spacingX() const
{
  return spacingX_;
}

float VolumeMetadata::spacingY() const
{
  return spacingY_;
}

float VolumeMetadata::spacingZ() const
{
  return spacingZ_;
}

} // namespace qvp
