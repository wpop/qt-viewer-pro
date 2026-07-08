#include "qtviewerpro/io/NiftiVolumeLoader.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itkImageRegionIterator.h>
#include <nifti1.h>

#include <cmath>
#include <cstddef>
#include <fstream>
#include <stdexcept>

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
const qvp::VolumeData::Direction kIdentityDirection{1.0, 0.0, 0.0,
                                                    0.0, 1.0, 0.0,
                                                    0.0, 0.0, 1.0};

std::size_t linearIndex(std::size_t x, std::size_t y, std::size_t z)
{
  return x + (kWidth * (y + (kHeight * z)));
}

ImageType::Pointer createTinyVolume(const qvp::VolumeData::Direction& directionValues = kDirection)
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
  direction[0][0] = directionValues[0];
  direction[0][1] = directionValues[1];
  direction[0][2] = directionValues[2];
  direction[1][0] = directionValues[3];
  direction[1][1] = directionValues[4];
  direction[1][2] = directionValues[5];
  direction[2][0] = directionValues[6];
  direction[2][1] = directionValues[7];
  direction[2][2] = directionValues[8];
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

void writeTinyNiftiFile(const QString& path,
                        const qvp::VolumeData::Direction& directionValues = kDirection)
{
  const auto image = createTinyVolume(directionValues);

  using WriterType = itk::ImageFileWriter<ImageType>;
  const auto writer = WriterType::New();
  writer->SetFileName(path.toStdString());
  writer->SetInput(image);
  writer->Update();
}

void clearNiftiOrientationCodes(const QString& path)
{
  std::fstream file(path.toStdString(), std::ios::binary | std::ios::in | std::ios::out);
  if (!file)
  {
    throw std::runtime_error("Failed to open NIfTI file for orientation patching");
  }

  const short zero = 0;
  file.seekp(static_cast<std::streamoff>(offsetof(nifti_1_header, qform_code)));
  file.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
  file.seekp(static_cast<std::streamoff>(offsetof(nifti_1_header, sform_code)));
  file.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
  if (!file)
  {
    throw std::runtime_error("Failed to patch NIfTI orientation codes");
  }
}

void verifyCommonLoadedVolume(const qvp::VolumeData& volume)
{
  QCOMPARE(volume.width(), kWidth);
  QCOMPARE(volume.height(), kHeight);
  QCOMPARE(volume.depth(), kDepth);

  QVERIFY(std::fabs(volume.spacingX() - kSpacingX) < 1e-6F);
  QVERIFY(std::fabs(volume.spacingY() - kSpacingY) < 1e-6F);
  QVERIFY(std::fabs(volume.spacingZ() - kSpacingZ) < 1e-6F);
  QVERIFY(volume.spatialGeometry().coordinateSystem ==
          qvp::VolumeData::CoordinateSystem::LPS);

  QCOMPARE(volume.voxelCount(), kWidth * kHeight * kDepth);
  QCOMPARE(volume.voxels().size(), kWidth * kHeight * kDepth);

  const auto& voxels = volume.voxels();
  QCOMPARE(voxels[linearIndex(0, 0, 0)], 0.0F);
  QCOMPARE(voxels[linearIndex(3, 2, 0)], 23.0F);
  QCOMPARE(voxels[linearIndex(1, 1, 1)], 111.0F);
  QCOMPARE(voxels[linearIndex(3, 2, 1)], 123.0F);
}

void verifyTrustedGeometry(const qvp::VolumeData& volume,
                           const qvp::VolumeData::Origin& expectedOrigin,
                           const qvp::VolumeData::Direction& expectedDirection)
{
  QVERIFY(volume.hasSpatialOrientation());
  for (std::size_t i = 0; i < expectedOrigin.size(); ++i)
  {
    QVERIFY(std::fabs(volume.spatialGeometry().origin[i] - expectedOrigin[i]) < 1e-6);
  }
  for (std::size_t i = 0; i < expectedDirection.size(); ++i)
  {
    QVERIFY(std::fabs(volume.spatialGeometry().direction[i] - expectedDirection[i]) < 1e-6);
  }
}

} // namespace

class TestNiftiVolumeLoader : public QObject
{
  Q_OBJECT

private slots:
  void trustedNiftiOrientation();
  void untrustedNiftiOrientation();
  void explicitlyAuthoredIdentityOrientationIsTrusted();
  void failsForMissingFile();
};

void TestNiftiVolumeLoader::trustedNiftiOrientation()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path = directory.filePath("tiny.nii.gz");
  writeTinyNiftiFile(path);

  const qvp::NiftiVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyCommonLoadedVolume(result.volume);
  verifyTrustedGeometry(result.volume, kOrigin, kDirection);
}

void TestNiftiVolumeLoader::untrustedNiftiOrientation()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path = directory.filePath("tiny-untrusted.nii");
  writeTinyNiftiFile(path);
  clearNiftiOrientationCodes(path);

  const qvp::NiftiVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyCommonLoadedVolume(result.volume);
  QVERIFY(!result.volume.hasSpatialOrientation());
}

void TestNiftiVolumeLoader::explicitlyAuthoredIdentityOrientationIsTrusted()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path = directory.filePath("tiny-identity.nii");
  writeTinyNiftiFile(path, kIdentityDirection);

  const qvp::NiftiVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyCommonLoadedVolume(result.volume);
  verifyTrustedGeometry(result.volume, kOrigin, kIdentityDirection);
}

void TestNiftiVolumeLoader::failsForMissingFile()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const qvp::NiftiVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(directory.filePath("missing.nii.gz"));

  QVERIFY(!result.success);
  QVERIFY(!result.errorMessage.isEmpty());
}

QTEST_MAIN(TestNiftiVolumeLoader)

#include "TestNiftiVolumeLoader.moc"
