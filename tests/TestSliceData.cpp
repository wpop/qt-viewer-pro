#include "qtviewerpro/core/SliceData.h"

#include <QtTest/QtTest>

#include <vector>

class TestSliceData : public QObject
{
  Q_OBJECT

private slots:
  void defaultConstructorCreatesEmptySlice();
  void constructorStoresDimensions();
  void constructorStoresSpacing();
  void constructorStoresOrientationAndSliceIndex();
  void constructorStoresPixelData();
};

void TestSliceData::defaultConstructorCreatesEmptySlice()
{
  const qvp::SliceData slice;

  QCOMPARE(slice.width(), std::size_t{0});
  QCOMPARE(slice.height(), std::size_t{0});
  QVERIFY(slice.pixels().empty());
}

void TestSliceData::constructorStoresDimensions()
{
  const qvp::SliceData slice(2, 3, 1.0F, 1.0F, qvp::SliceOrientation::Axial, 0, {});

  QCOMPARE(slice.width(), std::size_t{2});
  QCOMPARE(slice.height(), std::size_t{3});
}

void TestSliceData::constructorStoresSpacing()
{
  const qvp::SliceData slice(1, 1, 0.5F, 0.75F, qvp::SliceOrientation::Axial, 0, {});

  QCOMPARE(slice.spacingX(), 0.5F);
  QCOMPARE(slice.spacingY(), 0.75F);
}

void TestSliceData::constructorStoresOrientationAndSliceIndex()
{
  const qvp::SliceData slice(1, 1, 1.0F, 1.0F, qvp::SliceOrientation::Coronal, 4, {});

  QCOMPARE(slice.orientation(), qvp::SliceOrientation::Coronal);
  QCOMPARE(slice.sliceIndex(), std::size_t{4});
}

void TestSliceData::constructorStoresPixelData()
{
  const std::vector<float> pixels{1.0F, 2.0F, 3.0F, 4.0F};
  const qvp::SliceData slice(2, 2, 1.0F, 1.0F, qvp::SliceOrientation::Sagittal, 1, pixels);

  QVERIFY(slice.pixels() == pixels);
}

QTEST_MAIN(TestSliceData)

#include "TestSliceData.moc"
