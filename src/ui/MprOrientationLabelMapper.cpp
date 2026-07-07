#include "qtviewerpro/ui/MprOrientationLabelMapper.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace
{

constexpr double kMinimumDominantComponent = 0.75;
constexpr double kMinimumDominanceGap = 0.10;
constexpr double kMinimumVectorNorm = 1e-6;

enum class AnatomicalAxis
{
  X,
  Y,
  Z
};

struct ClassifiedDirection
{
  AnatomicalAxis axis;
  bool positive = false;
};

using Vector3 = std::array<double, 3>;

std::optional<ClassifiedDirection> classifyDirection(
    const Vector3& direction)
{
  for (const double component : direction)
  {
    if (!std::isfinite(component))
    {
      return std::nullopt;
    }
  }

  const double norm =
      std::sqrt((direction[0] * direction[0]) + (direction[1] * direction[1]) +
                (direction[2] * direction[2]));
  if (norm < kMinimumVectorNorm)
  {
    return std::nullopt;
  }

  const Vector3 normalized{
      direction[0] / norm, direction[1] / norm, direction[2] / norm};
  const Vector3 absoluteValues{
      std::fabs(normalized[0]), std::fabs(normalized[1]), std::fabs(normalized[2])};

  std::size_t dominantIndex = 0;
  if (absoluteValues[1] > absoluteValues[dominantIndex])
  {
    dominantIndex = 1;
  }
  if (absoluteValues[2] > absoluteValues[dominantIndex])
  {
    dominantIndex = 2;
  }

  double secondLargest = 0.0;
  for (std::size_t i = 0; i < absoluteValues.size(); ++i)
  {
    if (i == dominantIndex)
    {
      continue;
    }
    secondLargest = std::max(secondLargest, absoluteValues[i]);
  }

  const double dominantValue = absoluteValues[dominantIndex];
  if (dominantValue < kMinimumDominantComponent ||
      (dominantValue - secondLargest) < kMinimumDominanceGap)
  {
    return std::nullopt;
  }

  AnatomicalAxis axis = AnatomicalAxis::X;
  switch (dominantIndex)
  {
  case 0:
    axis = AnatomicalAxis::X;
    break;
  case 1:
    axis = AnatomicalAxis::Y;
    break;
  case 2:
    axis = AnatomicalAxis::Z;
    break;
  default:
    return std::nullopt;
  }

  return ClassifiedDirection{axis, normalized[dominantIndex] > 0.0};
}

std::optional<std::string_view> positiveDirectionLabel(
    const ClassifiedDirection& direction,
    qvp::VolumeData::CoordinateSystem coordinateSystem)
{
  switch (coordinateSystem)
  {
  case qvp::VolumeData::CoordinateSystem::LPS:
    switch (direction.axis)
    {
    case AnatomicalAxis::X:
      return direction.positive ? std::optional<std::string_view>{"L"}
                                : std::optional<std::string_view>{"R"};
    case AnatomicalAxis::Y:
      return direction.positive ? std::optional<std::string_view>{"P"}
                                : std::optional<std::string_view>{"A"};
    case AnatomicalAxis::Z:
      return direction.positive ? std::optional<std::string_view>{"S"}
                                : std::optional<std::string_view>{"I"};
    }
    break;
  case qvp::VolumeData::CoordinateSystem::RAS:
    switch (direction.axis)
    {
    case AnatomicalAxis::X:
      return direction.positive ? std::optional<std::string_view>{"R"}
                                : std::optional<std::string_view>{"L"};
    case AnatomicalAxis::Y:
      return direction.positive ? std::optional<std::string_view>{"A"}
                                : std::optional<std::string_view>{"P"};
    case AnatomicalAxis::Z:
      return direction.positive ? std::optional<std::string_view>{"S"}
                                : std::optional<std::string_view>{"I"};
    }
    break;
  case qvp::VolumeData::CoordinateSystem::Unknown:
    return std::nullopt;
  }

  return std::nullopt;
}

std::optional<std::string_view> oppositeLabel(std::string_view label)
{
  if (label == "L")
  {
    return "R";
  }
  if (label == "R")
  {
    return "L";
  }
  if (label == "A")
  {
    return "P";
  }
  if (label == "P")
  {
    return "A";
  }
  if (label == "S")
  {
    return "I";
  }
  if (label == "I")
  {
    return "S";
  }

  return std::nullopt;
}

Vector3 voxelXAxis(const qvp::VolumeData::Direction& direction)
{
  return Vector3{direction[0], direction[3], direction[6]};
}

Vector3 voxelYAxis(const qvp::VolumeData::Direction& direction)
{
  return Vector3{direction[1], direction[4], direction[7]};
}

Vector3 voxelZAxis(const qvp::VolumeData::Direction& direction)
{
  return Vector3{direction[2], direction[5], direction[8]};
}

struct DisplayAxes
{
  Vector3 horizontal;
  Vector3 vertical;
};

std::optional<DisplayAxes> displayAxes(const qvp::VolumeData::Direction& direction,
                                       qvp::SliceOrientation orientation)
{
  switch (orientation)
  {
  case qvp::SliceOrientation::Axial:
    return DisplayAxes{voxelXAxis(direction), voxelYAxis(direction)};
  case qvp::SliceOrientation::Sagittal:
    return DisplayAxes{voxelYAxis(direction), voxelZAxis(direction)};
  case qvp::SliceOrientation::Coronal:
    return DisplayAxes{voxelXAxis(direction), voxelZAxis(direction)};
  }

  return std::nullopt;
}

} // namespace

namespace qvp
{

std::optional<OrientationEdgeLabels> MprOrientationLabelMapper::edgeLabels(
    const VolumeData::SpatialGeometry& geometry,
    SliceOrientation orientation)
{
  if (!geometry.hasOrientation ||
      geometry.coordinateSystem == VolumeData::CoordinateSystem::Unknown)
  {
    return std::nullopt;
  }

  const auto axes = displayAxes(geometry.direction, orientation);
  if (!axes.has_value())
  {
    return std::nullopt;
  }

  const auto horizontal =
      classifyDirection(axes->horizontal);
  const auto vertical =
      classifyDirection(axes->vertical);
  if (!horizontal.has_value() || !vertical.has_value())
  {
    return std::nullopt;
  }

  if (horizontal->axis == vertical->axis)
  {
    return std::nullopt;
  }

  const auto right = positiveDirectionLabel(*horizontal, geometry.coordinateSystem);
  const auto bottom = positiveDirectionLabel(*vertical, geometry.coordinateSystem);
  if (!right.has_value() || !bottom.has_value())
  {
    return std::nullopt;
  }

  const auto left = oppositeLabel(*right);
  const auto top = oppositeLabel(*bottom);
  if (!left.has_value() || !top.has_value())
  {
    return std::nullopt;
  }

  return OrientationEdgeLabels{*left, *right, *top, *bottom};
}

} // namespace qvp
