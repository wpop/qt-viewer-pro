#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

namespace qvp
{

class OpenGLVolumeRendererWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT

public:
  explicit OpenGLVolumeRendererWidget(QWidget* parent = nullptr);
  ~OpenGLVolumeRendererWidget() override;

protected:
  void initializeGL() override;
  void resizeGL(int width, int height) override;
  void paintGL() override;

private:
  void initializeRenderingResources();
  void destroyRenderingResources();
  GLuint compileShader(GLenum shaderType, const char* source);

  GLuint shaderProgram_ = 0;
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
};

} // namespace qvp
