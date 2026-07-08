#pragma once

#include "qtviewerpro/core/AnatomicalOrientation.h"

#include <array>
#include <cstddef>
#include <optional>
#include <vector>
#include <utility>

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
  using Origin = std::array<double, 3>;
  using Direction = std::array<double, 9>;

  enum class CoordinateSystem
  {
    Unknown,
    LPS,
    RAS
  };

  struct SpatialGeometry
  {
    Origin origin{0.0, 0.0, 0.0};
    Direction direction{1.0, 0.0, 0.0,
                        0.0, 1.0, 0.0,
                        0.0, 0.0, 1.0};
    CoordinateSystem coordinateSystem = CoordinateSystem::Unknown;
    bool hasOrientation = false;
  };

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
  VolumeData(std::size_t width, std::size_t height, std::size_t depth, float spacingX,
             float spacingY, float spacingZ, std::vector<float> voxels);

  /**
   * @brief Creates a volume with dimensions, spacing, voxel values, and spatial geometry.
   * @param width Number of voxels along the X axis.
   * @param height Number of voxels along the Y axis.
   * @param depth Number of voxels along the Z axis.
   * @param spacingX Physical spacing between voxels along the X axis.
   * @param spacingY Physical spacing between voxels along the Y axis.
   * @param spacingZ Physical spacing between voxels along the Z axis.
   * @param voxels Linear voxel values stored as floats.
   * @param spatialGeometry Optional origin, direction, and coordinate system metadata.
   */
  VolumeData(std::size_t width,
             std::size_t height,
             std::size_t depth,
             float spacingX,
             float spacingY,
             float spacingZ,
             std::vector<float> voxels,
             SpatialGeometry spatialGeometry,
             std::optional<VoxelAxisAnatomy> voxelAxisAnatomy = std::nullopt);

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

  /**
   * @brief Returns optional spatial geometry metadata for the volume.
   */
  const SpatialGeometry& spatialGeometry() const noexcept;

  /**
   * @brief Returns optional voxel-axis anatomical direction metadata.
   */
  const std::optional<VoxelAxisAnatomy>& voxelAxisAnatomy() const noexcept;

  /**
   * @brief Returns true when voxel-axis anatomical directions are available.
   */
  bool hasVoxelAxisAnatomy() const noexcept;

  /**
   * @brief Returns true when spatial orientation is explicitly trusted.
   */
  bool hasSpatialOrientation() const noexcept;

  /**
   * @brief Returns true when the volume has a cached intensity range.
   */
  bool hasIntensityRange() const;

  /**
   * @brief Returns the minimum voxel intensity in the cached range.
   */
  float intensityMinimum() const;

  /**
   * @brief Returns the maximum voxel intensity in the cached range.
   */
  float intensityMaximum() const;

  /**
   * @brief Returns true when the volume has no usable voxel data.
   */
  bool isEmpty() const;

  /**
   * @brief Returns true when dimensions and voxel storage are internally consistent.
   */
  bool isValid() const;

  /**
   * @brief Returns the number of voxels implied by the dimensions.
   */
  std::size_t voxelCount() const;

private:
  std::size_t width_ = 0;
  std::size_t height_ = 0;
  std::size_t depth_ = 0;
  float spacingX_ = 1.0F;
  float spacingY_ = 1.0F;
  float spacingZ_ = 1.0F;
  std::vector<float> voxels_;
  float intensityMinimum_ = 0.0F;
  float intensityMaximum_ = 0.0F;
  bool hasIntensityRange_ = false;
  SpatialGeometry spatialGeometry_;
  std::optional<VoxelAxisAnatomy> voxelAxisAnatomy_;
};

} // namespace qvp
