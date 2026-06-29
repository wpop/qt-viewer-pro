#pragma once

#include "qtviewerpro/core/VolumeData.h"

#include <QString>

#include <utility>

namespace qvp
{

struct VolumeLoadResult
{
  bool success = false;
  VolumeData volume;
  QString errorMessage;

  static VolumeLoadResult makeSuccess(VolumeData volume)
  {
    VolumeLoadResult result;
    result.success = true;
    result.volume = std::move(volume);
    return result;
  }

  static VolumeLoadResult makeFailure(QString errorMessage)
  {
    VolumeLoadResult result;
    result.success = false;
    result.errorMessage = std::move(errorMessage);
    return result;
  }
};

} // namespace qvp
