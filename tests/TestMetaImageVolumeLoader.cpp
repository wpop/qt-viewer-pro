#include "qtviewerpro/io/MetaImageVolumeLoader.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itkImageRegionIterator.h>

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace
{

using ImageType = itk::Image<float, 3>;

constexpr std::size_t kWidth = 4;
constexpr std::size_t kHeight = 3;
constexpr std::size_t kDepth = 2;
constexpr float kSpacingX = 1.5F;
constexpr float kSpacingY = 2.0F;
constexpr float kSpacingZ = 2.5F;
const qvp::VolumeData::Origin kOrigin{12.5, -8.25, 42.0};
const qvp::VolumeData::Direction kDirection{0.0, -1.0, 0.0,
                                            1.0, 0.0, 0.0,
                                            0.0, 0.0, 1.0};

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
  ImageType::PointType origin;
  origin[0] = kOrigin[0];
  origin[1] = kOrigin[1];
  origin[2] = kOrigin[2];
  image->SetOrigin(origin);
  ImageType::DirectionType direction;
  direction[0][0] = kDirection[0];
  direction[0][1] = kDirection[1];
  direction[0][2] = kDirection[2];
  direction[1][0] = kDirection[3];
  direction[1][1] = kDirection[4];
  direction[1][2] = kDirection[5];
  direction[2][0] = kDirection[6];
  direction[2][1] = kDirection[7];
  direction[2][2] = kDirection[8];
  image->SetDirection(direction);
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

void writeRawVoxelFile(const QString& path, const std::vector<float>& voxels)
{
  std::ofstream file(path.toStdString(), std::ios::binary);
  if (!file)
  {
    throw std::runtime_error("Failed to open MetaImage raw file for writing");
  }

  file.write(reinterpret_cast<const char*>(voxels.data()),
             static_cast<std::streamsize>(voxels.size() * sizeof(float)));
  if (!file)
  {
    throw std::runtime_error("Failed to write MetaImage raw voxels");
  }
}

QString writeDetachedMetaImageFixture(const QTemporaryDir& directory,
                                      const QString& baseName,
                                      const QStringList& extraFields)
{
  std::vector<float> voxels;
  voxels.reserve(kWidth * kHeight * kDepth);
  for (std::size_t z = 0; z < kDepth; ++z)
  {
    for (std::size_t y = 0; y < kHeight; ++y)
    {
      for (std::size_t x = 0; x < kWidth; ++x)
      {
        voxels.push_back(static_cast<float>(x + (10 * y) + (100 * z)));
      }
    }
  }
  const QString rawPath = directory.filePath(baseName + ".raw");
  writeRawVoxelFile(rawPath, voxels);

  const QString headerPath = directory.filePath(baseName + ".mhd");
  std::ofstream file(headerPath.toStdString());
  if (!file)
  {
    throw std::runtime_error("Failed to open MetaImage header for writing");
  }

  file << "ObjectType = Image\n";
  file << "NDims = 3\n";
  file << "BinaryData = True\n";
  file << "BinaryDataByteOrderMSB = False\n";
  file << "CompressedData = False\n";
  file << "TransformMatrix = 0 1 0 -1 0 0 0 0 1\n";
  file << "Offset = 12.5 -8.25 42\n";
  file << "ElementSpacing = 1.5 2 2.5\n";
  file << "DimSize = 4 3 2\n";
  file << "ElementType = MET_FLOAT\n";
  for (const QString& field : extraFields)
  {
    file << field.toStdString() << '\n';
  }
  file << "ElementDataFile = " << QFileInfo(rawPath).fileName().toStdString() << '\n';
  if (!file)
  {
    throw std::runtime_error("Failed to write MetaImage header");
  }

  return headerPath;
}

void verifyLoadedVolume(const qvp::VolumeData& volume)
{
  QCOMPARE(volume.width(), kWidth);
  QCOMPARE(volume.height(), kHeight);
  QCOMPARE(volume.depth(), kDepth);

  QVERIFY(std::fabs(volume.spacingX() - kSpacingX) < 1e-6F);
  QVERIFY(std::fabs(volume.spacingY() - kSpacingY) < 1e-6F);
  QVERIFY(std::fabs(volume.spacingZ() - kSpacingZ) < 1e-6F);
  QVERIFY(!volume.hasSpatialOrientation());
  QVERIFY(volume.spatialGeometry().coordinateSystem ==
          qvp::VolumeData::CoordinateSystem::LPS);
  for (std::size_t i = 0; i < kOrigin.size(); ++i)
  {
    QVERIFY(std::fabs(volume.spatialGeometry().origin[i] - kOrigin[i]) < 1e-6);
  }
  for (std::size_t i = 0; i < kDirection.size(); ++i)
  {
    QVERIFY(std::fabs(volume.spatialGeometry().direction[i] - kDirection[i]) < 1e-6);
  }

  QCOMPARE(volume.voxelCount(), kWidth * kHeight * kDepth);
  QCOMPARE(volume.voxels().size(), kWidth * kHeight * kDepth);

  const auto& voxels = volume.voxels();
  QCOMPARE(voxels[linearIndex(0, 0, 0)], 0.0F);
  QCOMPARE(voxels[linearIndex(2, 1, 0)], 12.0F);
  QCOMPARE(voxels[linearIndex(1, 1, 1)], 111.0F);
  QCOMPARE(voxels[linearIndex(3, 2, 1)], 123.0F);
}

void verifyNoVoxelAxisAnatomy(const qvp::VolumeData& volume)
{
  QVERIFY(!volume.hasVoxelAxisAnatomy());
  QVERIFY(!volume.voxelAxisAnatomy().has_value());
}

} // namespace

