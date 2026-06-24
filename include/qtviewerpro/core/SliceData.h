#pragma once

#include "qtviewerpro/core/SliceOrientation.h"

#include <cstddef>
#include <vector>

namespace qvp
{

/**
 * @brief Stores pixel data and metadata for a 2D slice.
 *
 * SliceData is a lightweight container for extracted volume slices. It is not
 * connected to rendering or UI code.
 */
class SliceData
{
public:
  /**
   * @brief Creates an empty slice with zero dimensions and unit spacing.
   */
  SliceData() = default;

  /**
   * @brief Creates a slice with dimensions, spacing, orientation, index, and pixels.
   * @param width Number of pixels along the X axis.
   * @param height Number of pixels along the Y axis.
   * @param spacingX Physical spacing between pixels along the X axis.
   * @param spacingY Physical spacing between pixels along the Y axis.
   * @param orientation Orientation of the slice in the source volume.
   * @param sliceIndex Zero-based index of the slice in the source volume.
   * @param pixels Linear pixel values stored as floats.
   */
  SliceData(std::size_t width,
            std::size_t height,
            float spacingX,
            float spacingY,
            SliceOrientation orientation,
            std::size_t sliceIndex,
            std::vector<float> pixels);

  /**
   * @brief Returns the number of pixels along the X axis.
   */
  std::size_t width() const;

  /**
   * @brief Returns the number of pixels along the Y axis.
   */
  std::size_t height() const;

  /**
   * @brief Returns the physical pixel spacing along the X axis.
   */
  float spacingX() const;

  /**
   * @brief Returns the physical pixel spacing along the Y axis.
   */
  float spacingY() const;

  /**
   * @brief Returns the orientation of the slice in the source volume.
   */
  SliceOrientation orientation() const;

  /**
   * @brief Returns the zero-based slice index in the source volume.
   */
  std::size_t sliceIndex() const;

  /**
   * @brief Returns the linear pixel value buffer.
   */
  const std::vector<float>& pixels() const;

private:
  std::size_t width_ = 0;
  std::size_t height_ = 0;
  float spacingX_ = 1.0F;
  float spacingY_ = 1.0F;
  SliceOrientation orientation_ = SliceOrientation::Axial;
  std::size_t sliceIndex_ = 0;
  std::vector<float> pixels_;
};

} // namespace qvp
