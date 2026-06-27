#include "qtviewerpro/render/OpenGLSliceViewer.h"

namespace qvp
{

OpenGLSliceViewer::OpenGLSliceViewer(QWidget* parent) : QOpenGLWidget(parent)
{
}

OpenGLSliceViewer::~OpenGLSliceViewer() = default;

void OpenGLSliceViewer::setImage(const QImage& image)
{
  image_ = image;
  update();
}

bool OpenGLSliceViewer::hasImage() const
{
  return !image_.isNull();
}

void OpenGLSliceViewer::initializeGL()
{
  initializeOpenGLFunctions();
  glClearColor(17.0F / 255.0F, 17.0F / 255.0F, 17.0F / 255.0F, 1.0F);
}

void OpenGLSliceViewer::resizeGL(int width, int height)
{
  glViewport(0, 0, width, height);
}

void OpenGLSliceViewer::paintGL()
{
  glClear(GL_COLOR_BUFFER_BIT);
}

} // namespace qvp
