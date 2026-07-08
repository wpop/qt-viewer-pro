#pragma once

#include <optional>
#include <string_view>

namespace qvp
{

enum class AnatomicalDirection
{
  Unknown,
  Left,
  Right,
  Anterior,
  Posterior,
  Superior,
  Inferior
};

struct VoxelAxisAnatomy
{
  AnatomicalDirection x = AnatomicalDirection::Unknown;
  AnatomicalDirection y = AnatomicalDirection::Unknown;
  AnatomicalDirection z = AnatomicalDirection::Unknown;
};

std::optional<VoxelAxisAnatomy>
parseAnatomicalOrientationAcronym(std::string_view acronym);

} // namespace qvp
