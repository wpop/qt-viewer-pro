#include "qtviewerpro/processing/VolumeResampler.h"

#include <QtTest/QtTest>

#include <cmath>
#include <cstddef>
#include <vector>

class TestVolumeResampler : public QObject
{
  Q_OBJECT

private slots:
  void resamplesAnisotropicVolumeToIsotropicSpacing();
  void leavesAlreadyIsotropicVolumeUnchanged();
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

  for (const float value : outputVolume.voxels())
  {
    QVERIFY(std::isfinite(value));
    QVERIFY(std::fabs(value - 7.25F) <= 1e-5F);
  }
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
}

QTEST_GUILESS_MAIN(TestVolumeResampler)

#include "TestVolumeResampler.moc"
