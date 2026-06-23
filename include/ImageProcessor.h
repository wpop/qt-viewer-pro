#pragma once

#include <QImage>

class ImageProcessor
{
public:
  QImage toGrayscale(const QImage& image) const;
};
