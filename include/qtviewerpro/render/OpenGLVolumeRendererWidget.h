#pragma once

#include <memory>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QString>

namespace qvp
{

class VolumeData;

class OpenGLVolumeRendererWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT

public:
  explicit OpenGLVolumeRendererWidget(QWidget* parent = nullptr);
  ~OpenGLVolumeRendererWidget() override;

  void setVolume(std::shared_ptr<const VolumeData> volume);

signals:
  void volumeTextureUploaded(int width, int height, int depth);
  void volumeTextureUploadFailed(const QString& message);

protected:
  void initializeGL() override;
  void resizeGL(int width, int height) override;
  void paintGL() override;

private:
  void initializeRenderingResources();
  void destroyRenderingResources();
  void uploadVolumeTextureIfNeeded();
  void destroyVolumeTexture();
  GLuint compileShader(GLenum shaderType, const char* source);

  std::shared_ptr<const VolumeData> currentVolume_;
  GLuint volumeTextureId_ = 0;
  bool volumeTextureDirty_ = false;
  bool volumeTextureReady_ = false;
  GLuint shaderProgram_ = 0;
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
};

} // namespace qvp
