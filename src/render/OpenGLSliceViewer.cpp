#include "qtviewerpro/render/OpenGLSliceViewer.h"

#include <QColor>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QWheelEvent>

#include <algorithm>

namespace
{
constexpr float kZoomStep = 1.25F;
constexpr float kMinZoomFactor = 0.1F;
constexpr float kMaxZoomFactor = 20.0F;

QPointF widgetPositionToImageLocalPosition(const QPointF& widgetPosition,
                                           const QSize& widgetSize,
                                           const QPointF& panOffset,
                                           float halfWidth,
                                           float halfHeight)
{
  if (widgetSize.width() <= 0 || widgetSize.height() <= 0 || halfWidth <= 0.0F ||
      halfHeight <= 0.0F)
  {
    return QPointF(0.0, 0.0);
  }

  const double ndcX = (2.0 * widgetPosition.x() / static_cast<double>(widgetSize.width())) - 1.0;
  const double ndcY = 1.0 - (2.0 * widgetPosition.y() / static_cast<double>(widgetSize.height()));

  const double localX = (ndcX - panOffset.x()) / static_cast<double>(halfWidth);
  const double localY = (ndcY - panOffset.y()) / static_cast<double>(halfHeight);

  return QPointF(std::clamp(localX, -1.0, 1.0), std::clamp(localY, -1.0, 1.0));
}

QPoint imageLocalPositionToPixelPosition(const QPointF& imageLocalPosition, const QSize& imageSize)
{
  if (imageSize.width() <= 0 || imageSize.height() <= 0)
  {
    return QPoint(0, 0);
  }

  const double x = ((imageLocalPosition.x() + 1.0) * 0.5) * static_cast<double>(imageSize.width() - 1);
  const double y = ((1.0 - imageLocalPosition.y()) * 0.5) * static_cast<double>(imageSize.height() - 1);

  return QPoint(static_cast<int>(std::clamp(x, 0.0, static_cast<double>(imageSize.width() - 1))),
                static_cast<int>(std::clamp(y, 0.0, static_cast<double>(imageSize.height() - 1))));
}
}

namespace qvp
{

OpenGLSliceViewer::OpenGLSliceViewer(QWidget* parent) : QOpenGLWidget(parent)
{
  setMouseTracking(true);
}

OpenGLSliceViewer::~OpenGLSliceViewer()
{
  if (context())
  {
    makeCurrent();
    if (textureId_ != 0)
    {
      glDeleteTextures(1, &textureId_);
      textureId_ = 0;
    }
    destroyRenderingResources();
    doneCurrent();
  }
}

void OpenGLSliceViewer::setImage(const QImage& image)
{
  image_ = image;
  crosshairPosition_ = QPointF(0.0, 0.0);
  textureDirty_ = true;
  updateQuadGeometryWithCurrentContext();
  emit crosshairPositionChanged(crosshairPosition_);
  emit crosshairPositionValueChanged(crosshairPosition_, sampleImageValueAt(crosshairPosition_));
  update();
}

void OpenGLSliceViewer::setSliceImage(const QImage& image)
{
  setImage(image);
}

bool OpenGLSliceViewer::hasImage() const
{
  return !image_.isNull();
}

void OpenGLSliceViewer::setCrosshairVisible(bool visible)
{
  showCrosshair_ = visible;
  update();
}

bool OpenGLSliceViewer::isCrosshairVisible() const
{
  return showCrosshair_;
}

void OpenGLSliceViewer::resetCrosshair()
{
  crosshairPosition_ = QPointF(0.0, 0.0);
  emit crosshairPositionChanged(crosshairPosition_);
  emit crosshairPositionValueChanged(crosshairPosition_, sampleImageValueAt(crosshairPosition_));
  update();
}

void OpenGLSliceViewer::resetView()
{
  zoomFactor_ = 1.0F;
  panOffset_ = QPointF(0.0, 0.0);
  updateQuadGeometryWithCurrentContext();
  update();
}

QPointF OpenGLSliceViewer::panOffset() const
{
  return panOffset_;
}

float OpenGLSliceViewer::zoomFactor() const
{
  return zoomFactor_;
}

void OpenGLSliceViewer::initializeGL()
{
  initializeOpenGLFunctions();
  glGenTextures(1, &textureId_);
  initializeRenderingResources();
  glClearColor(17.0F / 255.0F, 17.0F / 255.0F, 17.0F / 255.0F, 1.0F);
}

void OpenGLSliceViewer::resizeGL(int width, int height)
{
  glViewport(0, 0, width, height);
  updateQuadGeometry();
}

void OpenGLSliceViewer::paintGL()
{
  glClear(GL_COLOR_BUFFER_BIT);
  uploadTextureIfNeeded();

  if (!hasImage() || textureId_ == 0 || shaderProgram_ == 0)
  {
    return;
  }

  auto* extraFunctions = QOpenGLContext::currentContext()->extraFunctions();
  glUseProgram(shaderProgram_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureId_);
  glUniform1i(glGetUniformLocation(shaderProgram_, "imageTexture"), 0);

  extraFunctions->glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  extraFunctions->glBindVertexArray(0);

  if (showImageBorder_)
  {
    drawImageBorder();
  }

  if (showCrosshair_)
  {
    drawCrosshair();
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
}

void OpenGLSliceViewer::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    isPanning_ = true;
    lastMousePosition_ = event->position().toPoint();
    event->accept();
    return;
  }

