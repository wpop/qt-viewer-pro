#include "qtviewerpro/processing/VolumeResampler.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace
{

bool nearlyEqual(double lhs, double rhs, double tolerance = 1e-9)
{
  return std::fabs(lhs - rhs) <= tolerance;
}

void verifyOriginEquals(const qvp::VolumeData::Origin& actual, const qvp::VolumeData::Origin& expected)
{
  for (std::size_t i = 0; i < actual.size(); ++i)
  {
    QVERIFY(nearlyEqual(actual[i], expected[i]));
  }
}

void verifyDirectionEquals(const qvp::VolumeData::Direction& actual,
                           const qvp::VolumeData::Direction& expected)
{
  for (std::size_t i = 0; i < actual.size(); ++i)
  {
    QVERIFY(nearlyEqual(actual[i], expected[i]));
  }
}

qvp::VolumeData::SpatialGeometry makeTestGeometry(qvp::VolumeData::CoordinateSystem coordinateSystem,
                                                  bool hasOrientation)
{
  qvp::VolumeData::SpatialGeometry geometry;
  geometry.origin = {12.5, -8.25, 42.0};
  geometry.direction = {0.0, -1.0, 0.0,
                        1.0, 0.0, 0.0,
                        0.0, 0.0, 1.0};
  geometry.coordinateSystem = coordinateSystem;
  geometry.hasOrientation = hasOrientation;
  return geometry;
}

} // namespace

class TestVolumeResampler : public QObject
{
  Q_OBJECT

private slots:
  void resamplesAnisotropicVolumeToIsotropicSpacing();
  void leavesAlreadyIsotropicVolumeUnchanged();
  void preservesExplicitOriginThroughResampling();
  void preservesNonIdentityDirectionThroughResampling();
  void preservesCoordinateSystemThroughResampling();
  void preservesTrustedOrientationFlagThroughResampling();
  void preservesUntrustedOrientationFlagThroughResampling();
};

void TestVolumeResampler::resamplesAnisotropicVolumeToIsotropicSpacing()
{
  const std::vector<float> voxels(3 * 4 * 5, 7.25F);
  const qvp::VolumeData inputVolume(3, 4, 5, 2.0F, 3.0F, 4.0F, voxels);

  const qvp::VolumeData outputVolume =
      qvp::VolumeResampler::resampleToIsotropicSpacing(inputVolume);

  QVERIFY(outputVolume.isValid());
  QCOMPARE(outputVolume.spacingX(), 1.0F);
  QCOMPARE(outputVolume.spacingY(), 1.0F);
  QCOMPARE(outputVolume.spacingZ(), 1.0F);
  QCOMPARE(outputVolume.width(), std::size_t{5});
  QCOMPARE(outputVolume.height(), std::size_t{10});
  QCOMPARE(outputVolume.depth(), std::size_t{17});
  QCOMPARE(outputVolume.voxels().size(), outputVolume.voxelCount());
  QVERIFY(outputVolume.hasIntensityRange());
  QVERIFY(std::isfinite(outputVolume.intensityMinimum()));
  QVERIFY(std::isfinite(outputVolume.intensityMaximum()));
  QVERIFY(outputVolume.intensityMinimum() <= outputVolume.intensityMaximum());
  QCOMPARE(outputVolume.intensityMinimum(), 7.25F);
  QCOMPARE(outputVolume.intensityMaximum(), 7.25F);

  for (const float value : outputVolume.voxels())
  {
    QVERIFY(std::isfinite(value));
    QVERIFY(std::fabs(value - 7.25F) <= 1e-5F);
  }

  const auto [minIt, maxIt] =
      std::minmax_element(outputVolume.voxels().begin(), outputVolume.voxels().end());
  QCOMPARE(outputVolume.intensityMinimum(), *minIt);
  QCOMPARE(outputVolume.intensityMaximum(), *maxIt);
}

