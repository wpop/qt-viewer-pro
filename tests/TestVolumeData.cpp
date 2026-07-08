#include "qtviewerpro/core/VolumeData.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <vector>
#include <utility>

class TestVolumeData : public QObject
{
  Q_OBJECT

private slots:
  void defaultConstructorCreatesEmptyVolume();
  void constructorStoresDimensions();
  void constructorStoresSpacing();
  void constructorStoresVoxelData();
  void constructorCachesIntensityRange();
  void legacyConstructorCreatesSpacingOnlyVolume();
  void defaultConstructorHasUnknownSpatialGeometry();
  void constructorStoresExplicitOrigin();
  void constructorStoresExplicitDirection();
  void constructorStoresCoordinateSystem();
  void constructorStoresExplicitOrientationTrust();
  void constructorStoresVoxelAxisAnatomy();
  void constantVolumeCachesSingleIntensityRange();
  void ctLikeVolumeCachesHounsfieldUnitRange();
  void copyPreservesIntensityRange();
  void movePreservesIntensityRange();
  void copyPreservesSpatialGeometry();
  void movePreservesSpatialGeometry();
};

void TestVolumeData::defaultConstructorCreatesEmptyVolume()
{
  const qvp::VolumeData volume;

  QCOMPARE(volume.width(), std::size_t{0});
  QCOMPARE(volume.height(), std::size_t{0});
  QCOMPARE(volume.depth(), std::size_t{0});
  QVERIFY(volume.voxels().empty());
  QVERIFY(!volume.hasIntensityRange());
}

void TestVolumeData::constructorStoresDimensions()
{
  const qvp::VolumeData volume(2, 3, 4, 1.0F, 1.0F, 1.0F, {});

  QCOMPARE(volume.width(), std::size_t{2});
  QCOMPARE(volume.height(), std::size_t{3});
  QCOMPARE(volume.depth(), std::size_t{4});
}

void TestVolumeData::constructorStoresSpacing()
{
  const qvp::VolumeData volume(1, 1, 1, 0.5F, 0.75F, 1.25F, {});

  QCOMPARE(volume.spacingX(), 0.5F);
  QCOMPARE(volume.spacingY(), 0.75F);
  QCOMPARE(volume.spacingZ(), 1.25F);
}

void TestVolumeData::constructorStoresVoxelData()
{
  const std::vector<float> voxels{1.0F, 2.0F, 3.0F, 4.0F};
  const qvp::VolumeData volume(2, 2, 1, 1.0F, 1.0F, 1.0F, voxels);

  QVERIFY(volume.voxels() == voxels);
}

void TestVolumeData::constructorCachesIntensityRange()
{
  const std::vector<float> voxels{3.5F, -7.0F, 11.25F, 0.0F};
  const qvp::VolumeData volume(2, 2, 1, 1.0F, 1.0F, 1.0F, voxels);

  QVERIFY(volume.hasIntensityRange());
  QCOMPARE(volume.intensityMinimum(), -7.0F);
  QCOMPARE(volume.intensityMaximum(), 11.25F);
}

void TestVolumeData::legacyConstructorCreatesSpacingOnlyVolume()
{
  const std::vector<float> voxels{1.0F, 2.0F, 3.0F, 4.0F};
  const qvp::VolumeData volume(2, 2, 1, 0.5F, 0.75F, 1.25F, voxels);
  const qvp::VolumeData::Origin defaultOrigin{0.0, 0.0, 0.0};
  const qvp::VolumeData::Direction identityDirection{1.0, 0.0, 0.0,
                                                     0.0, 1.0, 0.0,
                                                     0.0, 0.0, 1.0};

  QCOMPARE(volume.width(), std::size_t{2});
  QCOMPARE(volume.height(), std::size_t{2});
  QCOMPARE(volume.depth(), std::size_t{1});
  QCOMPARE(volume.spacingX(), 0.5F);
  QCOMPARE(volume.spacingY(), 0.75F);
  QCOMPARE(volume.spacingZ(), 1.25F);
  QVERIFY(volume.voxels() == voxels);
  QVERIFY(!volume.hasSpatialOrientation());
  QVERIFY(volume.spatialGeometry().coordinateSystem ==
          qvp::VolumeData::CoordinateSystem::Unknown);
  QVERIFY(volume.spatialGeometry().origin == defaultOrigin);
  QVERIFY(volume.spatialGeometry().direction == identityDirection);
}