  QOpenGLWidget::mousePressEvent(event);
}

void OpenGLSliceViewer::mouseMoveEvent(QMouseEvent* event)
{
  if (isPanning_)
  {
    const QPoint currentPosition = event->position().toPoint();
    const QPoint delta = currentPosition - lastMousePosition_;

    if (width() > 0 && height() > 0)
    {
      panOffset_ += QPointF(2.0 * static_cast<double>(delta.x()) / static_cast<double>(width()),
                            -2.0 * static_cast<double>(delta.y()) / static_cast<double>(height()));
      updateQuadGeometryWithCurrentContext();
      update();
    }

    lastMousePosition_ = currentPosition;
    event->accept();
    return;
  }

  if (hasImage())
  {
    float halfWidth = 1.0F;
    float halfHeight = 1.0F;
    computeQuadExtents(halfWidth, halfHeight);
    const QPointF newCrosshairPosition = widgetPositionToImageLocalPosition(
        event->position(), size(), panOffset_, halfWidth, halfHeight);
    if (crosshairPosition_ != newCrosshairPosition)
    {
      crosshairPosition_ = newCrosshairPosition;
      emit crosshairPositionChanged(crosshairPosition_);
      emit crosshairPositionValueChanged(crosshairPosition_, sampleImageValueAt(crosshairPosition_));
    }
    update();
    event->accept();
    return;
  }

  QOpenGLWidget::mouseMoveEvent(event);
}

void OpenGLSliceViewer::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton && isPanning_)
  {
    isPanning_ = false;
    event->accept();
    return;
  }

  QOpenGLWidget::mouseReleaseEvent(event);
}

void OpenGLSliceViewer::wheelEvent(QWheelEvent* event)
{
  if (event->angleDelta().y() > 0)
  {
    zoomFactor_ = std::clamp(zoomFactor_ * kZoomStep, kMinZoomFactor, kMaxZoomFactor);
  }
  else if (event->angleDelta().y() < 0)
  {
    zoomFactor_ = std::clamp(zoomFactor_ / kZoomStep, kMinZoomFactor, kMaxZoomFactor);
  }

  updateQuadGeometryWithCurrentContext();
  update();
  event->accept();
}

