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
  void extractsOrthogonalSlicesFromUniqueVolume();
  void extractsDistinctCoronalAndSagittalSlices();
  void throwsForInvalidSliceIndex();
};

namespace
{

qvp::VolumeData makeVolume()
{
  return qvp::VolumeData(
      2, 3, 2, 0.5F, 0.75F, 1.25F,
      {0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F, 10.0F, 11.0F});
}

qvp::VolumeData makeUniqueVolume()
{
  return qvp::VolumeData(3,
                         2,
                         2,
                         0.5F,
                         0.75F,
                         1.25F,
                         {0.0F, 1.0F, 2.0F, 10.0F, 11.0F, 12.0F,
                          100.0F, 101.0F, 102.0F, 110.0F, 111.0F, 112.0F});
}

} // namespace

void TestSliceExtractor::extractsAxialSlice()
{
  const auto volume = makeVolume();
  const auto slice = qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Axial, 1);
  const std::vector<float> expected{6.0F, 7.0F, 8.0F, 9.0F, 10.0F, 11.0F};

  QCOMPARE(slice.width(), std::size_t{2});
  QCOMPARE(slice.height(), std::size_t{3});
  QCOMPARE(slice.spacingX(), 0.5F);
  QCOMPARE(slice.spacingY(), 0.75F);
  QCOMPARE(slice.orientation(), qvp::SliceOrientation::Axial);
  QCOMPARE(slice.sliceIndex(), std::size_t{1});
  QVERIFY(slice.pixels() == expected);
}

void TestSliceExtractor::extractsCoronalSlice()
{
  const auto volume = makeVolume();
  const auto slice = qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Coronal, 1);
  const std::vector<float> expected{2.0F, 3.0F, 8.0F, 9.0F};

  QCOMPARE(slice.width(), std::size_t{2});
  QCOMPARE(slice.height(), std::size_t{2});
  QCOMPARE(slice.spacingX(), 0.5F);
  QCOMPARE(slice.spacingY(), 1.25F);
  QCOMPARE(slice.orientation(), qvp::SliceOrientation::Coronal);
  QCOMPARE(slice.sliceIndex(), std::size_t{1});
  QVERIFY(slice.pixels() == expected);
}

void TestSliceExtractor::extractsSagittalSlice()
{
  const auto volume = makeVolume();
  const auto slice = qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Sagittal, 1);
  const std::vector<float> expected{1.0F, 3.0F, 5.0F, 7.0F, 9.0F, 11.0F};

  QCOMPARE(slice.width(), std::size_t{3});
  QCOMPARE(slice.height(), std::size_t{2});
  QCOMPARE(slice.spacingX(), 0.75F);
  QCOMPARE(slice.spacingY(), 1.25F);
  QCOMPARE(slice.orientation(), qvp::SliceOrientation::Sagittal);
  QCOMPARE(slice.sliceIndex(), std::size_t{1});
  QVERIFY(slice.pixels() == expected);
}

void TestSliceExtractor::extractsOrthogonalSlicesFromUniqueVolume()
{
  const auto volume = makeUniqueVolume();

  const auto axial = qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Axial, 1);
  const auto sagittal = qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Sagittal, 2);
  const auto coronal = qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Coronal, 1);

  QCOMPARE(axial.width(), std::size_t{3});
  QCOMPARE(axial.height(), std::size_t{2});
  QVERIFY(axial.pixels() == std::vector<float>({100.0F, 101.0F, 102.0F, 110.0F, 111.0F, 112.0F}));

  QCOMPARE(sagittal.width(), std::size_t{2});
  QCOMPARE(sagittal.height(), std::size_t{2});
  QVERIFY(sagittal.pixels() == std::vector<float>({2.0F, 12.0F, 102.0F, 112.0F}));

  QCOMPARE(coronal.width(), std::size_t{3});
  QCOMPARE(coronal.height(), std::size_t{2});
  QVERIFY(coronal.pixels() == std::vector<float>({10.0F, 11.0F, 12.0F, 110.0F, 111.0F, 112.0F}));
}

void TestSliceExtractor::extractsDistinctCoronalAndSagittalSlices()
{
  const auto volume = makeVolume();
  const auto coronal = qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Coronal, 1);
  const auto sagittal = qvp::SliceExtractor::extract(volume, qvp::SliceOrientation::Sagittal, 1);

  QCOMPARE(coronal.width(), std::size_t{2});
  QCOMPARE(coronal.height(), std::size_t{2});
  QCOMPARE(sagittal.width(), std::size_t{3});
  QCOMPARE(sagittal.height(), std::size_t{2});
  QVERIFY(coronal.pixels() != sagittal.pixels());
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
