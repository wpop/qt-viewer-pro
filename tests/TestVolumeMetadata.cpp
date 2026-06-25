#include "qtviewerpro/core/VolumeMetadata.h"

#include <QtTest/QtTest>

class TestVolumeMetadata : public QObject
{
  Q_OBJECT

private slots:
  void defaultConstructorCreatesZeroDimensions();
  void defaultConstructorUsesUnitSpacing();
  void constructorStoresDimensions();
  void constructorStoresSpacing();
};

void TestVolumeMetadata::defaultConstructorCreatesZeroDimensions()
{
  const qvp::VolumeMetadata metadata;

  QCOMPARE(metadata.width(), std::size_t{0});
  QCOMPARE(metadata.height(), std::size_t{0});
  QCOMPARE(metadata.depth(), std::size_t{0});
}

void TestVolumeMetadata::defaultConstructorUsesUnitSpacing()
{
  const qvp::VolumeMetadata metadata;

  QCOMPARE(metadata.spacingX(), 1.0F);
  QCOMPARE(metadata.spacingY(), 1.0F);
  QCOMPARE(metadata.spacingZ(), 1.0F);
}

void TestVolumeMetadata::constructorStoresDimensions()
{
  const qvp::VolumeMetadata metadata(128, 64, 32, 1.0F, 1.0F, 1.0F);

  QCOMPARE(metadata.width(), std::size_t{128});
  QCOMPARE(metadata.height(), std::size_t{64});
  QCOMPARE(metadata.depth(), std::size_t{32});
}

void TestVolumeMetadata::constructorStoresSpacing()
{
  const qvp::VolumeMetadata metadata(1, 1, 1, 0.5F, 0.75F, 1.25F);

  QCOMPARE(metadata.spacingX(), 0.5F);
  QCOMPARE(metadata.spacingY(), 0.75F);
  QCOMPARE(metadata.spacingZ(), 1.25F);
}

QTEST_MAIN(TestVolumeMetadata)

#include "TestVolumeMetadata.moc"
