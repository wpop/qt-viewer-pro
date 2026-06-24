#include "qtviewerpro/processing/SliceImageConverter.h"

#include <QImage>
#include <QtTest/QtTest>

#include <stdexcept>
#include <vector>

class TestSliceImageConverter : public QObject
{
  Q_OBJECT

private slots:
  void convertsValidSliceDataToQImage();
  void throwsForInvalidPixelCount();
};

void TestSliceImageConverter::convertsValidSliceDataToQImage()
{
  const qvp::SliceData slice(
      3, 2, 1.0F, 1.0F, qvp::SliceOrientation::Axial, 0, {0.0F, 50.0F, 100.0F, 25.0F, 75.0F, 125.0F});

  const QImage image = qvp::SliceImageConverter::toGrayscaleImage(slice, 100.0F, 50.0F);

  QCOMPARE(image.width(), 3);
  QCOMPARE(image.height(), 2);
  QCOMPARE(image.format(), QImage::Format_Grayscale8);
  QCOMPARE(qGray(image.pixel(0, 0)), 0);
  QVERIFY(qGray(image.pixel(1, 0)) >= 127);
  QVERIFY(qGray(image.pixel(1, 0)) <= 128);
  QCOMPARE(qGray(image.pixel(2, 0)), 255);
  QCOMPARE(qGray(image.pixel(0, 1)), 64);
  QCOMPARE(qGray(image.pixel(1, 1)), 191);
  QCOMPARE(qGray(image.pixel(2, 1)), 255);
}

void TestSliceImageConverter::throwsForInvalidPixelCount()
{
  const qvp::SliceData slice(2, 2, 1.0F, 1.0F, qvp::SliceOrientation::Axial, 0, {0.0F, 50.0F, 100.0F});

  QVERIFY_EXCEPTION_THROWN(qvp::SliceImageConverter::toGrayscaleImage(slice, 100.0F, 50.0F),
                           std::invalid_argument);
}

QTEST_GUILESS_MAIN(TestSliceImageConverter)

#include "TestSliceImageConverter.moc"
