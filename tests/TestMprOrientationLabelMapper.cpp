#include "qtviewerpro/ui/MprOrientationLabelMapper.h"

#include <QtTest/QtTest>

#include <limits>
#include <optional>

class TestMprOrientationLabelMapper : public QObject
{
  Q_OBJECT

private slots:
  void untrustedGeometryReturnsNoLabels();
  void unknownCoordinateSystemReturnsNoLabels();
  void lpsIdentityAxial();
  void rasIdentityAxial();
  void lpsIdentitySagittal();
  void lpsIdentityCoronal();
  void flippedXInLpsAxial();
  void flippedYInLpsAxial();
  void flippedZInLpsSagittal();
  void permutedAxes();
  void ninetyDegreeRotation();
  void clearObliqueDirectionAccepted();
  void ambiguousFortyFiveDegreeDirectionRejected();
  void degenerateAxisRejected();
  void nonFiniteDirectionRejected();
  void duplicateDominantAxesRejected();
  void voxelAxisAnatomyLpsMapsAllOrientations();
  void voxelAxisAnatomyRaiMapsAllOrientations();
  void voxelAxisAnatomyUnknownFailsClosed();
};

namespace
{

qvp::VolumeData::SpatialGeometry makeGeometry()
{
  qvp::VolumeData::SpatialGeometry geometry;
  geometry.coordinateSystem = qvp::VolumeData::CoordinateSystem::LPS;
  geometry.hasOrientation = true;
  return geometry;
}

qvp::VoxelAxisAnatomy makeVoxelAxisAnatomy(qvp::AnatomicalDirection x,
                                           qvp::AnatomicalDirection y,
                                           qvp::AnatomicalDirection z)
{
  return qvp::VoxelAxisAnatomy{x, y, z};
}

void verifyLabels(const std::optional<qvp::OrientationEdgeLabels>& labels,
                  std::string_view left,
                  std::string_view right,
                  std::string_view top,
                  std::string_view bottom)
{
  QVERIFY(labels.has_value());
  QCOMPARE(labels->left, left);
  QCOMPARE(labels->right, right);
  QCOMPARE(labels->top, top);
  QCOMPARE(labels->bottom, bottom);
}

} // namespace

void TestMprOrientationLabelMapper::untrustedGeometryReturnsNoLabels()
{
  auto geometry = makeGeometry();
  geometry.hasOrientation = false;

  QVERIFY(!qvp::MprOrientationLabelMapper::edgeLabels(
               geometry, qvp::SliceOrientation::Axial)
               .has_value());
}

void TestMprOrientationLabelMapper::unknownCoordinateSystemReturnsNoLabels()
{
  auto geometry = makeGeometry();
  geometry.coordinateSystem = qvp::VolumeData::CoordinateSystem::Unknown;

  QVERIFY(!qvp::MprOrientationLabelMapper::edgeLabels(
               geometry, qvp::SliceOrientation::Axial)
               .has_value());
}

void TestMprOrientationLabelMapper::lpsIdentityAxial()
{
  const auto geometry = makeGeometry();

  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   geometry, qvp::SliceOrientation::Axial),
               "R",
               "L",
               "A",
               "P");
}

void TestMprOrientationLabelMapper::rasIdentityAxial()
{
  auto geometry = makeGeometry();
  geometry.coordinateSystem = qvp::VolumeData::CoordinateSystem::RAS;

  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   geometry, qvp::SliceOrientation::Axial),
               "L",
               "R",
               "P",
               "A");
}

void TestMprOrientationLabelMapper::lpsIdentitySagittal()
{
  const auto geometry = makeGeometry();

  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   geometry, qvp::SliceOrientation::Sagittal),
               "A",
               "P",
               "I",
               "S");
}

void TestMprOrientationLabelMapper::lpsIdentityCoronal()
{
  const auto geometry = makeGeometry();

  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   geometry, qvp::SliceOrientation::Coronal),
               "R",
               "L",
               "I",
               "S");
}

void TestMprOrientationLabelMapper::flippedXInLpsAxial()
{
  auto geometry = makeGeometry();
  geometry.direction = {-1.0, 0.0, 0.0,
                        0.0, 1.0, 0.0,
                        0.0, 0.0, 1.0};

  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   geometry, qvp::SliceOrientation::Axial),
               "L",
               "R",
               "A",
               "P");
}

void TestMprOrientationLabelMapper::flippedYInLpsAxial()
{
  auto geometry = makeGeometry();
  geometry.direction = {1.0, 0.0, 0.0,
                        0.0, -1.0, 0.0,
                        0.0, 0.0, 1.0};

  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   geometry, qvp::SliceOrientation::Axial),
               "R",
               "L",
               "P",
               "A");
}

void TestMprOrientationLabelMapper::flippedZInLpsSagittal()
{
  auto geometry = makeGeometry();
  geometry.direction = {1.0, 0.0, 0.0,
                        0.0, 1.0, 0.0,
                        0.0, 0.0, -1.0};

  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   geometry, qvp::SliceOrientation::Sagittal),
               "A",
               "P",
               "S",
               "I");
}

