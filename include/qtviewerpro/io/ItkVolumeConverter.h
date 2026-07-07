#pragma once

#include "qtviewerpro/core/VolumeData.h"

#include <itkImage.h>

namespace qvp
{

struct ItkSpatialGeometryPolicy
{
  bool hasTrustedOrientation = false;
  VolumeData::CoordinateSystem coordinateSystem = VolumeData::CoordinateSystem::LPS;
};

using ItkVolumeImage = itk::Image<float, 3>;

VolumeData convertItkImageToVolume(const ItkVolumeImage::Pointer& image,
                                   const ItkSpatialGeometryPolicy& policy,
                                   const char* formatName);

} // namespace qvp
