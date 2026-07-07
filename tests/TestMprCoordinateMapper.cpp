#include "qtviewerpro/ui/MprCoordinateMapper.h"

#include <QtTest/QtTest>

#include <vector>

class TestMprCoordinateMapper : public QObject
{
  Q_OBJECT

private slots:
  void mapsFirstPixelToZeroForAllOrientations();
  void mapsAxialImageCoordinatesToVoxelPosition();
  void mapsSagittalImageCoordinatesToVoxelPosition();
  void mapsCoronalImageCoordinatesToVoxelPosition();
  void clampsImageCoordinatesToVolumeBounds();
  void returnsCrosshairImagePointsForAllOrientations();
};

namespace
{

qvp::VolumeData makeVolume()
{
  return qvp::VolumeData(4, 3, 2, 0.5F, 0.75F, 1.25F, std::vector<float>(4 * 3 * 2, 0.0F));
}

} // namespace

void TestMprCoordinateMapper::mapsFirstPixelToZeroForAllOrientations()
{
  const auto volume = makeVolume();

  const qvp::MprVoxelPosition axialCurrent{2, 1, 1};
  const auto axial = qvp::MprCoordinateMapper::voxelPositionFromImagePoint(
      volume, qvp::SliceOrientation::Axial, 0, 0, axialCurrent);
  QCOMPARE(axial.x, std::size_t{0});
  QCOMPARE(axial.y, std::size_t{0});
  QCOMPARE(axial.z, std::size_t{1});

  const qvp::MprVoxelPosition sagittalCurrent{2, 1, 1};
  const auto sagittal = qvp::MprCoordinateMapper::voxelPositionFromImagePoint(
      volume, qvp::SliceOrientation::Sagittal, 0, 0, sagittalCurrent);
  QCOMPARE(sagittal.x, std::size_t{2});
  QCOMPARE(sagittal.y, std::size_t{0});
  QCOMPARE(sagittal.z, std::size_t{0});

  const qvp::MprVoxelPosition coronalCurrent{2, 1, 1};
  const auto coronal = qvp::MprCoordinateMapper::voxelPositionFromImagePoint(
      volume, qvp::SliceOrientation::Coronal, 0, 0, coronalCurrent);
  QCOMPARE(coronal.x, std::size_t{0});
  QCOMPARE(coronal.y, std::size_t{1});
  QCOMPARE(coronal.z, std::size_t{0});
}

void TestMprCoordinateMapper::mapsAxialImageCoordinatesToVoxelPosition()
{
  const auto volume = makeVolume();
  const qvp::MprVoxelPosition current{1, 1, 1};

  const auto updated = qvp::MprCoordinateMapper::voxelPositionFromImagePoint(
      volume, qvp::SliceOrientation::Axial, 3, 2, current);

  QCOMPARE(updated.x, std::size_t{3});
  QCOMPARE(updated.y, std::size_t{2});
  QCOMPARE(updated.z, std::size_t{1});
}

void TestMprCoordinateMapper::mapsSagittalImageCoordinatesToVoxelPosition()
{
  const auto volume = makeVolume();
  const qvp::MprVoxelPosition current{2, 0, 0};

  const auto updated = qvp::MprCoordinateMapper::voxelPositionFromImagePoint(
      volume, qvp::SliceOrientation::Sagittal, 2, 1, current);

  QCOMPARE(updated.x, std::size_t{2});
  QCOMPARE(updated.y, std::size_t{2});
  QCOMPARE(updated.z, std::size_t{1});
}

void TestMprCoordinateMapper::mapsCoronalImageCoordinatesToVoxelPosition()
{
  const auto volume = makeVolume();
  const qvp::MprVoxelPosition current{0, 1, 0};

  const auto updated = qvp::MprCoordinateMapper::voxelPositionFromImagePoint(
      volume, qvp::SliceOrientation::Coronal, 3, 1, current);

  QCOMPARE(updated.x, std::size_t{3});
  QCOMPARE(updated.y, std::size_t{1});
  QCOMPARE(updated.z, std::size_t{1});
}

void TestMprCoordinateMapper::clampsImageCoordinatesToVolumeBounds()
{
  const auto volume = makeVolume();
  const qvp::MprVoxelPosition current{1, 1, 1};

  const auto sagittal = qvp::MprCoordinateMapper::voxelPositionFromImagePoint(
      volume, qvp::SliceOrientation::Sagittal, 99, 99, current);
  QCOMPARE(sagittal.x, std::size_t{1});
  QCOMPARE(sagittal.y, std::size_t{2});
  QCOMPARE(sagittal.z, std::size_t{1});

  const auto coronal = qvp::MprCoordinateMapper::voxelPositionFromImagePoint(
      volume, qvp::SliceOrientation::Coronal, 99, 99, current);
  QCOMPARE(coronal.x, std::size_t{3});
  QCOMPARE(coronal.y, std::size_t{1});
  QCOMPARE(coronal.z, std::size_t{1});
}

void TestMprCoordinateMapper::returnsCrosshairImagePointsForAllOrientations()
{
  const qvp::MprVoxelPosition position{3, 2, 1};

  const auto axial =
      qvp::MprCoordinateMapper::crosshairImagePoint(qvp::SliceOrientation::Axial, position);
  QCOMPARE(axial.x, std::size_t{3});
  QCOMPARE(axial.y, std::size_t{2});

  const auto sagittal =
      qvp::MprCoordinateMapper::crosshairImagePoint(qvp::SliceOrientation::Sagittal, position);
  QCOMPARE(sagittal.x, std::size_t{2});
  QCOMPARE(sagittal.y, std::size_t{1});

  const auto coronal =
      qvp::MprCoordinateMapper::crosshairImagePoint(qvp::SliceOrientation::Coronal, position);
  QCOMPARE(coronal.x, std::size_t{3});
  QCOMPARE(coronal.y, std::size_t{1});
}

QTEST_MAIN(TestMprCoordinateMapper)

#include "TestMprCoordinateMapper.moc"
