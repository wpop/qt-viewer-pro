#pragma once

#include "qtviewerpro/io/MedicalVolumeLoader.h"

namespace qvp
{

class MetaImageVolumeLoader final : public MedicalVolumeLoader
{
public:
  bool canLoad(const QString& path) const override;
  VolumeLoadResult load(const QString& path) const override;
};

} // namespace qvp
