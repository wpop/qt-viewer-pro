#pragma once

#include <memory>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QQuaternion>
#include <QPoint>
#include <QVector3D>
#include <QString>

class QMouseEvent;
class QWheelEvent;

namespace qvp
{

enum class VolumeRenderPreset
{
  Default = 0,
  CtBone = 1,
  CtLung = 2
};

class VolumeData;

class OpenGLVolumeRendererWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT

public:
  explicit OpenGLVolumeRendererWidget(QWidget* parent = nullptr);
  ~OpenGLVolumeRendererWidget() override;

  void setVolume(std::shared_ptr<const VolumeData> volume);
  void setRenderPreset(VolumeRenderPreset renderPreset);
  void resetView();

signals:
  void volumeTextureUploaded(int width, int height, int depth);
  void volumeTextureUploadFailed(const QString& message);

protected:
  void initializeGL() override;
  void resizeGL(int width, int height) override;
  void paintGL() override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

private:
  static constexpr float kDefaultCameraDistance = 2.5F;

  void initializeRenderingResources();
  void destroyRenderingResources();
  void uploadVolumeTextureIfNeeded();
  void destroyVolumeTexture();
  GLuint compileShader(GLenum shaderType, const char* source);
  QVector3D mapToArcball(const QPoint& position) const;
  QQuaternion rotationBetweenVectors(const QVector3D& from, const QVector3D& to) const;

  std::shared_ptr<const VolumeData> currentVolume_;
  GLuint volumeTextureId_ = 0;
  bool volumeTextureDirty_ = false;
  bool volumeTextureReady_ = false;
  GLuint shaderProgram_ = 0;
  GLuint volumeShaderProgram_ = 0;
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint volumeVao_ = 0;
  GLuint volumeVbo_ = 0;
  float volumeIntensityMinimum_ = 0.0F;
  float volumeIntensityMaximum_ = 0.0F;
  VolumeRenderPreset activeRenderPreset_ = VolumeRenderPreset::Default;
  QQuaternion volumeRotation_ = QQuaternion(1.0F, 0.0F, 0.0F, 0.0F);
  QPoint lastMousePosition_;
  bool isRotating_ = false;
  float cameraDistance_ = kDefaultCameraDistance;
};

} // namespace qvp
