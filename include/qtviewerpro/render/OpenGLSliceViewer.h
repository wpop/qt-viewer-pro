#pragma once

#include <QImage>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>

namespace qvp
{

class OpenGLSliceViewer : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
  explicit OpenGLSliceViewer(QWidget* parent = nullptr);
  ~OpenGLSliceViewer() override;

  void setImage(const QImage& image);
  bool hasImage() const;

protected:
  void initializeGL() override;
  void resizeGL(int width, int height) override;
  void paintGL() override;

private:
  void initializeRenderingResources();
  void destroyRenderingResources();
  GLuint compileShader(GLenum shaderType, const char* source);
  void uploadTextureIfNeeded();

  QImage image_;
  GLuint textureId_ = 0;
  GLuint shaderProgram_ = 0;
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  bool textureDirty_ = false;
};

} // namespace qvp
