#include "ImageProcessor.h"

#include <QImage>

#include <opencv2/imgproc.hpp>

QImage ImageProcessor::toGrayscale(const QImage& image) const
{
  if (image.isNull())
    return QImage();

  const QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);

  cv::Mat rgbMat(
      rgbImage.height(),
      rgbImage.width(),
      CV_8UC3,
      const_cast<uchar*>(rgbImage.bits()),
      static_cast<size_t>(rgbImage.bytesPerLine()));

  cv::Mat grayMat;
  cv::cvtColor(rgbMat, grayMat, cv::COLOR_RGB2GRAY);

  QImage grayImage(
      grayMat.data,
      grayMat.cols,
      grayMat.rows,
      static_cast<int>(grayMat.step),
      QImage::Format_Grayscale8);

  return grayImage.copy();
}
