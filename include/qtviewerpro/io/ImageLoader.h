#pragma once

#include <QString>

class QImage;

class ImageLoader
{
public:
  QImage load(const QString& fileName) const;
};
