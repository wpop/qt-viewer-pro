#pragma once

#include "qtviewerpro/core/SliceOrientation.h"
#include "qtviewerpro/core/VolumeData.h"

#include <cstddef>
#include <vector>

namespace qvp
{

/**
 * @brief Extracts 2D voxel slices from a 3D volume.
 *
 * Slices are returned as row-major float buffers. Axial slices are width by
 * height, coronal slices are width by depth, and sagittal slices are height by
 * depth.
 */
class SliceExtractor
{
public:
  /**
   * @brief Extracts a slice from the given volume.
   * @param volume Source volume data.
   * @param orientation Orientation of the requested slice.
   * @param sliceIndex Zero-based slice index along the orientation axis.
   * @return Row-major float buffer containing the extracted slice.
   * @throws std::out_of_range If sliceIndex is outside the volume bounds.
   */
  static std::vector<float> extract(const VolumeData& volume,
                                    SliceOrientation orientation,
                                    std::size_t sliceIndex);
};

} // namespace qvp
