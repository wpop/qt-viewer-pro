#include "qtviewerpro/processing/WindowLevelProcessor.h"

#include <QtTest/QtTest>

#include <cstdint>
#include <stdexcept>
#include <vector>

class TestWindowLevelProcessor : public QObject
{
  Q_OBJECT

private slots:
  void mapsLowerValueToZero();
  void mapsUpperValueToMax();
  void mapsLevelValueToMidpoint();
  void clampsValueBelowLowerToZero();
  void clampsValueAboveUpperToMax();
  void mapsSliceDataLikePixelBuffer();
  void throwsForNonPositiveWindow();
};

void TestWindowLevelProcessor::mapsLowerValueToZero()
{
  const auto output = qvp::WindowLevelProcessor::apply({0.0F}, 100.0F, 50.0F);

  QCOMPARE(output, std::vector<std::uint8_t>{0});
}

void TestWindowLevelProcessor::mapsUpperValueToMax()
{
  const auto output = qvp::WindowLevelProcessor::apply({100.0F}, 100.0F, 50.0F);

  QCOMPARE(output, std::vector<std::uint8_t>{255});
}

void TestWindowLevelProcessor::mapsLevelValueToMidpoint()
{
  const auto output = qvp::WindowLevelProcessor::apply({50.0F}, 100.0F, 50.0F);

  QCOMPARE(output.size(), std::size_t{1});
  QVERIFY(output.front() >= 127);
  QVERIFY(output.front() <= 128);
}

void TestWindowLevelProcessor::clampsValueBelowLowerToZero()
{
  const auto output = qvp::WindowLevelProcessor::apply({-10.0F}, 100.0F, 50.0F);

  QCOMPARE(output, std::vector<std::uint8_t>{0});
}

void TestWindowLevelProcessor::clampsValueAboveUpperToMax()
{
  const auto output = qvp::WindowLevelProcessor::apply({110.0F}, 100.0F, 50.0F);

  QCOMPARE(output, std::vector<std::uint8_t>{255});
}

void TestWindowLevelProcessor::mapsSliceDataLikePixelBuffer()
{
  const std::vector<float> pixels{0.0F, 50.0F, 100.0F};
  const qvp::SliceData slice(3, 1, 1.0F, 1.0F, qvp::SliceOrientation::Axial, 0, pixels);

  const auto fromPixels = qvp::WindowLevelProcessor::apply(pixels, 100.0F, 50.0F);
  const auto fromSlice = qvp::WindowLevelProcessor::apply(slice, 100.0F, 50.0F);

  QCOMPARE(fromSlice, fromPixels);
}

void TestWindowLevelProcessor::throwsForNonPositiveWindow()
{
  QVERIFY_EXCEPTION_THROWN(qvp::WindowLevelProcessor::apply({50.0F}, 0.0F, 50.0F),
                           std::invalid_argument);
  QVERIFY_EXCEPTION_THROWN(qvp::WindowLevelProcessor::apply({50.0F}, -1.0F, 50.0F),
                           std::invalid_argument);
}

QTEST_MAIN(TestWindowLevelProcessor)

#include "TestWindowLevelProcessor.moc"
