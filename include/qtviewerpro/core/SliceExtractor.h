#pragma once

#include "qtviewerpro/core/SliceData.h"
#include "qtviewerpro/core/SliceOrientation.h"
#include "qtviewerpro/core/VolumeData.h"

#include <cstddef>

namespace qvp
{

/**
 * @brief Extracts 2D voxel slices from a 3D volume.
 *
 * Slices are returned with row-major pixel buffers and metadata describing
 * their dimensions, spacing, orientation, and source slice index.
 */
class SliceExtractor
{
public:
  /**
   * @brief Extracts a slice from the given volume.
   * @param volume Source volume data.
   * @param orientation Orientation of the requested slice.
   * @param sliceIndex Zero-based slice index along the orientation axis.
   * @return Slice data containing metadata and extracted pixels.
   * @throws std::out_of_range If sliceIndex is outside the volume bounds.
   */
  static SliceData extract(const VolumeData& volume,
                           SliceOrientation orientation,
                           std::size_t sliceIndex);
};

} // namespace qvp
