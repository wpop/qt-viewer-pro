#pragma once

#include "qtviewerpro/io/MedicalVolumeLoader.h"

namespace qvp
{

class NrrdVolumeLoader final : public MedicalVolumeLoader
{
public:
  bool canLoad(const QString& path) const override;
  VolumeLoadResult load(const QString& path) const override;
};

} // namespace qvp
