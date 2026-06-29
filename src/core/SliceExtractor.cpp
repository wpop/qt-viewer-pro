#include "qtviewerpro/core/SliceExtractor.h"

#include <stdexcept>
#include <utility>
#include <vector>

namespace qvp
{

namespace
{
} // namespace

SliceData SliceExtractor::extract(const VolumeData& volume,
                                  SliceOrientation orientation,
                                  std::size_t sliceIndex)
{
  if (!volume.isValid())
  {
    throw std::invalid_argument("Volume data is invalid");
  }

  const auto& voxels = volume.voxels();

  switch (orientation)
  {
  case SliceOrientation::Axial:
  {
    if (sliceIndex >= volume.depth())
    {
      throw std::out_of_range("Axial slice index is out of range");
    }

    const std::size_t sliceWidth = volume.width();
    const std::size_t sliceHeight = volume.height();
    std::vector<float> slice;
    slice.reserve(sliceWidth * sliceHeight);

    for (std::size_t y = 0; y < volume.height(); ++y)
    {
      for (std::size_t x = 0; x < volume.width(); ++x)
      {
        const std::size_t voxelIndex =
            (sliceIndex * volume.height() * volume.width()) + (y * volume.width()) + x;
        slice.push_back(voxels.at(voxelIndex));
      }
    }

    return SliceData(sliceWidth,
                     sliceHeight,
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

    const std::size_t sliceWidth = volume.width();
    const std::size_t sliceHeight = volume.depth();
    std::vector<float> slice;
    slice.reserve(sliceWidth * sliceHeight);

    for (std::size_t z = 0; z < volume.depth(); ++z)
    {
      for (std::size_t x = 0; x < volume.width(); ++x)
      {
        const std::size_t voxelIndex =
            (z * volume.height() * volume.width()) + (sliceIndex * volume.width()) + x;
        slice.push_back(voxels.at(voxelIndex));
      }
    }

    return SliceData(sliceWidth,
                     sliceHeight,
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

    const std::size_t sliceWidth = volume.height();
    const std::size_t sliceHeight = volume.depth();
    std::vector<float> slice;
    slice.reserve(sliceWidth * sliceHeight);

    for (std::size_t z = 0; z < volume.depth(); ++z)
    {
      for (std::size_t y = 0; y < volume.height(); ++y)
      {
        const std::size_t voxelIndex =
            (z * volume.height() * volume.width()) + (y * volume.width()) + sliceIndex;
        slice.push_back(voxels.at(voxelIndex));
      }
    }

    return SliceData(sliceWidth,
                     sliceHeight,
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
