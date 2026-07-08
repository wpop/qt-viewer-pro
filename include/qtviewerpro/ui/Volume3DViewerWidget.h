#pragma once

#include "qtviewerpro/core/VolumeData.h"
#include "qtviewerpro/render/VolumeTransferFunction.h"

#include <memory>
#include <QWidget>

class QLabel;

namespace qvp
{

enum class VolumeRenderPreset;
class OpenGLVolumeRendererWidget;

class Volume3DViewerWidget : public QWidget
{
  Q_OBJECT

public:
  explicit Volume3DViewerWidget(QWidget* parent = nullptr);

  void setVolume(std::shared_ptr<const VolumeData> volume);
  void setRenderPreset(VolumeRenderPreset preset);
  void setGlobalOpacity(float opacity);
  void setManualIntensityRange(float minimum, float maximum);
  VolumeTransferFunctionState transferFunctionState() const;
  void resetView();

private:
  OpenGLVolumeRendererWidget* rendererWidget_ = nullptr;
  std::shared_ptr<const VolumeData> currentVolume_;
  QLabel* statusLabel_ = nullptr;
};

} // namespace qvp
