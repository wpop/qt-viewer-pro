#include "qtviewerpro/core/SliceExtractor.h"

#include <stdexcept>
#include <utility>
#include <vector>

namespace qvp
{

namespace
{

std::size_t voxelIndex(const VolumeData& volume, std::size_t x, std::size_t y, std::size_t z)
{
  return (z * volume.height() * volume.width()) + (y * volume.width()) + x;
}

} // namespace

SliceData SliceExtractor::extract(const VolumeData& volume,
                                  SliceOrientation orientation,
                                  std::size_t sliceIndex)
{
  const auto& voxels = volume.voxels();

  switch (orientation)
  {
  case SliceOrientation::Axial:
  {
    if (sliceIndex >= volume.depth())
    {
      throw std::out_of_range("Axial slice index is out of range");
    }

    std::vector<float> slice;
    slice.reserve(volume.width() * volume.height());

    for (std::size_t y = 0; y < volume.height(); ++y)
    {
      for (std::size_t x = 0; x < volume.width(); ++x)
      {
        slice.push_back(voxels.at(voxelIndex(volume, x, y, sliceIndex)));
      }
    }

    return SliceData(volume.width(),
                     volume.height(),
                     volume.spacingX(),
                     volume.spacingY(),
                     orientation,
                     sliceIndex,
                     std::move(slice));
  }
  case SliceOrientation::Coronal:
  {
    if (sliceIndex >= volume.height())
    {
      throw std::out_of_range("Coronal slice index is out of range");
    }

    std::vector<float> slice;
    slice.reserve(volume.width() * volume.depth());

    for (std::size_t z = 0; z < volume.depth(); ++z)
    {
      for (std::size_t x = 0; x < volume.width(); ++x)
      {
        slice.push_back(voxels.at(voxelIndex(volume, x, sliceIndex, z)));
      }
    }

    return SliceData(volume.width(),
                     volume.depth(),
                     volume.spacingX(),
                     volume.spacingZ(),
                     orientation,
                     sliceIndex,
                     std::move(slice));
  }
  case SliceOrientation::Sagittal:
  {
    if (sliceIndex >= volume.width())
    {
      throw std::out_of_range("Sagittal slice index is out of range");
    }

    std::vector<float> slice;
    slice.reserve(volume.height() * volume.depth());

    for (std::size_t z = 0; z < volume.depth(); ++z)
    {
      for (std::size_t y = 0; y < volume.height(); ++y)
      {
        slice.push_back(voxels.at(voxelIndex(volume, sliceIndex, y, z)));
      }
    }

    return SliceData(volume.height(),
                     volume.depth(),
                     volume.spacingY(),
                     volume.spacingZ(),
                     orientation,
                     sliceIndex,
                     std::move(slice));
  }
  }

  throw std::out_of_range("Unknown slice orientation");
}

} // namespace qvp