void TestVolumeData::defaultConstructorHasUnknownSpatialGeometry()
{
  const qvp::VolumeData volume;

  QVERIFY(!volume.hasSpatialOrientation());
  QVERIFY(volume.spatialGeometry().coordinateSystem ==
          qvp::VolumeData::CoordinateSystem::Unknown);
}

void TestVolumeData::constructorStoresExplicitOrigin()
{
  qvp::VolumeData::SpatialGeometry geometry;
  geometry.origin = {12.5, -8.25, 42.0};

  const qvp::VolumeData volume(1, 1, 1, 1.0F, 1.0F, 1.0F, {7.0F}, geometry);

  QVERIFY(volume.spatialGeometry().origin == geometry.origin);
}

void TestVolumeData::constructorStoresExplicitDirection()
{
  qvp::VolumeData::SpatialGeometry geometry;
  geometry.direction = {0.0, -1.0, 0.0,
                        1.0, 0.0, 0.0,
                        0.0, 0.0, 1.0};

  const qvp::VolumeData volume(1, 1, 1, 1.0F, 1.0F, 1.0F, {7.0F}, geometry);

  QVERIFY(volume.spatialGeometry().direction == geometry.direction);
}

void TestVolumeData::constructorStoresCoordinateSystem()
{
  qvp::VolumeData::SpatialGeometry geometry;
  geometry.coordinateSystem = qvp::VolumeData::CoordinateSystem::LPS;

  const qvp::VolumeData volume(1, 1, 1, 1.0F, 1.0F, 1.0F, {7.0F}, geometry);

  QVERIFY(volume.spatialGeometry().coordinateSystem ==
          qvp::VolumeData::CoordinateSystem::LPS);
}

void TestVolumeData::constructorStoresExplicitOrientationTrust()
{
  qvp::VolumeData::SpatialGeometry untrustedGeometry;
  untrustedGeometry.coordinateSystem = qvp::VolumeData::CoordinateSystem::LPS;
  untrustedGeometry.origin = {1.0, 2.0, 3.0};

  const qvp::VolumeData untrustedVolume(
      1, 1, 1, 1.0F, 1.0F, 1.0F, {7.0F}, untrustedGeometry);
  QVERIFY(!untrustedVolume.hasSpatialOrientation());

  qvp::VolumeData::SpatialGeometry trustedGeometry = untrustedGeometry;
  trustedGeometry.hasOrientation = true;

  const qvp::VolumeData trustedVolume(
      1, 1, 1, 1.0F, 1.0F, 1.0F, {7.0F}, trustedGeometry);
  QVERIFY(trustedVolume.hasSpatialOrientation());
}

void TestVolumeData::constructorStoresVoxelAxisAnatomy()
{
  qvp::VolumeData::SpatialGeometry geometry;
  const qvp::VoxelAxisAnatomy anatomy{qvp::AnatomicalDirection::Left,
                                      qvp::AnatomicalDirection::Posterior,
                                      qvp::AnatomicalDirection::Superior};

  const qvp::VolumeData volume(1, 1, 1, 1.0F, 1.0F, 1.0F, {7.0F}, geometry, anatomy);

  QVERIFY(volume.hasVoxelAxisAnatomy());
  QVERIFY(volume.voxelAxisAnatomy().has_value());
  QCOMPARE(volume.voxelAxisAnatomy()->x, qvp::AnatomicalDirection::Left);
  QCOMPARE(volume.voxelAxisAnatomy()->y, qvp::AnatomicalDirection::Posterior);
  QCOMPARE(volume.voxelAxisAnatomy()->z, qvp::AnatomicalDirection::Superior);
  QVERIFY(!volume.hasSpatialOrientation());
}

void TestVolumeData::constantVolumeCachesSingleIntensityRange()
{
  const std::vector<float> voxels(8, 42.0F);
  const qvp::VolumeData volume(2, 2, 2, 1.0F, 1.0F, 1.0F, voxels);

  QVERIFY(volume.hasIntensityRange());
  QCOMPARE(volume.intensityMinimum(), 42.0F);
  QCOMPARE(volume.intensityMaximum(), 42.0F);
}

