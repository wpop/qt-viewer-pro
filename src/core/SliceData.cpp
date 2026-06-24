#include "qtviewerpro/core/SliceData.h"

#include <utility>

namespace qvp
{

SliceData::SliceData(std::size_t width,
                     std::size_t height,
                     float spacingX,
                     float spacingY,
                     SliceOrientation orientation,
                     std::size_t sliceIndex,
                     std::vector<float> pixels)
    : width_(width),
      height_(height),
      spacingX_(spacingX),
      spacingY_(spacingY),
      orientation_(orientation),
      sliceIndex_(sliceIndex),
      pixels_(std::move(pixels))
{
}

std::size_t SliceData::width() const
{
  return width_;
}

std::size_t SliceData::height() const
{
  return height_;
}

float SliceData::spacingX() const
{
  return spacingX_;
}

float SliceData::spacingY() const
{
  return spacingY_;
}

SliceOrientation SliceData::orientation() const
{
  return orientation_;
}

std::size_t SliceData::sliceIndex() const
{
  return sliceIndex_;
}

const std::vector<float>& SliceData::pixels() const
{
  return pixels_;
}

} // namespace qvp