class TestMetaImageVolumeLoader : public QObject
{
  Q_OBJECT

private slots:
  void loadsTinySyntheticVolume();
  void loadsLpsAnatomicalOrientationAsVoxelAxisMetadata();
  void loadsRaiAnatomicalOrientationAsVoxelAxisMetadata();
  void missingAnatomicalOrientationLeavesVoxelAxisMetadataUnset();
  void invalidAnatomicalOrientationLeavesVoxelAxisMetadataUnset();
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
  QVERIFY(!result.volume.hasSpatialOrientation());
}

void TestMetaImageVolumeLoader::loadsLpsAnatomicalOrientationAsVoxelAxisMetadata()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path =
      writeDetachedMetaImageFixture(directory, "lps-anatomy", {"AnatomicalOrientation = LPS"});

  const qvp::MetaImageVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyLoadedVolume(result.volume);
  QVERIFY(!result.volume.hasSpatialOrientation());
  QVERIFY(result.volume.hasVoxelAxisAnatomy());
  QVERIFY(result.volume.voxelAxisAnatomy().has_value());
  QCOMPARE(result.volume.voxelAxisAnatomy()->x, qvp::AnatomicalDirection::Left);
  QCOMPARE(result.volume.voxelAxisAnatomy()->y, qvp::AnatomicalDirection::Posterior);
  QCOMPARE(result.volume.voxelAxisAnatomy()->z, qvp::AnatomicalDirection::Superior);
}

void TestMetaImageVolumeLoader::loadsRaiAnatomicalOrientationAsVoxelAxisMetadata()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path =
      writeDetachedMetaImageFixture(directory, "rai-anatomy", {"AnatomicalOrientation = RAI"});

  const qvp::MetaImageVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyLoadedVolume(result.volume);
  QVERIFY(!result.volume.hasSpatialOrientation());
  QVERIFY(result.volume.hasVoxelAxisAnatomy());
  QVERIFY(result.volume.voxelAxisAnatomy().has_value());
  QCOMPARE(result.volume.voxelAxisAnatomy()->x, qvp::AnatomicalDirection::Right);
  QCOMPARE(result.volume.voxelAxisAnatomy()->y, qvp::AnatomicalDirection::Anterior);
  QCOMPARE(result.volume.voxelAxisAnatomy()->z, qvp::AnatomicalDirection::Inferior);
}

void TestMetaImageVolumeLoader::missingAnatomicalOrientationLeavesVoxelAxisMetadataUnset()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path = writeDetachedMetaImageFixture(directory, "missing-anatomy", {});

  const qvp::MetaImageVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyLoadedVolume(result.volume);
  QVERIFY(!result.volume.hasSpatialOrientation());
  verifyNoVoxelAxisAnatomy(result.volume);
}

void TestMetaImageVolumeLoader::invalidAnatomicalOrientationLeavesVoxelAxisMetadataUnset()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path =
      writeDetachedMetaImageFixture(directory, "invalid-anatomy", {"AnatomicalOrientation = LLS"});

  const qvp::MetaImageVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyLoadedVolume(result.volume);
  QVERIFY(!result.volume.hasSpatialOrientation());
  verifyNoVoxelAxisAnatomy(result.volume);
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
