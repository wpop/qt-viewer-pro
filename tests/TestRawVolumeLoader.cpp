#include "qtviewerpro/io/RawVolumeLoader.h"
#include "qtviewerpro/io/MedicalVolumeLoaderRegistry.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <stdexcept>
#include <vector>

namespace
{

QString metadataPath(const QTemporaryDir& directory)
{
  return directory.filePath("volume.json");
}

QString rawPath(const QTemporaryDir& directory)
{
  return directory.filePath("volume.raw");
}

QString rawPath(const QTemporaryDir& directory, const QString& fileName)
{
  return directory.filePath(fileName);
}

void writeMetadata(const QString& path, int width, int height, int depth, double spacingX,
                   double spacingY, double spacingZ)
{
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));

  const QJsonObject metadata{{"width", width},
                             {"height", height},
                             {"depth", depth},
                             {"spacingX", spacingX},
                             {"spacingY", spacingY},
                             {"spacingZ", spacingZ}};
  const QByteArray json = QJsonDocument(metadata).toJson(QJsonDocument::Compact);
  QCOMPARE(file.write(json), static_cast<qint64>(json.size()));
}

void writeRaw(const QString& path, const std::vector<float>& voxels)
{
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));

  const auto* data = reinterpret_cast<const char*>(voxels.data());
  const qint64 byteCount = static_cast<qint64>(voxels.size() * sizeof(float));
  QCOMPARE(file.write(data, byteCount), byteCount);
}

} // namespace

class TestRawVolumeLoader : public QObject
{
  Q_OBJECT

private slots:
  void loadsSmallValidRawVolume();
  void loadsSiblingVolumeRawWhenOnlyMetadataPathIsProvided();
  void loadsRepositorySampleMetadataAndRaw();
  void loadsRepositorySampleViaMedicalVolumeDispatch();
  void throwsForMissingMetadataFile();
  void throwsForInvalidRawSize();
};

void TestRawVolumeLoader::loadsSmallValidRawVolume()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const std::vector<float> voxels{1.0F, 2.0F, 3.0F, 4.0F};
  writeMetadata(metadataPath(directory), 2, 2, 1, 0.5, 0.75, 1.25);
  writeRaw(rawPath(directory), voxels);

  const qvp::VolumeData volume =
      qvp::RawVolumeLoader::load(metadataPath(directory), rawPath(directory));

  QCOMPARE(volume.width(), std::size_t{2});
  QCOMPARE(volume.height(), std::size_t{2});
  QCOMPARE(volume.depth(), std::size_t{1});
  QCOMPARE(volume.spacingX(), 0.5F);
  QCOMPARE(volume.spacingY(), 0.75F);
  QCOMPARE(volume.spacingZ(), 1.25F);
  QVERIFY(volume.voxels() == voxels);
}

void TestRawVolumeLoader::loadsSiblingVolumeRawWhenOnlyMetadataPathIsProvided()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const std::vector<float> voxels{1.0F, 2.0F, 3.0F, 4.0F};
  const QString siblingRawPath = rawPath(directory, "payload.raw");
  writeMetadata(metadataPath(directory), 2, 2, 1, 0.5, 0.75, 1.25);
  writeRaw(siblingRawPath, voxels);

  const qvp::VolumeData volume = qvp::RawVolumeLoader::load(metadataPath(directory));

  QCOMPARE(volume.width(), std::size_t{2});
  QCOMPARE(volume.height(), std::size_t{2});
  QCOMPARE(volume.depth(), std::size_t{1});
  QCOMPARE(volume.spacingX(), 0.5F);
  QCOMPARE(volume.spacingY(), 0.75F);
  QCOMPARE(volume.spacingZ(), 1.25F);
  QVERIFY(volume.voxels() == voxels);
}

void TestRawVolumeLoader::loadsRepositorySampleMetadataAndRaw()
{
  const QString metadata = QFINDTESTDATA("../data/samples/raw/custom_test_volume/metadata.json");
  QVERIFY2(!metadata.isEmpty(), "Repository RAW sample metadata was not found");

  const qvp::VolumeData volume = qvp::RawVolumeLoader::load(metadata);

  QCOMPARE(volume.width(), std::size_t{64});
  QCOMPARE(volume.height(), std::size_t{64});
  QCOMPARE(volume.depth(), std::size_t{16});
  QCOMPARE(volume.spacingX(), 1.0F);
  QCOMPARE(volume.spacingY(), 1.0F);
  QCOMPARE(volume.spacingZ(), 2.0F);
  QCOMPARE(volume.voxelCount(), std::size_t{64 * 64 * 16});
  QVERIFY(volume.isValid());
}

void TestRawVolumeLoader::loadsRepositorySampleViaMedicalVolumeDispatch()
{
  const QString metadata = QFINDTESTDATA("../data/samples/raw/custom_test_volume/metadata.json");
  QVERIFY2(!metadata.isEmpty(), "Repository RAW sample metadata was not found");

  const qvp::VolumeLoadResult result = qvp::loadMedicalVolume(metadata);

  QVERIFY(result.success);
  QCOMPARE(result.volume.width(), std::size_t{64});
  QCOMPARE(result.volume.height(), std::size_t{64});
  QCOMPARE(result.volume.depth(), std::size_t{16});
  QCOMPARE(result.volume.spacingX(), 1.0F);
  QCOMPARE(result.volume.spacingY(), 1.0F);
  QCOMPARE(result.volume.spacingZ(), 2.0F);
  QVERIFY(result.volume.isValid());
}

void TestRawVolumeLoader::throwsForMissingMetadataFile()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  writeRaw(rawPath(directory), {1.0F});

  QVERIFY_EXCEPTION_THROWN(
      qvp::RawVolumeLoader::load(metadataPath(directory), rawPath(directory)),
      std::runtime_error);
}

void TestRawVolumeLoader::throwsForInvalidRawSize()
{
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  writeMetadata(metadataPath(directory), 2, 2, 1, 1.0, 1.0, 1.0);
  writeRaw(rawPath(directory), {1.0F});

  QVERIFY_EXCEPTION_THROWN(
      qvp::RawVolumeLoader::load(metadataPath(directory), rawPath(directory)),
      std::runtime_error);
}

QTEST_MAIN(TestRawVolumeLoader)

#include "TestRawVolumeLoader.moc"
