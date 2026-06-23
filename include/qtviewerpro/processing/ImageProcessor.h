#pragma once

#include <QImage>

/**
 * @brief Performs image-processing operations on QImage instances.
 *
 * ImageProcessor isolates processing logic from UI and IO responsibilities,
 * matching the single-responsibility principle in the current design.
 */
class ImageProcessor
{
public:
  /**
   * @brief Converts an image to grayscale.
   * @param image Source image to convert.
   * @return Grayscale image, or a null QImage when the input is null.
   */
  QImage toGrayscale(const QImage& image) const;
};
