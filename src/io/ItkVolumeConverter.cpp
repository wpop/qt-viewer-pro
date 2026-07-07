#include "qtviewerpro/io/ItkVolumeConverter.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

std::size_t checkedMultiply(std::size_t lhs, std::size_t rhs, const char* formatName)
{
  if (lhs != 0 && rhs > std::numeric_limits<std::size_t>::max() / lhs)
  {
    throw std::overflow_error(std::string(formatName) + " volume dimensions exceed size limits");
  }

  return lhs * rhs;
}

} // namespace

namespace qvp
{

VolumeData convertItkImageToVolume(const ItkVolumeImage::Pointer& image,
                                   const ItkSpatialGeometryPolicy& policy,
                                   const char* formatName)
{
  if (!image)
  {
    throw std::runtime_error(std::string(formatName) + " image is null");
  }

  const auto region = image->GetLargestPossibleRegion();
  const auto size = region.GetSize();

  const std::size_t width = static_cast<std::size_t>(size[0]);
  const std::size_t height = static_cast<std::size_t>(size[1]);
  const std::size_t depth = static_cast<std::size_t>(size[2]);
  if (width == 0 || height == 0 || depth == 0)
  {
    throw std::runtime_error(std::string(formatName) + " volume has zero dimensions");
  }

  const std::size_t voxelCount =
      checkedMultiply(checkedMultiply(width, height, formatName), depth, formatName);

  const auto spacing = image->GetSpacing();
  const float spacingX = static_cast<float>(spacing[0]);
  const float spacingY = static_cast<float>(spacing[1]);
  const float spacingZ = static_cast<float>(spacing[2]);

  const auto* buffer = image->GetBufferPointer();
  if (buffer == nullptr)
  {
    throw std::runtime_error(std::string(formatName) + " image buffer is null");
  }

  std::vector<float> voxels(voxelCount);
  std::copy(buffer, buffer + voxelCount, voxels.begin());

  const auto origin = image->GetOrigin();
  const auto direction = image->GetDirection();
  VolumeData::SpatialGeometry spatialGeometry;
  spatialGeometry.origin = {origin[0], origin[1], origin[2]};
  spatialGeometry.direction = {direction[0][0], direction[0][1], direction[0][2],
                               direction[1][0], direction[1][1], direction[1][2],
                               direction[2][0], direction[2][1], direction[2][2]};
  spatialGeometry.coordinateSystem = policy.coordinateSystem;
  spatialGeometry.hasOrientation = policy.hasTrustedOrientation;

  return VolumeData(width,
                    height,
                    depth,
                    spacingX,
                    spacingY,
                    spacingZ,
                    std::move(voxels),
                    spatialGeometry);
}

} // namespace qvp
