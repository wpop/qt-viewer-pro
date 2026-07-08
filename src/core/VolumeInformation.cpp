#include "qtviewerpro/core/VolumeInformation.h"

#include <limits>
#include <stdexcept>

namespace
{

std::size_t checkedMultiply(std::size_t lhs, std::size_t rhs, const char* errorMessage)
{
  if (lhs != 0 && rhs > std::numeric_limits<std::size_t>::max() / lhs)
  {
    throw std::overflow_error(errorMessage);
  }

  return lhs * rhs;
}

} // namespace

namespace qvp
{

VolumeInformation makeVolumeInformation(const VolumeData& volume)
{
  VolumeInformation information;
  information.width = volume.width();
  information.height = volume.height();
  information.depth = volume.depth();

  information.spacingX = volume.spacingX();
  information.spacingY = volume.spacingY();
  information.spacingZ = volume.spacingZ();

  information.voxelCount = volume.voxelCount();
  information.memoryBytes =
      checkedMultiply(information.voxelCount, sizeof(float), "Volume memory size exceeds limits");

  information.origin = volume.spatialGeometry().origin;
  information.direction = volume.spatialGeometry().direction;
  information.coordinateSystem = volume.spatialGeometry().coordinateSystem;
  information.patientWorldOrientationTrusted = volume.hasSpatialOrientation();
  information.voxelAxisAnatomy = volume.voxelAxisAnatomy();

  information.hasIntensityRange = volume.hasIntensityRange();
  if (information.hasIntensityRange)
  {
    information.intensityMinimum = volume.intensityMinimum();
    information.intensityMaximum = volume.intensityMaximum();
  }

  return information;
}

} // namespace qvp