void OpenGLSliceViewer::initializeRenderingResources()
{
  static constexpr char kVertexShaderSource[] = R"(
#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;

out vec2 fragmentTexCoord;

void main()
{
  fragmentTexCoord = texCoord;
  gl_Position = vec4(position, 0.0, 1.0);
}
)";

  static constexpr char kFragmentShaderSource[] = R"(
#version 330 core
in vec2 fragmentTexCoord;

out vec4 outputColor;

uniform sampler2D imageTexture;

void main()
{
  outputColor = texture(imageTexture, fragmentTexCoord);
}
)";
  static constexpr char kCrosshairVertexShaderSource[] = R"(
#version 330 core
layout (location = 0) in vec2 position;

void main()
{
  gl_Position = vec4(position, 0.0, 1.0);
}
)";

  static constexpr char kCrosshairFragmentShaderSource[] = R"(
#version 330 core

out vec4 outputColor;

uniform vec4 crosshairColor;

void main()
{
  outputColor = crosshairColor;
}
)";

  const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, kVertexShaderSource);
  const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);
  if (vertexShader == 0 || fragmentShader == 0)
  {
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return;
  }

  shaderProgram_ = glCreateProgram();
  glAttachShader(shaderProgram_, vertexShader);
  glAttachShader(shaderProgram_, fragmentShader);
  glLinkProgram(shaderProgram_);

  GLint linkStatus = GL_FALSE;
  glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &linkStatus);

  glDetachShader(shaderProgram_, vertexShader);
  glDetachShader(shaderProgram_, fragmentShader);
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  if (linkStatus != GL_TRUE)
  {
    glDeleteProgram(shaderProgram_);
    shaderProgram_ = 0;
    return;
  }

  auto* extraFunctions = QOpenGLContext::currentContext()->extraFunctions();
  extraFunctions->glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);

  extraFunctions->glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<qopengl_GLsizeiptr>(24 * sizeof(float)),
               nullptr,
               GL_DYNAMIC_DRAW);

  constexpr GLsizei kStride = 4 * sizeof(float);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, kStride, nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1,
                        2,
                        GL_FLOAT,
                        GL_FALSE,
                        kStride,
                        reinterpret_cast<const void*>(2 * sizeof(float)));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  extraFunctions->glBindVertexArray(0);

  const GLuint crosshairVertexShader =
      compileShader(GL_VERTEX_SHADER, kCrosshairVertexShaderSource);
  const GLuint crosshairFragmentShader =
      compileShader(GL_FRAGMENT_SHADER, kCrosshairFragmentShaderSource);
  if (crosshairVertexShader == 0 || crosshairFragmentShader == 0)
  {
    glDeleteShader(crosshairVertexShader);
    glDeleteShader(crosshairFragmentShader);
    return;
  }

  crosshairShaderProgram_ = glCreateProgram();
  glAttachShader(crosshairShaderProgram_, crosshairVertexShader);
  glAttachShader(crosshairShaderProgram_, crosshairFragmentShader);
  glLinkProgram(crosshairShaderProgram_);

  glGetProgramiv(crosshairShaderProgram_, GL_LINK_STATUS, &linkStatus);

  glDetachShader(crosshairShaderProgram_, crosshairVertexShader);
  glDetachShader(crosshairShaderProgram_, crosshairFragmentShader);
  glDeleteShader(crosshairVertexShader);
  glDeleteShader(crosshairFragmentShader);

  if (linkStatus != GL_TRUE)
  {
    glDeleteProgram(crosshairShaderProgram_);
    crosshairShaderProgram_ = 0;
    return;
  }

  extraFunctions->glGenVertexArrays(1, &crosshairVao_);
  glGenBuffers(1, &crosshairVbo_);

  extraFunctions->glBindVertexArray(crosshairVao_);
  glBindBuffer(GL_ARRAY_BUFFER, crosshairVbo_);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<qopengl_GLsizeiptr>(8 * sizeof(float)),
               nullptr,
               GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  extraFunctions->glBindVertexArray(0);

  updateQuadGeometry();
}

