#pragma once

#include "qtviewerpro/core/SliceOrientation.h"
#include "qtviewerpro/core/VolumeData.h"

#include <optional>
#include <string_view>

namespace qvp
{

struct OrientationEdgeLabels
{
  std::string_view left;
  std::string_view right;
  std::string_view top;
  std::string_view bottom;
};

class MprOrientationLabelMapper
{
public:
  static std::optional<OrientationEdgeLabels> edgeLabels(
      const VolumeData::SpatialGeometry& geometry,
      SliceOrientation orientation);
};

} // namespace qvp
