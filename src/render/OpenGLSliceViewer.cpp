#include "qtviewerpro/render/OpenGLSliceViewer.h"

namespace qvp
{

OpenGLSliceViewer::OpenGLSliceViewer(QWidget* parent) : QOpenGLWidget(parent)
{
}

OpenGLSliceViewer::~OpenGLSliceViewer()
{
  if (textureId_ != 0 && context())
  {
    makeCurrent();
    glDeleteTextures(1, &textureId_);
    textureId_ = 0;
    doneCurrent();
  }
}

void OpenGLSliceViewer::setImage(const QImage& image)
{
  image_ = image;
  textureDirty_ = true;
  update();
}

bool OpenGLSliceViewer::hasImage() const
{
  return !image_.isNull();
}

void OpenGLSliceViewer::initializeGL()
{
  initializeOpenGLFunctions();
  glGenTextures(1, &textureId_);
  glClearColor(17.0F / 255.0F, 17.0F / 255.0F, 17.0F / 255.0F, 1.0F);
}

void OpenGLSliceViewer::resizeGL(int width, int height)
{
  glViewport(0, 0, width, height);
}

void OpenGLSliceViewer::paintGL()
{
  glClear(GL_COLOR_BUFFER_BIT);
  uploadTextureIfNeeded();
}

void OpenGLSliceViewer::uploadTextureIfNeeded()
{
  if (!textureDirty_)
  {
    return;
  }

  if (image_.isNull())
  {
    textureDirty_ = false;
    return;
  }

  if (textureId_ == 0)
  {
    return;
  }

  glBindTexture(GL_TEXTURE_2D, textureId_);

  const QImage textureImage = image_.convertToFormat(QImage::Format_RGBA8888);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               textureImage.width(),
               textureImage.height(),
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               textureImage.constBits());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_2D, 0);
  textureDirty_ = false;
}

} // namespace qvp
