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
  void constantVolumeCachesSingleIntensityRange();
  void ctLikeVolumeCachesHounsfieldUnitRange();
  void copyPreservesIntensityRange();
  void movePreservesIntensityRange();
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

QTEST_MAIN(TestVolumeData)

#include "TestVolumeData.moc"