void TestMprOrientationLabelMapper::permutedAxes()
{
  auto geometry = makeGeometry();
  geometry.direction = {0.0, 1.0, 0.0,
                        1.0, 0.0, 0.0,
                        0.0, 0.0, 1.0};

  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   geometry, qvp::SliceOrientation::Axial),
               "A",
               "P",
               "R",
               "L");
}

void TestMprOrientationLabelMapper::ninetyDegreeRotation()
{
  auto geometry = makeGeometry();
  geometry.direction = {0.0, 1.0, 0.0,
                        -1.0, 0.0, 0.0,
                        0.0, 0.0, 1.0};

  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   geometry, qvp::SliceOrientation::Axial),
               "P",
               "A",
               "R",
               "L");
}

void TestMprOrientationLabelMapper::clearObliqueDirectionAccepted()
{
  auto geometry = makeGeometry();
  geometry.direction = {0.8, -0.6, 0.0,
                        0.6, 0.8, 0.0,
                        0.0, 0.0, 1.0};

  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   geometry, qvp::SliceOrientation::Axial),
               "R",
               "L",
               "A",
               "P");
}

void TestMprOrientationLabelMapper::ambiguousFortyFiveDegreeDirectionRejected()
{
  auto geometry = makeGeometry();
  geometry.direction = {0.70710678118, -0.70710678118, 0.0,
                        0.70710678118, 0.70710678118, 0.0,
                        0.0, 0.0, 1.0};

  QVERIFY(!qvp::MprOrientationLabelMapper::edgeLabels(
               geometry, qvp::SliceOrientation::Axial)
               .has_value());
}

void TestMprOrientationLabelMapper::degenerateAxisRejected()
{
  auto geometry = makeGeometry();
  geometry.direction = {0.0, 0.0, 0.0,
                        0.0, 1.0, 0.0,
                        0.0, 0.0, 1.0};

  QVERIFY(!qvp::MprOrientationLabelMapper::edgeLabels(
               geometry, qvp::SliceOrientation::Axial)
               .has_value());
}

void TestMprOrientationLabelMapper::nonFiniteDirectionRejected()
{
  auto geometry = makeGeometry();
  geometry.direction = {std::numeric_limits<double>::infinity(), 0.0, 0.0,
                        0.0, 1.0, 0.0,
                        0.0, 0.0, 1.0};

  QVERIFY(!qvp::MprOrientationLabelMapper::edgeLabels(
               geometry, qvp::SliceOrientation::Axial)
               .has_value());
}

void TestMprOrientationLabelMapper::duplicateDominantAxesRejected()
{
  auto geometry = makeGeometry();
  geometry.direction = {1.0, 0.9, 0.0,
                        0.0, 0.1, 0.0,
                        0.0, 0.0, 1.0};

  QVERIFY(!qvp::MprOrientationLabelMapper::edgeLabels(
               geometry, qvp::SliceOrientation::Axial)
               .has_value());
}

void TestMprOrientationLabelMapper::voxelAxisAnatomyLpsMapsAllOrientations()
{
  const auto anatomy = makeVoxelAxisAnatomy(qvp::AnatomicalDirection::Left,
                                            qvp::AnatomicalDirection::Posterior,
                                            qvp::AnatomicalDirection::Superior);

  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   anatomy, qvp::SliceOrientation::Axial),
               "R",
               "L",
               "A",
               "P");
  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   anatomy, qvp::SliceOrientation::Sagittal),
               "A",
               "P",
               "I",
               "S");
  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   anatomy, qvp::SliceOrientation::Coronal),
               "R",
               "L",
               "I",
               "S");
}

void TestMprOrientationLabelMapper::voxelAxisAnatomyRaiMapsAllOrientations()
{
  const auto anatomy = makeVoxelAxisAnatomy(qvp::AnatomicalDirection::Right,
                                            qvp::AnatomicalDirection::Anterior,
                                            qvp::AnatomicalDirection::Inferior);

  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   anatomy, qvp::SliceOrientation::Axial),
               "L",
               "R",
               "P",
               "A");
  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   anatomy, qvp::SliceOrientation::Sagittal),
               "P",
               "A",
               "S",
               "I");
  verifyLabels(qvp::MprOrientationLabelMapper::edgeLabels(
                   anatomy, qvp::SliceOrientation::Coronal),
               "L",
               "R",
               "S",
               "I");
}

void TestMprOrientationLabelMapper::voxelAxisAnatomyUnknownFailsClosed()
{
  const auto anatomy = makeVoxelAxisAnatomy(qvp::AnatomicalDirection::Unknown,
                                            qvp::AnatomicalDirection::Posterior,
                                            qvp::AnatomicalDirection::Superior);

  QVERIFY(!qvp::MprOrientationLabelMapper::edgeLabels(
               anatomy, qvp::SliceOrientation::Axial)
               .has_value());
}

QTEST_MAIN(TestMprOrientationLabelMapper)

#include "TestMprOrientationLabelMapper.moc"
