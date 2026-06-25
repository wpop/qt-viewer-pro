#pragma once

#include "qtviewerpro/core/VolumeData.h"

#include <QString>

namespace qvp
{

/**
 * @brief Loads float32 volume data from JSON metadata and RAW voxel files.
 */
class RawVolumeLoader
{
public:
  /**
   * @brief Loads a volume from metadata and raw voxel data files.
   * @param metadataPath Path to the JSON metadata file.
   * @param rawPath Path to the RAW float32 voxel file.
   * @return Loaded volume data.
   * @throws std::runtime_error when metadata or raw data is invalid.
   */
  static VolumeData load(const QString& metadataPath, const QString& rawPath);
};

} // namespace qvp
