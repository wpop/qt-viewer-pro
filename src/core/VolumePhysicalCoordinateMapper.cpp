#include "qtviewerpro/core/VolumePhysicalCoordinateMapper.h"

namespace qvp
{

PhysicalPoint3D VolumePhysicalCoordinateMapper::voxelToPhysical(const VolumeData& volume,
                                                                const VoxelIndex3D& index)
{
  const auto& geometry = volume.spatialGeometry();
  const auto& direction = geometry.direction;
  const auto& origin = geometry.origin;

  const double scaledX = static_cast<double>(index.x) * static_cast<double>(volume.spacingX());
  const double scaledY = static_cast<double>(index.y) * static_cast<double>(volume.spacingY());
  const double scaledZ = static_cast<double>(index.z) * static_cast<double>(volume.spacingZ());

  return PhysicalPoint3D{
      origin[0] + (direction[0] * scaledX) + (direction[1] * scaledY) + (direction[2] * scaledZ),
      origin[1] + (direction[3] * scaledX) + (direction[4] * scaledY) + (direction[5] * scaledZ),
      origin[2] + (direction[6] * scaledX) + (direction[7] * scaledY) + (direction[8] * scaledZ)};
}

} // namespace qvp
