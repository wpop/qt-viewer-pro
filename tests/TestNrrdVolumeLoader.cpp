#include "qtviewerpro/io/NrrdVolumeLoader.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <cmath>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

constexpr std::size_t kWidth = 2;
constexpr std::size_t kHeight = 2;
constexpr std::size_t kDepth = 2;
constexpr float kSpacingX = 1.5F;
constexpr float kSpacingY = 2.0F;
constexpr float kSpacingZ = 2.5F;
const qvp::VolumeData::Origin kLpsOrigin{10.0, 20.0, 30.0};
const qvp::VolumeData::Direction kIdentityDirection{1.0, 0.0, 0.0,
                                                    0.0, 1.0, 0.0,
                                                    0.0, 0.0, 1.0};
const qvp::VolumeData::Origin kRasConvertedOrigin{-10.0, -20.0, 30.0};
const qvp::VolumeData::Direction kRasConvertedDirection{-1.0, 0.0, 0.0,
                                                        0.0, -1.0, 0.0,
                                                        0.0, 0.0, 1.0};
const std::vector<float> kVoxelValues{0.0F, 1.0F, 10.0F, 11.0F,
                                      100.0F, 101.0F, 110.0F, 111.0F};

std::size_t linearIndex(std::size_t x, std::size_t y, std::size_t z)
{
  return x + (kWidth * (y + (kHeight * z)));
}

void writeRawVoxelFile(const QString& path, const std::vector<float>& voxels)
{
  std::ofstream file(path.toStdString(), std::ios::binary);
  if (!file)
  {
    throw std::runtime_error("Failed to open NRRD raw file for writing");
  }

  file.write(reinterpret_cast<const char*>(voxels.data()),
             static_cast<std::streamsize>(voxels.size() * sizeof(float)));
  if (!file)
  {
    throw std::runtime_error("Failed to write NRRD raw voxels");
  }
}

void writeNrrdHeader(const QString& path,
                     const QString& rawFileName,
                     const QStringList& extraFields)
{
  std::ofstream file(path.toStdString());
  if (!file)
  {
    throw std::runtime_error("Failed to open NRRD header for writing");
  }

  file << "NRRD0005\n";
  file << "type: float\n";
  file << "dimension: 3\n";
  file << "sizes: 2 2 2\n";
  file << "encoding: raw\n";
  file << "endian: little\n";
  for (const QString& field : extraFields)
  {
    file << field.toStdString() << '\n';
  }
  file << "data file: " << rawFileName.toStdString() << '\n';
  if (!file)
  {
    throw std::runtime_error("Failed to write NRRD header");
  }
}

QString writeDetachedNrrdFixture(const QTemporaryDir& directory,
                                 const QString& baseName,
                                 const QStringList& extraFields)
{
  const QString rawPath = directory.filePath(baseName + ".raw");
  writeRawVoxelFile(rawPath, kVoxelValues);

  const QString headerPath = directory.filePath(baseName + ".nhdr");
  writeNrrdHeader(headerPath, QFileInfo(rawPath).fileName(), extraFields);
  return headerPath;
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

  QCOMPARE(volume.voxelCount(), kVoxelValues.size());
  QCOMPARE(volume.voxels().size(), kVoxelValues.size());

  const auto& voxels = volume.voxels();
  QCOMPARE(voxels[linearIndex(0, 0, 0)], 0.0F);
  QCOMPARE(voxels[linearIndex(1, 0, 0)], 1.0F);
  QCOMPARE(voxels[linearIndex(0, 1, 0)], 10.0F);
  QCOMPARE(voxels[linearIndex(1, 1, 0)], 11.0F);
  QCOMPARE(voxels[linearIndex(0, 0, 1)], 100.0F);
  QCOMPARE(voxels[linearIndex(1, 0, 1)], 101.0F);
  QCOMPARE(voxels[linearIndex(0, 1, 1)], 110.0F);
  QCOMPARE(voxels[linearIndex(1, 1, 1)], 111.0F);
}

void verifySpatialGeometry(const qvp::VolumeData& volume,
                           const qvp::VolumeData::Origin& expectedOrigin,
                           const qvp::VolumeData::Direction& expectedDirection)
{
  for (std::size_t i = 0; i < expectedOrigin.size(); ++i)
  {
    QVERIFY(std::fabs(volume.spatialGeometry().origin[i] - expectedOrigin[i]) < 1e-6);
  }
  for (std::size_t i = 0; i < expectedDirection.size(); ++i)
  {
    QVERIFY(std::fabs(volume.spatialGeometry().direction[i] - expectedDirection[i]) < 1e-6);
  }
}

