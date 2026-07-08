#include "qtviewerpro/core/VolumeInformation.h"

#include <QtTest/QtTest>

class TestVolumeInformation : public QObject
{
  Q_OBJECT

private slots:
  void extractsDimensions();
  void extractsSpacing();
  void extractsVoxelCount();
  void computesMemoryBytes();
  void preservesOrigin();
  void preservesDirection();
  void preservesCoordinateSystem();
  void preservesPatientWorldTrust();
  void preservesVoxelAxisAnatomy();
  void extractsCachedIntensityRange();
  void doesNotInventIntensityRangeForEmptyVolume();
};

namespace
{

qvp::VolumeData::SpatialGeometry makeGeometry()
{
  qvp::VolumeData::SpatialGeometry geometry;
  geometry.origin = {12.5, -8.25, 42.0};
  geometry.direction = {0.0, -1.0, 0.0,
                        1.0, 0.0, 0.0,
                        0.0, 0.0, 1.0};
  geometry.coordinateSystem = qvp::VolumeData::CoordinateSystem::RAS;
  geometry.hasOrientation = true;
  return geometry;
}

qvp::VolumeData makeVolume()
{
  const auto geometry = makeGeometry();
  const qvp::VoxelAxisAnatomy anatomy{qvp::AnatomicalDirection::Right,
                                      qvp::AnatomicalDirection::Anterior,
                                      qvp::AnatomicalDirection::Inferior};
  return qvp::VolumeData(2, 3, 4, 0.5F, 0.75F, 1.25F, {1.0F, -2.0F, 3.5F, 4.0F,
                                                        5.0F, 6.0F, 7.0F, 8.0F,
                                                        9.0F, 10.0F, 11.0F, 12.0F,
                                                        13.0F, 14.0F, 15.0F, 16.0F,
                                                        17.0F, 18.0F, 19.0F, 20.0F,
                                                        21.0F, 22.0F, 23.0F, 24.0F},
                         geometry,
                         anatomy);
}

} // namespace

void TestVolumeInformation::extractsDimensions()
{
  const auto information = qvp::makeVolumeInformation(makeVolume());

  QCOMPARE(information.width, std::size_t{2});
  QCOMPARE(information.height, std::size_t{3});
  QCOMPARE(information.depth, std::size_t{4});
}

void TestVolumeInformation::extractsSpacing()
{
  const auto information = qvp::makeVolumeInformation(makeVolume());

  QCOMPARE(information.spacingX, 0.5);
  QCOMPARE(information.spacingY, 0.75);
  QCOMPARE(information.spacingZ, 1.25);
}

void TestVolumeInformation::extractsVoxelCount()
{
  const auto information = qvp::makeVolumeInformation(makeVolume());

  QCOMPARE(information.voxelCount, std::size_t{24});
}

void TestVolumeInformation::computesMemoryBytes()
{
  const auto information = qvp::makeVolumeInformation(makeVolume());

  QCOMPARE(information.memoryBytes, std::size_t{24 * sizeof(float)});
}

void TestVolumeInformation::preservesOrigin()
{
  const auto geometry = makeGeometry();
  const auto information = qvp::makeVolumeInformation(makeVolume());

  QCOMPARE(information.origin, geometry.origin);
}

void TestVolumeInformation::preservesDirection()
{
  const auto geometry = makeGeometry();
  const auto information = qvp::makeVolumeInformation(makeVolume());

  QCOMPARE(information.direction, geometry.direction);
}

void TestVolumeInformation::preservesCoordinateSystem()
{
  const auto information = qvp::makeVolumeInformation(makeVolume());

  QCOMPARE(information.coordinateSystem, qvp::VolumeData::CoordinateSystem::RAS);
}

void TestVolumeInformation::preservesPatientWorldTrust()
{
  const auto information = qvp::makeVolumeInformation(makeVolume());

  QVERIFY(information.patientWorldOrientationTrusted);
}

void TestVolumeInformation::preservesVoxelAxisAnatomy()
{
  const auto information = qvp::makeVolumeInformation(makeVolume());

  QVERIFY(information.voxelAxisAnatomy.has_value());
  QCOMPARE(information.voxelAxisAnatomy->x, qvp::AnatomicalDirection::Right);
  QCOMPARE(information.voxelAxisAnatomy->y, qvp::AnatomicalDirection::Anterior);
  QCOMPARE(information.voxelAxisAnatomy->z, qvp::AnatomicalDirection::Inferior);
}

void TestVolumeInformation::extractsCachedIntensityRange()
{
  const auto information = qvp::makeVolumeInformation(makeVolume());

  QVERIFY(information.hasIntensityRange);
  QCOMPARE(information.intensityMinimum, -2.0F);
  QCOMPARE(information.intensityMaximum, 24.0F);
}

void TestVolumeInformation::doesNotInventIntensityRangeForEmptyVolume()
{
  const qvp::VolumeData volume;
  const auto information = qvp::makeVolumeInformation(volume);

  QVERIFY(!information.hasIntensityRange);
  QCOMPARE(information.intensityMinimum, 0.0F);
  QCOMPARE(information.intensityMaximum, 0.0F);
  QCOMPARE(information.voxelCount, std::size_t{0});
  QCOMPARE(information.memoryBytes, std::size_t{0});
}

QTEST_MAIN(TestVolumeInformation)

#include "TestVolumeInformation.moc"
