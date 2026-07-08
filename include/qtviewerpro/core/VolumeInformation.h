#pragma once

#include "qtviewerpro/core/AnatomicalOrientation.h"
#include "qtviewerpro/core/VolumeData.h"

#include <cstddef>
#include <optional>

namespace qvp
{

struct VolumeInformation
{
  std::size_t width = 0;
  std::size_t height = 0;
  std::size_t depth = 0;

  double spacingX = 1.0;
  double spacingY = 1.0;
  double spacingZ = 1.0;

  std::size_t voxelCount = 0;
  std::size_t memoryBytes = 0;

  VolumeData::Origin origin{0.0, 0.0, 0.0};
  VolumeData::Direction direction{1.0, 0.0, 0.0,
                                  0.0, 1.0, 0.0,
                                  0.0, 0.0, 1.0};

  VolumeData::CoordinateSystem coordinateSystem = VolumeData::CoordinateSystem::Unknown;

  bool patientWorldOrientationTrusted = false;

  std::optional<VoxelAxisAnatomy> voxelAxisAnatomy;

  bool hasIntensityRange = false;
  float intensityMinimum = 0.0F;
  float intensityMaximum = 0.0F;
};

VolumeInformation makeVolumeInformation(const VolumeData& volume);

} // namespace qvp
