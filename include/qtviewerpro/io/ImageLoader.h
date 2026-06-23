#pragma once

#include <QString>

class QImage;

namespace qvp
{

/**
 * @brief Loads image files into Qt image objects.
 *
 * ImageLoader keeps file decoding separate from UI code, supporting a focused
 * single responsibility for image input.
 */
class ImageLoader
{
public:
  /**
   * @brief Loads an image from disk.
   * @param fileName Path to the image file.
   * @return Loaded image, or a null QImage when the file cannot be decoded.
   */
  QImage load(const QString& fileName) const;
};

} // namespace qvp
