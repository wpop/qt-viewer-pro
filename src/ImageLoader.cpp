#include "ImageLoader.h"

#include <QImage>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

QImage ImageLoader::load(const QString& fileName) const
{
  cv::Mat bgrImage = cv::imread(fileName.toStdString(), cv::IMREAD_COLOR);

  if (bgrImage.empty())
    return QImage();

  cv::Mat rgbImage;
  cv::cvtColor(bgrImage, rgbImage, cv::COLOR_BGR2RGB);

  QImage image(
      rgbImage.data,
      rgbImage.cols,
      rgbImage.rows,
      static_cast<int>(rgbImage.step),
      QImage::Format_RGB888);

  return image.copy();
}
