#pragma once

#include <cstdint>
#include <vector>

namespace qvp
{

/**
 * @brief Converts floating-point intensity values to 8-bit grayscale values.
 *
 * WindowLevelProcessor applies standard window/level mapping without depending
 * on UI or image classes.
 */
class WindowLevelProcessor
{
public:
  /**
   * @brief Applies window/level mapping to a pixel buffer.
   * @param pixels Source floating-point intensity values.
   * @param window Width of the intensity window. Must be positive.
   * @param level Center of the intensity window.
   * @return 8-bit grayscale values in the range 0 to 255.
   * @throws std::invalid_argument If window is not positive.
   */
  static std::vector<std::uint8_t> apply(const std::vector<float>& pixels, float window, float level);
};

} // namespace qvp