void TestVolumeResampler::leavesAlreadyIsotropicVolumeUnchanged()
{
  const std::vector<float> voxels{0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F};
  const qvp::VolumeData inputVolume(2, 2, 2, 1.0F, 1.0F, 1.0F, voxels);

  const qvp::VolumeData outputVolume =
      qvp::VolumeResampler::resampleToIsotropicSpacing(inputVolume);

  QCOMPARE(outputVolume.width(), std::size_t{2});
  QCOMPARE(outputVolume.height(), std::size_t{2});
  QCOMPARE(outputVolume.depth(), std::size_t{2});
  QCOMPARE(outputVolume.spacingX(), 1.0F);
  QCOMPARE(outputVolume.spacingY(), 1.0F);
  QCOMPARE(outputVolume.spacingZ(), 1.0F);
  QVERIFY(outputVolume.voxels() == voxels);
  QVERIFY(outputVolume.hasIntensityRange());
  QVERIFY(std::isfinite(outputVolume.intensityMinimum()));
  QVERIFY(std::isfinite(outputVolume.intensityMaximum()));
  QVERIFY(outputVolume.intensityMinimum() <= outputVolume.intensityMaximum());

  const auto [minIt, maxIt] = std::minmax_element(voxels.begin(), voxels.end());
  QCOMPARE(outputVolume.intensityMinimum(), *minIt);
  QCOMPARE(outputVolume.intensityMaximum(), *maxIt);
}

void TestVolumeResampler::preservesExplicitOriginThroughResampling()
{
  const std::vector<float> voxels(3 * 4 * 5, 7.25F);
  const qvp::VolumeData::SpatialGeometry geometry =
      makeTestGeometry(qvp::VolumeData::CoordinateSystem::LPS, true);
  const qvp::VolumeData inputVolume(3, 4, 5, 2.0F, 3.0F, 4.0F, voxels, geometry);

  const qvp::VolumeData outputVolume =
      qvp::VolumeResampler::resampleToIsotropicSpacing(inputVolume);

  verifyOriginEquals(outputVolume.spatialGeometry().origin, geometry.origin);
}

void TestVolumeResampler::preservesNonIdentityDirectionThroughResampling()
{
  const std::vector<float> voxels(3 * 4 * 5, 7.25F);
  const qvp::VolumeData::SpatialGeometry geometry =
      makeTestGeometry(qvp::VolumeData::CoordinateSystem::LPS, true);
  const qvp::VolumeData inputVolume(3, 4, 5, 2.0F, 3.0F, 4.0F, voxels, geometry);

  const qvp::VolumeData outputVolume =
      qvp::VolumeResampler::resampleToIsotropicSpacing(inputVolume);

  verifyDirectionEquals(outputVolume.spatialGeometry().direction, geometry.direction);
}

void TestVolumeResampler::preservesCoordinateSystemThroughResampling()
{
  const std::vector<float> voxels(3 * 4 * 5, 7.25F);
  const qvp::VolumeData::SpatialGeometry geometry =
      makeTestGeometry(qvp::VolumeData::CoordinateSystem::LPS, true);
  const qvp::VolumeData inputVolume(3, 4, 5, 2.0F, 3.0F, 4.0F, voxels, geometry);

  const qvp::VolumeData outputVolume =
      qvp::VolumeResampler::resampleToIsotropicSpacing(inputVolume);

  QCOMPARE(outputVolume.spatialGeometry().coordinateSystem,
           qvp::VolumeData::CoordinateSystem::LPS);
}

void TestVolumeResampler::preservesTrustedOrientationFlagThroughResampling()
{
  const std::vector<float> voxels(3 * 4 * 5, 7.25F);
  const qvp::VolumeData::SpatialGeometry geometry =
      makeTestGeometry(qvp::VolumeData::CoordinateSystem::LPS, true);
  const qvp::VolumeData inputVolume(3, 4, 5, 2.0F, 3.0F, 4.0F, voxels, geometry);

  const qvp::VolumeData outputVolume =
      qvp::VolumeResampler::resampleToIsotropicSpacing(inputVolume);

  QVERIFY(outputVolume.hasSpatialOrientation());
}

void TestVolumeResampler::preservesUntrustedOrientationFlagThroughResampling()
{
  const std::vector<float> voxels(3 * 4 * 5, 7.25F);
  const qvp::VolumeData::SpatialGeometry geometry =
      makeTestGeometry(qvp::VolumeData::CoordinateSystem::LPS, false);
  const qvp::VolumeData inputVolume(3, 4, 5, 2.0F, 3.0F, 4.0F, voxels, geometry);

  const qvp::VolumeData outputVolume =
      qvp::VolumeResampler::resampleToIsotropicSpacing(inputVolume);

  QVERIFY(!outputVolume.hasSpatialOrientation());
}

QTEST_GUILESS_MAIN(TestVolumeResampler)

#include "TestVolumeResampler.moc"
