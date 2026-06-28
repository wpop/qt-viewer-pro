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
  Q_OBJECT

public:
  explicit OpenGLSliceViewer(QWidget* parent = nullptr);
  ~OpenGLSliceViewer() override;

  void setImage(const QImage& image);
  void setSliceImage(const QImage& image);
  bool hasImage() const;
  void setCrosshairVisible(bool visible);
  bool isCrosshairVisible() const;
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

signals:
  void crosshairPositionChanged(QPointF position);

private:
  void initializeRenderingResources();
  void destroyRenderingResources();
  GLuint compileShader(GLenum shaderType, const char* source);
  void drawImageBorder();
  void drawCrosshair();
  void computeQuadExtents(float& halfWidth, float& halfHeight) const;
  void updateQuadGeometry();
  void updateQuadGeometryWithCurrentContext();
  void uploadTextureIfNeeded();

  QImage image_;
  GLuint textureId_ = 0;
  GLuint shaderProgram_ = 0;
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint crosshairShaderProgram_ = 0;
  GLuint crosshairVao_ = 0;
  GLuint crosshairVbo_ = 0;
  float zoomFactor_ = 1.0F;
  QPoint lastMousePosition_;
  QPointF panOffset_ = QPointF(0.0, 0.0);
  QPointF crosshairPosition_ = QPointF(0.0, 0.0);
  bool isPanning_ = false;
  bool textureDirty_ = false;
  bool showImageBorder_ = true;
  bool showCrosshair_ = true;
};

} // namespace qvp
