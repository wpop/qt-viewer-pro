#include "qtviewerpro/core/VolumeData.h"

#include <QtTest/QtTest>

#include <vector>

class TestVolumeData : public QObject
{
  Q_OBJECT

private slots:
  void defaultConstructorCreatesEmptyVolume();
  void constructorStoresDimensions();
  void constructorStoresSpacing();
  void constructorStoresVoxelData();
};

void TestVolumeData::defaultConstructorCreatesEmptyVolume()
{
  const qvp::VolumeData volume;

  QCOMPARE(volume.width(), std::size_t{0});
  QCOMPARE(volume.height(), std::size_t{0});
  QCOMPARE(volume.depth(), std::size_t{0});
  QVERIFY(volume.voxels().empty());
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

QTEST_MAIN(TestVolumeData)

#include "TestVolumeData.moc"
