#pragma once

namespace qvp
{

/**
 * @brief Orientation of a 2D slice through a 3D volume.
 *
 * Used by future volume slice extraction code to identify the anatomical
 * plane requested by callers.
 */
enum class SliceOrientation
{
  Axial,
  Coronal,
  Sagittal
};

} // namespace qvp
