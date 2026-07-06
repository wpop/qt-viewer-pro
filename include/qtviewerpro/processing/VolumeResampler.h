#pragma once

#include "qtviewerpro/core/VolumeData.h"

namespace qvp
{

/**
 * @brief Resamples a medical volume to 1 mm isotropic spacing.
 *
 * The resampler preserves raw voxel intensities and relies on ITK linear
 * interpolation so CT-style intensity data can continue flowing to the
 * existing rendering presets.
 */
class VolumeResampler
{
public:
  /**
   * @brief Resamples the input volume to 1.0 mm spacing on all axes.
   * @param volume Source medical volume.
   * @return A new volume resampled to isotropic spacing.
   * @throws std::invalid_argument If the input volume is invalid.
   * @throws std::runtime_error If ITK resampling fails.
   */
  static VolumeData resampleToIsotropicSpacing(const VolumeData& volume);
};

} // namespace qvp
