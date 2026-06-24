#include "qtviewerpro/core/SliceExtractor.h"

#include <QtTest/QtTest>

#include <stdexcept>
#include <vector>

class TestSliceExtractor : public QObject
{
  Q_OBJECT

private slots:
  void extractsAxialSlice();
  void extractsCoronalSlice();
  void extractsSagittalSlice();
  void throwsForInvalidSliceIndex();
};

namespace
{

qvp::VolumeData makeVolume()
{
  return qvp::VolumeData(
      2, 3, 2, 1.0F, 1.0F, 1.0F,
      {0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F, 10.0F, 11.0F});
}

} // namespace

void TestSliceExtractor::extractsAxialSlice()
{
  const auto volume = makeVolume();
  const auto slice = qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Axial, 1);
  const std::vector<float> expected{6.0F, 7.0F, 8.0F, 9.0F, 10.0F, 11.0F};

  QVERIFY(slice == expected);
}

void TestSliceExtractor::extractsCoronalSlice()
{
  const auto volume = makeVolume();
  const auto slice = qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Coronal, 1);
  const std::vector<float> expected{2.0F, 3.0F, 8.0F, 9.0F};

  QVERIFY(slice == expected);
}

void TestSliceExtractor::extractsSagittalSlice()
{
  const auto volume = makeVolume();
  const auto slice = qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Sagittal, 1);
  const std::vector<float> expected{1.0F, 3.0F, 5.0F, 7.0F, 9.0F, 11.0F};

  QVERIFY(slice == expected);
}

void TestSliceExtractor::throwsForInvalidSliceIndex()
{
  const auto volume = makeVolume();

  QVERIFY_EXCEPTION_THROWN(qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Axial, 2),
                           std::out_of_range);
  QVERIFY_EXCEPTION_THROWN(qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Coronal, 3),
                           std::out_of_range);
  QVERIFY_EXCEPTION_THROWN(qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Sagittal, 2),
                           std::out_of_range);
}

QTEST_MAIN(TestSliceExtractor)

#include "TestSliceExtractor.moc"
