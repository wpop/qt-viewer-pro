#include "qtviewerpro/core/AnatomicalOrientation.h"

#include <array>
#include <string>

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

std::optional<AnatomicalAxisFamily> anatomicalAxisFamily(qvp::AnatomicalDirection direction)
{
  switch (direction)
  {
  case qvp::AnatomicalDirection::Left:
  case qvp::AnatomicalDirection::Right:
    return AnatomicalAxisFamily::LeftRight;
  case qvp::AnatomicalDirection::Anterior:
  case qvp::AnatomicalDirection::Posterior:
    return AnatomicalAxisFamily::AnteriorPosterior;
  case qvp::AnatomicalDirection::Superior:
  case qvp::AnatomicalDirection::Inferior:
    return AnatomicalAxisFamily::SuperiorInferior;
  case qvp::AnatomicalDirection::Unknown:
    return std::nullopt;
  }

  return std::nullopt;
}

std::optional<char> anatomicalDirectionLetter(qvp::AnatomicalDirection direction)
{
  switch (direction)
  {
  case qvp::AnatomicalDirection::Left:
    return 'L';
  case qvp::AnatomicalDirection::Right:
    return 'R';
  case qvp::AnatomicalDirection::Anterior:
    return 'A';
  case qvp::AnatomicalDirection::Posterior:
    return 'P';
  case qvp::AnatomicalDirection::Superior:
    return 'S';
  case qvp::AnatomicalDirection::Inferior:
    return 'I';
  case qvp::AnatomicalDirection::Unknown:
    return std::nullopt;
  }

  return std::nullopt;
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

std::optional<std::string>
anatomicalOrientationAcronym(const VoxelAxisAnatomy& anatomy)
{
  const auto xFamily = anatomicalAxisFamily(anatomy.x);
  const auto yFamily = anatomicalAxisFamily(anatomy.y);
  const auto zFamily = anatomicalAxisFamily(anatomy.z);
  if (!xFamily.has_value() || !yFamily.has_value() || !zFamily.has_value())
  {
    return std::nullopt;
  }

  if (xFamily == yFamily || xFamily == zFamily || yFamily == zFamily)
  {
    return std::nullopt;
  }

  const auto xLetter = anatomicalDirectionLetter(anatomy.x);
  const auto yLetter = anatomicalDirectionLetter(anatomy.y);
  const auto zLetter = anatomicalDirectionLetter(anatomy.z);
  if (!xLetter.has_value() || !yLetter.has_value() || !zLetter.has_value())
  {
    return std::nullopt;
  }

  std::string acronym;
  acronym.reserve(3);
  acronym.push_back(*xLetter);
  acronym.push_back(*yLetter);
  acronym.push_back(*zLetter);
  return acronym;
}

} // namespace qvp
