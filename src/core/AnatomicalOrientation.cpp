#include "qtviewerpro/core/AnatomicalOrientation.h"

#include <array>

namespace
{

enum class AnatomicalAxisFamily
{
  LeftRight,
  AnteriorPosterior,
  SuperiorInferior
};

struct ParsedDirection
{
  qvp::AnatomicalDirection direction = qvp::AnatomicalDirection::Unknown;
  AnatomicalAxisFamily family = AnatomicalAxisFamily::LeftRight;
};

std::optional<ParsedDirection> parseDirection(char value)
{
  switch (value)
  {
  case 'L':
    return ParsedDirection{qvp::AnatomicalDirection::Left, AnatomicalAxisFamily::LeftRight};
  case 'R':
    return ParsedDirection{qvp::AnatomicalDirection::Right, AnatomicalAxisFamily::LeftRight};
  case 'A':
    return ParsedDirection{qvp::AnatomicalDirection::Anterior, AnatomicalAxisFamily::AnteriorPosterior};
  case 'P':
    return ParsedDirection{qvp::AnatomicalDirection::Posterior, AnatomicalAxisFamily::AnteriorPosterior};
  case 'S':
    return ParsedDirection{qvp::AnatomicalDirection::Superior, AnatomicalAxisFamily::SuperiorInferior};
  case 'I':
    return ParsedDirection{qvp::AnatomicalDirection::Inferior, AnatomicalAxisFamily::SuperiorInferior};
  default:
    return std::nullopt;
  }
}

} // namespace

namespace qvp
{

std::optional<VoxelAxisAnatomy>
parseAnatomicalOrientationAcronym(std::string_view acronym)
{
  if (acronym.size() != 3)
  {
    return std::nullopt;
  }

  std::array<AnatomicalAxisFamily, 3> families{};
  VoxelAxisAnatomy anatomy;

  const auto xDirection = parseDirection(acronym[0]);
  const auto yDirection = parseDirection(acronym[1]);
  const auto zDirection = parseDirection(acronym[2]);
  if (!xDirection.has_value() || !yDirection.has_value() || !zDirection.has_value())
  {
    return std::nullopt;
  }

  anatomy.x = xDirection->direction;
  anatomy.y = yDirection->direction;
  anatomy.z = zDirection->direction;
  families[0] = xDirection->family;
  families[1] = yDirection->family;
  families[2] = zDirection->family;

  if (families[0] == families[1] || families[0] == families[2] || families[1] == families[2])
  {
    return std::nullopt;
  }

  return anatomy;
}

} // namespace qvp
