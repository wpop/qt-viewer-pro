#pragma once

#include "qtviewerpro/core/VolumeData.h"

#include <QWidget>

namespace qvp
{

class OpenGLVolumeViewerWidget;

class OpenGLDemoWindow : public QWidget
{
  Q_OBJECT

public:
  explicit OpenGLDemoWindow(QWidget* parent = nullptr);
  ~OpenGLDemoWindow() override = default;

  void setVolume(VolumeData volume);
  void openMaskOverlay();

private:
  void createUi();

  OpenGLVolumeViewerWidget* viewerWidget_ = nullptr;
};

} // namespace qvp
