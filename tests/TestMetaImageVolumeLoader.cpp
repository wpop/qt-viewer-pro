#include "qtviewerpro/io/MetaImageVolumeLoader.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itkImageRegionIterator.h>

#include <cmath>

namespace
{

using ImageType = itk::Image<float, 3>;

constexpr std::size_t kWidth = 4;
constexpr std::size_t kHeight = 3;
constexpr std::size_t kDepth = 2;
constexpr float kSpacingX = 1.5F;
constexpr float kSpacingY = 2.0F;
constexpr float kSpacingZ = 2.5F;

std::size_t linearIndex(std::size_t x, std::size_t y, std::size_t z)
{
  return x + (kWidth * (y + (kHeight * z)));
}

ImageType::Pointer createTinyVolume()
{
  auto image = ImageType::New();

  ImageType::IndexType start{};
  start.Fill(0);

  ImageType::SizeType size{};
  size[0] = kWidth;
  size[1] = kHeight;
  size[2] = kDepth;

  ImageType::RegionType region;
  region.SetIndex(start);
  region.SetSize(size);

  image->SetRegions(region);
  ImageType::SpacingType spacing;
  spacing[0] = kSpacingX;
  spacing[1] = kSpacingY;
  spacing[2] = kSpacingZ;
  image->SetSpacing(spacing);
  image->Allocate();

  itk::ImageRegionIterator<ImageType> it(image, region);
  for (it.GoToBegin(); !it.IsAtEnd(); ++it)
  {
    const auto index = it.GetIndex();
    const float value = static_cast<float>(index[0] + (10 * index[1]) + (100 * index[2]));
    it.Set(value);
  }

  return image;
}

void writeTinyMetaImageFile(const QString& path)
{
  const auto image = createTinyVolume();

  using WriterType = itk::ImageFileWriter<ImageType>;
  const auto writer = WriterType::New();
  writer->SetFileName(path.toStdString());
  writer->SetInput(image);
  writer->Update();
}

void verifyLoadedVolume(const qvp::VolumeData& volume)
{
  QCOMPARE(volume.width(), kWidth);
  QCOMPARE(volume.height(), kHeight);
  QCOMPARE(volume.depth(), kDepth);

  QVERIFY(std::fabs(volume.spacingX() - kSpacingX) < 1e-6F);
  QVERIFY(std::fabs(volume.spacingY() - kSpacingY) < 1e-6F);
  QVERIFY(std::fabs(volume.spacingZ() - kSpacingZ) < 1e-6F);

  QCOMPARE(volume.voxelCount(), kWidth * kHeight * kDepth);
  QCOMPARE(volume.voxels().size(), kWidth * kHeight * kDepth);

  const auto& voxels = volume.voxels();
  QCOMPARE(voxels[linearIndex(0, 0, 0)], 0.0F);
  QCOMPARE(voxels[linearIndex(2, 1, 0)], 12.0F);
  QCOMPARE(voxels[linearIndex(1, 1, 1)], 111.0F);
  QCOMPARE(voxels[linearIndex(3, 2, 1)], 123.0F);
}

} // namespace

class TestMetaImageVolumeLoader : public QObject
{
  Q_OBJECT

private slots:
  void loadsTinySyntheticVolume();
  void failsForMissingFile();
};

void TestMetaImageVolumeLoader::loadsTinySyntheticVolume()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path = directory.filePath("tiny.mha");
  writeTinyMetaImageFile(path);

  const qvp::MetaImageVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyLoadedVolume(result.volume);
}

void TestMetaImageVolumeLoader::failsForMissingFile()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const qvp::MetaImageVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(directory.filePath("missing.mha"));

  QVERIFY(!result.success);
  QVERIFY(!result.errorMessage.isEmpty());
}

QTEST_MAIN(TestMetaImageVolumeLoader)

#include "TestMetaImageVolumeLoader.moc"