void TestVolumeData::ctLikeVolumeCachesHounsfieldUnitRange()
{
  const std::vector<float> voxels{-1024.0F, -950.0F, -120.0F, 0.0F, 45.0F, 325.0F, 1024.0F};
  const qvp::VolumeData volume(7, 1, 1, 0.7F, 0.7F, 1.5F, voxels);

  QVERIFY(volume.hasIntensityRange());
  QCOMPARE(volume.intensityMinimum(), -1024.0F);
  QCOMPARE(volume.intensityMaximum(), 1024.0F);

  const auto [minIt, maxIt] = std::minmax_element(voxels.begin(), voxels.end());
  QCOMPARE(volume.intensityMinimum(), *minIt);
  QCOMPARE(volume.intensityMaximum(), *maxIt);
}

void TestVolumeData::copyPreservesIntensityRange()
{
  const std::vector<float> voxels{-2.0F, 5.0F, 1.0F, 9.0F};
  const qvp::VolumeData original(2, 2, 1, 1.0F, 1.0F, 1.0F, voxels);
  const qvp::VolumeData copied = original;

  QVERIFY(copied.hasIntensityRange());
  QCOMPARE(copied.intensityMinimum(), -2.0F);
  QCOMPARE(copied.intensityMaximum(), 9.0F);
  QVERIFY(copied.voxels() == original.voxels());
}

void TestVolumeData::movePreservesIntensityRange()
{
  const std::vector<float> voxels{-9.5F, 4.25F, 2.0F, 13.0F};
  qvp::VolumeData original(2, 2, 1, 1.0F, 1.0F, 1.0F, voxels);
  qvp::VolumeData moved = std::move(original);

  QVERIFY(moved.isValid());
  QVERIFY(moved.hasIntensityRange());
  QCOMPARE(moved.intensityMinimum(), -9.5F);
  QCOMPARE(moved.intensityMaximum(), 13.0F);
}

void TestVolumeData::copyPreservesSpatialGeometry()
{
  qvp::VolumeData::SpatialGeometry geometry;
  geometry.origin = {4.0, 5.0, 6.0};
  geometry.direction = {0.0, 1.0, 0.0,
                        1.0, 0.0, 0.0,
                        0.0, 0.0, -1.0};
  geometry.coordinateSystem = qvp::VolumeData::CoordinateSystem::LPS;
  geometry.hasOrientation = true;

  const qvp::VolumeData original(1, 1, 1, 1.0F, 1.0F, 1.0F, {7.0F}, geometry);
  const qvp::VolumeData copied = original;

  QVERIFY(copied.hasSpatialOrientation());
  QVERIFY(copied.spatialGeometry().origin == geometry.origin);
  QVERIFY(copied.spatialGeometry().direction == geometry.direction);
  QVERIFY(copied.spatialGeometry().coordinateSystem ==
          qvp::VolumeData::CoordinateSystem::LPS);
}

void TestVolumeData::movePreservesSpatialGeometry()
{
  qvp::VolumeData::SpatialGeometry geometry;
  geometry.origin = {-1.0, -2.0, -3.0};
  geometry.direction = {-1.0, 0.0, 0.0,
                        0.0, -1.0, 0.0,
                        0.0, 0.0, 1.0};
  geometry.coordinateSystem = qvp::VolumeData::CoordinateSystem::RAS;
  geometry.hasOrientation = true;

  qvp::VolumeData original(1, 1, 1, 1.0F, 1.0F, 1.0F, {7.0F}, geometry);
  qvp::VolumeData moved = std::move(original);

  QVERIFY(moved.hasSpatialOrientation());
  QVERIFY(moved.spatialGeometry().origin == geometry.origin);
  QVERIFY(moved.spatialGeometry().direction == geometry.direction);
  QVERIFY(moved.spatialGeometry().coordinateSystem ==
          qvp::VolumeData::CoordinateSystem::RAS);
}

QTEST_MAIN(TestVolumeData)

#include "TestVolumeData.moc"