void OpenGLSliceViewer::destroyRenderingResources()
{
  if (vbo_ != 0)
  {
    glDeleteBuffers(1, &vbo_);
    vbo_ = 0;
  }

  if (vao_ != 0)
  {
    QOpenGLContext::currentContext()->extraFunctions()->glDeleteVertexArrays(1, &vao_);
    vao_ = 0;
  }

  if (crosshairVbo_ != 0)
  {
    glDeleteBuffers(1, &crosshairVbo_);
    crosshairVbo_ = 0;
  }

  if (crosshairVao_ != 0)
  {
    QOpenGLContext::currentContext()->extraFunctions()->glDeleteVertexArrays(1, &crosshairVao_);
    crosshairVao_ = 0;
  }

  if (crosshairShaderProgram_ != 0)
  {
    glDeleteProgram(crosshairShaderProgram_);
    crosshairShaderProgram_ = 0;
  }

  if (shaderProgram_ != 0)
  {
    glDeleteProgram(shaderProgram_);
    shaderProgram_ = 0;
  }
}

GLuint OpenGLSliceViewer::compileShader(GLenum shaderType, const char* source)
{
  const GLuint shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  GLint compileStatus = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
  if (compileStatus != GL_TRUE)
  {
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

void OpenGLSliceViewer::computeQuadExtents(float& halfWidth, float& halfHeight) const
{
  halfWidth = 1.0F;
  halfHeight = 1.0F;

  if (!image_.isNull() && width() > 0 && height() > 0 && image_.width() > 0 && image_.height() > 0)
  {
    const float widgetAspect = static_cast<float>(width()) / static_cast<float>(height());
    const float imageAspect =
        static_cast<float>(image_.width()) / static_cast<float>(image_.height());

    if (imageAspect > widgetAspect)
    {
      halfHeight = widgetAspect / imageAspect;
    }
    else
    {
      halfWidth = imageAspect / widgetAspect;
    }
  }

  halfWidth *= zoomFactor_;
  halfHeight *= zoomFactor_;
}

void OpenGLSliceViewer::updateQuadGeometry()
{
  if (vbo_ == 0)
  {
    return;
  }

  float halfWidth = 1.0F;
  float halfHeight = 1.0F;
  computeQuadExtents(halfWidth, halfHeight);

  const float centerX = static_cast<float>(panOffset_.x());
  const float centerY = static_cast<float>(panOffset_.y());
  const float left = centerX - halfWidth;
  const float right = centerX + halfWidth;
  const float bottom = centerY - halfHeight;
  const float top = centerY + halfHeight;

  const float quadVertices[] = {
      left,  bottom, 0.0F, 1.0F,
      right, bottom, 1.0F, 1.0F,
      right, top,    1.0F, 0.0F,
      left,  bottom, 0.0F, 1.0F,
      right, top,    1.0F, 0.0F,
      left,  top,    0.0F, 0.0F,
  };

  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferSubData(GL_ARRAY_BUFFER,
                  0,
                  static_cast<qopengl_GLsizeiptr>(sizeof(quadVertices)),
                  quadVertices);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLSliceViewer::drawCrosshair()
{
  if (crosshairShaderProgram_ == 0 || crosshairVao_ == 0 || crosshairVbo_ == 0)
  {
    return;
  }

  float halfWidth = 1.0F;
  float halfHeight = 1.0F;
  computeQuadExtents(halfWidth, halfHeight);

  const float centerX = static_cast<float>(panOffset_.x());
  const float centerY = static_cast<float>(panOffset_.y());
  const float crosshairCenterX =
      centerX + (static_cast<float>(crosshairPosition_.x()) * halfWidth);
  const float crosshairCenterY =
      centerY + (static_cast<float>(crosshairPosition_.y()) * halfHeight);
  const float crosshairVertices[] = {
      centerX - halfWidth, crosshairCenterY,
      centerX + halfWidth, crosshairCenterY,
      crosshairCenterX, centerY - halfHeight,
      crosshairCenterX, centerY + halfHeight,
  };

  glBindBuffer(GL_ARRAY_BUFFER, crosshairVbo_);
  glBufferSubData(GL_ARRAY_BUFFER,
                  0,
                  static_cast<qopengl_GLsizeiptr>(sizeof(crosshairVertices)),
                  crosshairVertices);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glUseProgram(crosshairShaderProgram_);
  const GLint colorLocation = glGetUniformLocation(crosshairShaderProgram_, "crosshairColor");

  auto* extraFunctions = QOpenGLContext::currentContext()->extraFunctions();
  extraFunctions->glBindVertexArray(crosshairVao_);

  glLineWidth(2.0F);
  glUniform4f(colorLocation, 0.0F, 0.0F, 0.0F, 0.65F);
  glDrawArrays(GL_LINES, 0, 4);

  glLineWidth(1.0F);
  glUniform4f(colorLocation, 1.0F, 1.0F, 1.0F, 0.90F);
  glDrawArrays(GL_LINES, 0, 4);

  extraFunctions->glBindVertexArray(0);
  glLineWidth(1.0F);
  glUseProgram(0);
}

int OpenGLSliceViewer::sampleImageValueAt(const QPointF& position) const
{
  if (image_.isNull())
  {
    return 0;
  }

  const QPoint pixelPosition = imageLocalPositionToPixelPosition(position, image_.size());
  const QColor color = image_.pixelColor(pixelPosition);
  return qGray(color.rgb());
}

void OpenGLSliceViewer::drawImageBorder()
{
  if (crosshairShaderProgram_ == 0 || crosshairVao_ == 0 || crosshairVbo_ == 0)
  {
    return;
  }

  float halfWidth = 1.0F;
  float halfHeight = 1.0F;
  computeQuadExtents(halfWidth, halfHeight);

  const float centerX = static_cast<float>(panOffset_.x());
  const float centerY = static_cast<float>(panOffset_.y());
  const float borderVertices[] = {
      centerX - halfWidth, centerY - halfHeight,
      centerX + halfWidth, centerY - halfHeight,
      centerX + halfWidth, centerY + halfHeight,
      centerX - halfWidth, centerY + halfHeight,
  };

  glBindBuffer(GL_ARRAY_BUFFER, crosshairVbo_);
  glBufferSubData(GL_ARRAY_BUFFER,
                  0,
                  static_cast<qopengl_GLsizeiptr>(sizeof(borderVertices)),
                  borderVertices);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glUseProgram(crosshairShaderProgram_);
  const GLint colorLocation = glGetUniformLocation(crosshairShaderProgram_, "crosshairColor");

  auto* extraFunctions = QOpenGLContext::currentContext()->extraFunctions();
  extraFunctions->glBindVertexArray(crosshairVao_);

  glLineWidth(2.0F);
  glUniform4f(colorLocation, 0.0F, 0.0F, 0.0F, 0.45F);
  glDrawArrays(GL_LINE_LOOP, 0, 4);

  glLineWidth(1.0F);
  glUniform4f(colorLocation, 1.0F, 1.0F, 1.0F, 0.55F);
  glDrawArrays(GL_LINE_LOOP, 0, 4);

  extraFunctions->glBindVertexArray(0);
  glLineWidth(1.0F);
  glUseProgram(0);
}

void OpenGLSliceViewer::updateQuadGeometryWithCurrentContext()
{
  if (vbo_ == 0 || !context())
  {
    return;
  }

  const bool contextAlreadyCurrent = QOpenGLContext::currentContext() == context();
  if (!contextAlreadyCurrent)
  {
    makeCurrent();
  }

  updateQuadGeometry();

  if (!contextAlreadyCurrent)
  {
    doneCurrent();
  }
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
