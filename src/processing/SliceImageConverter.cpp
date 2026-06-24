#include "qtviewerpro/processing/SliceImageConverter.h"

#include "qtviewerpro/processing/WindowLevelProcessor.h"

#include <QImage>

#include <limits>
#include <stdexcept>

namespace qvp
{

QImage SliceImageConverter::toGrayscaleImage(const SliceData& slice, float window, float level)
{
  const std::size_t expectedSize = slice.width() * slice.height();
  if (slice.pixels().size() != expectedSize)
  {
    throw std::invalid_argument("Slice pixel count does not match slice dimensions");
  }

  if (slice.width() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
      slice.height() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
  {
    throw std::invalid_argument("Slice dimensions exceed QImage limits");
  }

  const auto grayscale = WindowLevelProcessor::apply(slice, window, level);
  QImage image(static_cast<int>(slice.width()),
               static_cast<int>(slice.height()),
               QImage::Format_Grayscale8);

  for (std::size_t y = 0; y < slice.height(); ++y)
  {
    auto* row = image.scanLine(static_cast<int>(y));
    const std::size_t sourceOffset = y * slice.width();
    for (std::size_t x = 0; x < slice.width(); ++x)
    {
      row[x] = grayscale[sourceOffset + x];
    }
  }

  return image;
}

} // namespace qvp