QStringList anatomicalLpsFields()
{
  return {
      "space: left-posterior-superior",
      "space directions: (1.5,0,0) (0,2,0) (0,0,2.5)",
      "space origin: (10,20,30)",
  };
}

} // namespace

class TestNrrdVolumeLoader : public QObject
{
  Q_OBJECT

private slots:
  void trustedLpsNrrdOrientation();
  void trustedRasNrrdIsConvertedToLps();
  void explicitlyAuthoredIdentityOrientationIsTrusted();
  void scannerXyzIsUntrusted();
  void genericHandedSpaceIsUntrusted();
  void missingAnatomicalSpaceIsUntrusted();
  void failsForMissingFile();
};

void TestNrrdVolumeLoader::trustedLpsNrrdOrientation()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path = writeDetachedNrrdFixture(directory, "trusted-lps", anatomicalLpsFields());

  const qvp::NrrdVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyCommonLoadedVolume(result.volume);
  QVERIFY(result.volume.hasSpatialOrientation());
  verifySpatialGeometry(result.volume, kLpsOrigin, kIdentityDirection);
}

void TestNrrdVolumeLoader::trustedRasNrrdIsConvertedToLps()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path = writeDetachedNrrdFixture(
      directory,
      "trusted-ras",
      {
          "space: right-anterior-superior",
          "space directions: (1.5,0,0) (0,2,0) (0,0,2.5)",
          "space origin: (10,20,30)",
      });

  const qvp::NrrdVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyCommonLoadedVolume(result.volume);
  QVERIFY(result.volume.hasSpatialOrientation());
  verifySpatialGeometry(result.volume, kRasConvertedOrigin, kRasConvertedDirection);
}

void TestNrrdVolumeLoader::explicitlyAuthoredIdentityOrientationIsTrusted()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path = writeDetachedNrrdFixture(directory, "identity-lps", anatomicalLpsFields());

  const qvp::NrrdVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyCommonLoadedVolume(result.volume);
  QVERIFY(result.volume.hasSpatialOrientation());
  QVERIFY(result.volume.spatialGeometry().direction == kIdentityDirection);
}

void TestNrrdVolumeLoader::scannerXyzIsUntrusted()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path = writeDetachedNrrdFixture(
      directory,
      "scanner-xyz",
      {
          "space: scanner-xyz",
          "space directions: (1.5,0,0) (0,2,0) (0,0,2.5)",
          "space origin: (10,20,30)",
      });

  const qvp::NrrdVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyCommonLoadedVolume(result.volume);
  QVERIFY(!result.volume.hasSpatialOrientation());
}

void TestNrrdVolumeLoader::genericHandedSpaceIsUntrusted()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path = writeDetachedNrrdFixture(
      directory,
      "right-handed",
      {
          "space: 3D-right-handed",
          "space directions: (1.5,0,0) (0,2,0) (0,0,2.5)",
          "space origin: (10,20,30)",
      });

  const qvp::NrrdVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyCommonLoadedVolume(result.volume);
  QVERIFY(!result.volume.hasSpatialOrientation());
}

void TestNrrdVolumeLoader::missingAnatomicalSpaceIsUntrusted()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString path = writeDetachedNrrdFixture(
      directory,
      "missing-space",
      {
          "space dimension: 3",
          "space directions: (1.5,0,0) (0,2,0) (0,0,2.5)",
          "space origin: (10,20,30)",
      });

  const qvp::NrrdVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(path);

  QVERIFY(result.success);
  QVERIFY(result.volume.isValid());
  verifyCommonLoadedVolume(result.volume);
  QVERIFY(!result.volume.hasSpatialOrientation());
}

void TestNrrdVolumeLoader::failsForMissingFile()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const qvp::NrrdVolumeLoader loader;
  const qvp::VolumeLoadResult result = loader.load(directory.filePath("missing.nhdr"));

  QVERIFY(!result.success);
  QVERIFY(!result.errorMessage.isEmpty());
}

QTEST_MAIN(TestNrrdVolumeLoader)

#include "TestNrrdVolumeLoader.moc"
