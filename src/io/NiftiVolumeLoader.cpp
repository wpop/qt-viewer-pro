#include "qtviewerpro/io/NiftiVolumeLoader.h"

#include "qtviewerpro/io/ItkVolumeConverter.h"

#include <QFileInfo>

#include <itkImageFileReader.h>
#include <itkMetaDataObject.h>
#include <itkNiftiImageIO.h>

#include <charconv>
#include <cctype>
#include <exception>
#include <string>
#include <string_view>

namespace
{

bool hasNiftiExtension(const QString& path)
{
  const QString lowerFileName = QFileInfo(path).fileName().toLower();
  return lowerFileName.endsWith(QStringLiteral(".nii")) ||
         lowerFileName.endsWith(QStringLiteral(".nii.gz"));
}

std::string_view trimAsciiWhitespace(std::string_view value)
{
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
  {
    value.remove_prefix(1);
  }
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
  {
    value.remove_suffix(1);
  }
  return value;
}

bool parsePositiveCodeValue(const std::string& value)
{
  const std::string_view trimmedValue = trimAsciiWhitespace(value);
  if (trimmedValue.empty())
  {
    return false;
  }

  int parsedValue = 0;
  const char* const begin = trimmedValue.data();
  const char* const end = begin + trimmedValue.size();
  const auto [ptr, ec] = std::from_chars(begin, end, parsedValue);
  if (ec != std::errc{} || ptr != end)
  {
    return false;
  }

  return parsedValue > 0;
}

bool hasTrustedNiftiOrientation(const itk::MetaDataDictionary& dictionary)
{
  for (const char* key : {"qform_code", "sform_code"})
  {
    std::string codeValue;
    if (!itk::ExposeMetaData<std::string>(dictionary, key, codeValue))
    {
      continue;
    }

    if (parsePositiveCodeValue(codeValue))
    {
      return true;
    }
  }

  return false;
}

} // namespace

namespace qvp
{

bool NiftiVolumeLoader::canLoad(const QString& path) const
{
  return hasNiftiExtension(path);
}

VolumeLoadResult NiftiVolumeLoader::load(const QString& path) const
{
  using ReaderType = itk::ImageFileReader<ItkVolumeImage>;

  try
  {
    const auto niftiImageIO = itk::NiftiImageIO::New();
    const auto reader = ReaderType::New();
    reader->SetFileName(path.toStdString());
    reader->SetImageIO(niftiImageIO);
    reader->Update();

    const ItkSpatialGeometryPolicy policy{
        hasTrustedNiftiOrientation(niftiImageIO->GetMetaDataDictionary()),
        VolumeData::CoordinateSystem::LPS};

    const VolumeData volume =
        convertItkImageToVolume(reader->GetOutput(), policy, "NIfTI");
    if (!volume.isValid())
    {
      return VolumeLoadResult::makeFailure(QStringLiteral("Loaded NIfTI volume is invalid"));
    }

    return VolumeLoadResult::makeSuccess(volume);
  }
  catch (const itk::ExceptionObject& exception)
  {
    return VolumeLoadResult::makeFailure(
        QStringLiteral("Failed to load NIfTI volume: ") + exception.GetDescription());
  }
  catch (const std::exception& exception)
  {
    return VolumeLoadResult::makeFailure(
        QStringLiteral("Failed to load NIfTI volume: ") + QString::fromUtf8(exception.what()));
  }
}

} // namespace qvp
