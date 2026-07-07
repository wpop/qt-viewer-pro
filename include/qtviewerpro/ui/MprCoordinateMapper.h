#pragma once

#include "qtviewerpro/core/SliceOrientation.h"
#include "qtviewerpro/core/VolumeData.h"

#include <cstddef>

namespace qvp
{

struct MprVoxelPosition
{
  std::size_t x = 0;
  std::size_t y = 0;
  std::size_t z = 0;
};

struct MprImagePoint
{
  std::size_t x = 0;
  std::size_t y = 0;
};

class MprCoordinateMapper
{
public:
  static MprImagePoint crosshairImagePoint(SliceOrientation orientation,
                                           const MprVoxelPosition& position);

  static MprVoxelPosition voxelPositionFromImagePoint(const VolumeData& volume,
                                                      SliceOrientation orientation,
                                                      std::size_t imageX,
                                                      std::size_t imageY,
                                                      const MprVoxelPosition& currentPosition);
};

} // namespace qvp
