#include "qtviewerpro/ui/OpenGLDemoWindow.h"

#include "qtviewerpro/ui/OpenGLVolumeViewerWidget.h"

#include <QVBoxLayout>

namespace qvp
{

OpenGLDemoWindow::OpenGLDemoWindow(QWidget* parent) : QWidget(parent)
{
  setWindowTitle("OpenGL Slice Viewer Demo");
  resize(640, 480);

  createUi();
}

void OpenGLDemoWindow::setVolume(VolumeData volume)
{
  viewerWidget_->setVolume(std::move(volume));
}

void OpenGLDemoWindow::openMaskOverlay()
{
  viewerWidget_->openMaskOverlay();
}

void OpenGLDemoWindow::createUi()
{
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  viewerWidget_ = new OpenGLVolumeViewerWidget(this);
  layout->addWidget(viewerWidget_);
}

} // namespace qvp
