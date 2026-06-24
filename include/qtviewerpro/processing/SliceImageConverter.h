#pragma once

#include "qtviewerpro/core/SliceData.h"

#include <QImage>

namespace qvp
{

/**
 * @brief Converts slice data to grayscale Qt images.
 *
 * SliceImageConverter applies window/level processing and creates a grayscale
 * image without depending on UI widgets.
 */
class SliceImageConverter
{
public:
  /**
   * @brief Converts a slice to an 8-bit grayscale image.
   * @param slice Source slice data.
   * @param window Width of the intensity window. Must be positive.
   * @param level Center of the intensity window.
   * @return Grayscale image with dimensions matching the slice.
   * @throws std::invalid_argument If the slice pixel count is invalid or the window is not positive.
   */
  static QImage toGrayscaleImage(const SliceData& slice, float window, float level);
};

} // namespace qvp
