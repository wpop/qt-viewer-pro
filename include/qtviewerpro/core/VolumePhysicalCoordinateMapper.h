#pragma once

#include "qtviewerpro/core/VolumeData.h"

#include <cstddef>

namespace qvp
{

struct VoxelIndex3D
{
  std::size_t x = 0;
  std::size_t y = 0;
  std::size_t z = 0;
};

struct PhysicalPoint3D
{
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

class VolumePhysicalCoordinateMapper
{
public:
  static PhysicalPoint3D voxelToPhysical(const VolumeData& volume,
                                         const VoxelIndex3D& index);
};

} // namespace qvp
