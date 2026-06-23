#pragma once

#include <cstddef>
#include <vector>

namespace qvp
{

/**
 * @brief Stores voxel data and metadata for a 3D volume.
 *
 * VolumeData is a lightweight container for future medical or scientific
 * volume workflows. It is not connected to rendering or UI code yet.
 */
class VolumeData
{
public:
  /**
   * @brief Creates an empty volume with zero dimensions and unit spacing.
   */
  VolumeData() = default;

  /**
   * @brief Creates a volume with dimensions, spacing, and voxel values.
   * @param width Number of voxels along the X axis.
   * @param height Number of voxels along the Y axis.
   * @param depth Number of voxels along the Z axis.
   * @param spacingX Physical spacing between voxels along the X axis.
   * @param spacingY Physical spacing between voxels along the Y axis.
   * @param spacingZ Physical spacing between voxels along the Z axis.
   * @param voxels Linear voxel values stored as floats.
   */
  VolumeData(std::size_t width,
             std::size_t height,
             std::size_t depth,
             float spacingX,
             float spacingY,
             float spacingZ,
             std::vector<float> voxels);

  /**
   * @brief Returns the number of voxels along the X axis.
   */
  std::size_t width() const;

  /**
   * @brief Returns the number of voxels along the Y axis.
   */
  std::size_t height() const;

  /**
   * @brief Returns the number of voxels along the Z axis.
   */
  std::size_t depth() const;

  /**
   * @brief Returns the physical voxel spacing along the X axis.
   */
  float spacingX() const;

  /**
   * @brief Returns the physical voxel spacing along the Y axis.
   */
  float spacingY() const;

  /**
   * @brief Returns the physical voxel spacing along the Z axis.
   */
  float spacingZ() const;

  /**
   * @brief Returns the linear voxel value buffer.
   */
  const std::vector<float>& voxels() const;

private:
  std::size_t width_ = 0;
  std::size_t height_ = 0;
  std::size_t depth_ = 0;
  float spacingX_ = 1.0F;
  float spacingY_ = 1.0F;
  float spacingZ_ = 1.0F;
  std::vector<float> voxels_;
};

} // namespace qvp
