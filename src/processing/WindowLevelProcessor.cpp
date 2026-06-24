#include "qtviewerpro/processing/WindowLevelProcessor.h"

#include <cmath>
#include <stdexcept>

namespace qvp
{

std::vector<std::uint8_t> WindowLevelProcessor::apply(const std::vector<float>& pixels,
                                                      float window,
                                                      float level)
{
  if (window <= 0.0F)
  {
    throw std::invalid_argument("Window must be positive");
  }

  const float lower = level - (window / 2.0F);
  const float upper = level + (window / 2.0F);

  std::vector<std::uint8_t> output;
  output.reserve(pixels.size());

  for (const float value : pixels)
  {
    if (value <= lower)
    {
      output.push_back(0);
    }
    else if (value >= upper)
    {
      output.push_back(255);
    }
    else
    {
      const float normalized = (value - lower) / (upper - lower);
      output.push_back(static_cast<std::uint8_t>(std::lround(normalized * 255.0F)));
    }
  }

  return output;
}

} // namespace qvp
