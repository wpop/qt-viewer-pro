#include "qtviewerpro/io/RawVolumeLoader.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{

QJsonValue requiredValue(const QJsonObject& object, const QString& fieldName)
{
  if (!object.contains(fieldName))
    throw std::runtime_error(("Missing metadata field: " + fieldName).toStdString());

  return object.value(fieldName);
}

std::size_t positiveDimension(const QJsonObject& object, const QString& fieldName)
{
  const QJsonValue value = requiredValue(object, fieldName);
  if (!value.isDouble())
    throw std::runtime_error(("Metadata field must be numeric: " + fieldName).toStdString());

  const double number = value.toDouble();
  if (!std::isfinite(number) || number <= 0.0 || std::floor(number) != number)
    throw std::runtime_error(("Metadata dimension must be a positive integer: " + fieldName)
                                 .toStdString());

  if (number > static_cast<double>(std::numeric_limits<std::size_t>::max()))
    throw std::runtime_error(("Metadata dimension is too large: " + fieldName).toStdString());

  return static_cast<std::size_t>(number);
}

float positiveSpacing(const QJsonObject& object, const QString& fieldName)
{
  const QJsonValue value = requiredValue(object, fieldName);
  if (!value.isDouble())
    throw std::runtime_error(("Metadata field must be numeric: " + fieldName).toStdString());

  const double number = value.toDouble();
  if (!std::isfinite(number) || number <= 0.0 ||
      number > static_cast<double>(std::numeric_limits<float>::max()))
    throw std::runtime_error(("Metadata spacing must be positive: " + fieldName).toStdString());

  return static_cast<float>(number);
}

std::size_t checkedMultiply(std::size_t lhs, std::size_t rhs)
{
  if (lhs != 0 && rhs > std::numeric_limits<std::size_t>::max() / lhs)
    throw std::runtime_error("Volume dimensions are too large");

  return lhs * rhs;
}

} // namespace

namespace qvp
{

VolumeData RawVolumeLoader::load(const QString& metadataPath, const QString& rawPath)
{
  QFile metadataFile(metadataPath);
  if (!metadataFile.open(QIODevice::ReadOnly))
    throw std::runtime_error("Unable to open metadata file");

  QJsonParseError parseError;
  const QJsonDocument metadataDocument =
      QJsonDocument::fromJson(metadataFile.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !metadataDocument.isObject())
    throw std::runtime_error("Metadata JSON must be a valid object");

  const QJsonObject metadata = metadataDocument.object();
  const std::size_t width = positiveDimension(metadata, "width");
  const std::size_t height = positiveDimension(metadata, "height");
  const std::size_t depth = positiveDimension(metadata, "depth");
  const float spacingX = positiveSpacing(metadata, "spacingX");
  const float spacingY = positiveSpacing(metadata, "spacingY");
  const float spacingZ = positiveSpacing(metadata, "spacingZ");

  const std::size_t voxelCount = checkedMultiply(checkedMultiply(width, height), depth);
  const std::size_t expectedBytes = checkedMultiply(voxelCount, sizeof(float));

  QFile rawFile(rawPath);
  if (!rawFile.open(QIODevice::ReadOnly))
    throw std::runtime_error("Unable to open raw volume file");

  if (expectedBytes > static_cast<std::size_t>(std::numeric_limits<qint64>::max()) ||
      rawFile.size() != static_cast<qint64>(expectedBytes))
    throw std::runtime_error("Raw volume file size does not match metadata dimensions");

  const QByteArray rawBytes = rawFile.readAll();
  if (rawBytes.size() != static_cast<qsizetype>(expectedBytes))
    throw std::runtime_error("Unable to read complete raw volume file");

  std::vector<float> voxels(voxelCount);
  std::memcpy(voxels.data(), rawBytes.constData(), expectedBytes);

  return VolumeData(width, height, depth, spacingX, spacingY, spacingZ, std::move(voxels));
}

} // namespace qvp
