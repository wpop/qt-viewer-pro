#include "qtviewerpro/ui/MprCoordinateMapper.h"

#include <algorithm>
#include <stdexcept>

namespace
{

std::size_t clampCoordinate(std::size_t value, std::size_t count)
{
  if (count == 0)
  {
    return 0;
  }

  return std::min(value, count - 1);
}

} // namespace

namespace qvp
{

MprImagePoint MprCoordinateMapper::crosshairImagePoint(SliceOrientation orientation,
                                                       const MprVoxelPosition& position)
{
  switch (orientation)
  {
  case SliceOrientation::Axial:
    return MprImagePoint{position.x, position.y};
  case SliceOrientation::Sagittal:
    return MprImagePoint{position.y, position.z};
  case SliceOrientation::Coronal:
    return MprImagePoint{position.x, position.z};
  }

  throw std::invalid_argument("Unknown MPR orientation");
}

MprVoxelPosition MprCoordinateMapper::voxelPositionFromImagePoint(const VolumeData& volume,
                                                                  SliceOrientation orientation,
                                                                  std::size_t imageX,
                                                                  std::size_t imageY,
                                                                  const MprVoxelPosition& currentPosition)
{
  if (!volume.isValid())
  {
    throw std::invalid_argument("Volume data is invalid");
  }

  MprVoxelPosition position = currentPosition;

  switch (orientation)
  {
  case SliceOrientation::Axial:
    position.x = clampCoordinate(imageX, volume.width());
    position.y = clampCoordinate(imageY, volume.height());
    return position;
  case SliceOrientation::Sagittal:
    position.y = clampCoordinate(imageX, volume.height());
    position.z = clampCoordinate(imageY, volume.depth());
    return position;
  case SliceOrientation::Coronal:
    position.x = clampCoordinate(imageX, volume.width());
    position.z = clampCoordinate(imageY, volume.depth());
    return position;
  }

  throw std::invalid_argument("Unknown MPR orientation");
}

} // namespace qvp
