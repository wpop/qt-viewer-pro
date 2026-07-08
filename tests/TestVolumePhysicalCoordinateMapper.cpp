#include "qtviewerpro/core/VolumePhysicalCoordinateMapper.h"

#include <QtTest/QtTest>

#include <cmath>
#include <vector>

class TestVolumePhysicalCoordinateMapper : public QObject
{
  Q_OBJECT

private slots:
  void zeroIndexReturnsOrigin();
  void identityDirectionUsesSpacing();
  void anisotropicSpacing();
  void rotatedDirectionMatrix();
  void negativeAxes();
  void nonZeroOrigin();
  void edgeVoxel();
  void untrustedGeometryStillMapsNumerically();
};

namespace
{

constexpr double kTolerance = 1e-9;

qvp::VolumeData makeVolume(std::size_t width,
                           std::size_t height,
                           std::size_t depth,
                           float spacingX,
                           float spacingY,
                           float spacingZ,
                           const qvp::VolumeData::Origin& origin,
                           const qvp::VolumeData::Direction& direction,
                           bool hasOrientation = true,
                           qvp::VolumeData::CoordinateSystem coordinateSystem =
                               qvp::VolumeData::CoordinateSystem::LPS)
{
  qvp::VolumeData::SpatialGeometry geometry;
  geometry.origin = origin;
  geometry.direction = direction;
  geometry.coordinateSystem = coordinateSystem;
  geometry.hasOrientation = hasOrientation;

  return qvp::VolumeData(
      width, height, depth, spacingX, spacingY, spacingZ, std::vector<float>(width * height * depth, 0.0F), geometry);
}

void comparePoint(const qvp::PhysicalPoint3D& actual,
                  double expectedX,
                  double expectedY,
                  double expectedZ)
{
  QVERIFY(std::fabs(actual.x - expectedX) < kTolerance);
  QVERIFY(std::fabs(actual.y - expectedY) < kTolerance);
  QVERIFY(std::fabs(actual.z - expectedZ) < kTolerance);
}

} // namespace

void TestVolumePhysicalCoordinateMapper::zeroIndexReturnsOrigin()
{
  const qvp::VolumeData volume = makeVolume(3,
                                            4,
                                            5,
                                            2.0F,
                                            3.0F,
                                            4.0F,
                                            {10.5, -20.25, 30.75},
                                            {1.0, 0.0, 0.0,
                                             0.0, 1.0, 0.0,
                                             0.0, 0.0, 1.0});

  const auto point =
      qvp::VolumePhysicalCoordinateMapper::voxelToPhysical(volume, qvp::VoxelIndex3D{0, 0, 0});

  comparePoint(point, 10.5, -20.25, 30.75);
}

void TestVolumePhysicalCoordinateMapper::identityDirectionUsesSpacing()
{
  const qvp::VolumeData volume = makeVolume(4,
                                            5,
                                            6,
                                            2.0F,
                                            3.0F,
                                            4.0F,
                                            {10.0, 20.0, 30.0},
                                            {1.0, 0.0, 0.0,
                                             0.0, 1.0, 0.0,
                                             0.0, 0.0, 1.0});

  const auto point =
      qvp::VolumePhysicalCoordinateMapper::voxelToPhysical(volume, qvp::VoxelIndex3D{1, 2, 3});

  comparePoint(point, 12.0, 26.0, 42.0);
}

void TestVolumePhysicalCoordinateMapper::anisotropicSpacing()
{
  const qvp::VolumeData volume = makeVolume(10,
                                            10,
                                            10,
                                            0.5F,
                                            1.25F,
                                            2.5F,
                                            {1.0, 2.0, 3.0},
                                            {1.0, 0.0, 0.0,
                                             0.0, 1.0, 0.0,
                                             0.0, 0.0, 1.0});

  const auto point =
      qvp::VolumePhysicalCoordinateMapper::voxelToPhysical(volume, qvp::VoxelIndex3D{4, 3, 2});

  comparePoint(point, 3.0, 5.75, 8.0);
}

void TestVolumePhysicalCoordinateMapper::rotatedDirectionMatrix()
{
  const qvp::VolumeData volume = makeVolume(8,
                                            8,
                                            8,
                                            2.0F,
                                            3.0F,
                                            4.0F,
                                            {100.0, 200.0, 300.0},
                                            {0.0, -1.0, 0.0,
                                             1.0, 0.0, 0.0,
                                             0.0, 0.0, 1.0});

  const auto point =
      qvp::VolumePhysicalCoordinateMapper::voxelToPhysical(volume, qvp::VoxelIndex3D{1, 2, 3});

  comparePoint(point, 94.0, 202.0, 312.0);
}

void TestVolumePhysicalCoordinateMapper::negativeAxes()
{
  const qvp::VolumeData volume = makeVolume(6,
                                            6,
                                            6,
                                            1.5F,
                                            2.0F,
                                            2.5F,
                                            {0.0, 0.0, 0.0},
                                            {-1.0, 0.0, 0.0,
                                             0.0, -1.0, 0.0,
                                             0.0, 0.0, 1.0});

  const auto point =
      qvp::VolumePhysicalCoordinateMapper::voxelToPhysical(volume, qvp::VoxelIndex3D{2, 3, 4});

  comparePoint(point, -3.0, -6.0, 10.0);
}

void TestVolumePhysicalCoordinateMapper::nonZeroOrigin()
{
  const qvp::VolumeData volume = makeVolume(5,
                                            5,
                                            5,
                                            2.0F,
                                            2.0F,
                                            2.0F,
                                            {-5.0, 10.0, 100.0},
                                            {1.0, 0.0, 0.0,
                                             0.0, 1.0, 0.0,
                                             0.0, 0.0, 1.0});

  const auto point =
      qvp::VolumePhysicalCoordinateMapper::voxelToPhysical(volume, qvp::VoxelIndex3D{3, 1, 2});

  comparePoint(point, 1.0, 12.0, 104.0);
}

void TestVolumePhysicalCoordinateMapper::edgeVoxel()
{
  const qvp::VolumeData volume = makeVolume(4,
                                            3,
                                            2,
                                            1.5F,
                                            2.0F,
                                            2.5F,
                                            {10.0, 20.0, 30.0},
                                            {1.0, 0.0, 0.0,
                                             0.0, 1.0, 0.0,
                                             0.0, 0.0, 1.0});

  const auto point = qvp::VolumePhysicalCoordinateMapper::voxelToPhysical(
      volume, qvp::VoxelIndex3D{volume.width() - 1, volume.height() - 1, volume.depth() - 1});

  comparePoint(point, 14.5, 24.0, 32.5);
}

void TestVolumePhysicalCoordinateMapper::untrustedGeometryStillMapsNumerically()
{
  const qvp::VolumeData volume = makeVolume(4,
                                            4,
                                            4,
                                            2.0F,
                                            3.0F,
                                            4.0F,
                                            {7.0, 8.0, 9.0},
                                            {1.0, 0.0, 0.0,
                                             0.0, 1.0, 0.0,
                                             0.0, 0.0, 1.0},
                                            false,
                                            qvp::VolumeData::CoordinateSystem::Unknown);

  QVERIFY(!volume.hasSpatialOrientation());

  const auto point =
      qvp::VolumePhysicalCoordinateMapper::voxelToPhysical(volume, qvp::VoxelIndex3D{1, 2, 3});

  comparePoint(point, 9.0, 14.0, 21.0);
}

QTEST_MAIN(TestVolumePhysicalCoordinateMapper)

#include "TestVolumePhysicalCoordinateMapper.moc"
