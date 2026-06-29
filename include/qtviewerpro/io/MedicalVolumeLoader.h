#pragma once

#include "qtviewerpro/io/VolumeLoadResult.h"

#include <QString>

namespace qvp
{

class MedicalVolumeLoader
{
public:
  virtual ~MedicalVolumeLoader() = default;

  virtual bool canLoad(const QString& path) const = 0;
  virtual VolumeLoadResult load(const QString& path) const = 0;
};

} // namespace qvp
