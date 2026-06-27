#pragma once

#include <QImage>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QPoint>
#include <QPointF>

class QMouseEvent;
class QWheelEvent;

namespace qvp
{

class OpenGLSliceViewer : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
  explicit OpenGLSliceViewer(QWidget* parent = nullptr);
  ~OpenGLSliceViewer() override;

  void setImage(const QImage& image);
  bool hasImage() const;
  void resetView();
  QPointF panOffset() const;
  float zoomFactor() const;

protected:
  void initializeGL() override;
  void resizeGL(int width, int height) override;
  void paintGL() override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

private:
  void initializeRenderingResources();
  void destroyRenderingResources();
  GLuint compileShader(GLenum shaderType, const char* source);
  void updateQuadGeometry();
  void updateQuadGeometryWithCurrentContext();
  void uploadTextureIfNeeded();

  QImage image_;
  GLuint textureId_ = 0;
  GLuint shaderProgram_ = 0;
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  float zoomFactor_ = 1.0F;
  QPoint lastMousePosition_;
  QPointF panOffset_ = QPointF(0.0, 0.0);
  bool isPanning_ = false;
  bool textureDirty_ = false;
};

} // namespace qvp
