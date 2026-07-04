#include "qtviewerpro/render/OpenGLVolumeRendererWidget.h"

#include <QSizePolicy>

namespace qvp
{

OpenGLVolumeRendererWidget::OpenGLVolumeRendererWidget(QWidget* parent) : QOpenGLWidget(parent)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void OpenGLVolumeRendererWidget::initializeGL()
{
  initializeOpenGLFunctions();
  glClearColor(0.08F, 0.08F, 0.10F, 1.0F);
  glEnable(GL_DEPTH_TEST);
}

void OpenGLVolumeRendererWidget::resizeGL(int width, int height)
{
  glViewport(0, 0, width, height);
}

void OpenGLVolumeRendererWidget::paintGL()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

} // namespace qvp
