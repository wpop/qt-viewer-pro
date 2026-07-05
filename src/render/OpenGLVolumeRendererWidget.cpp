#include "qtviewerpro/render/OpenGLVolumeRendererWidget.h"

#include "qtviewerpro/core/VolumeData.h"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QString>
#include <QMatrix4x4>
#include <QVector3D>
#include <QWheelEvent>

#include <cstddef>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace
{
constexpr float kDefaultCameraDistance = 2.5F;
constexpr float kMinCameraDistance = 1.25F;
constexpr float kMaxCameraDistance = 8.0F;
constexpr float kZoomStepFactor = 0.9F;

constexpr float kCubeVertices[] = {
    // Back face
    -0.5F, -0.5F, -0.5F,  0.5F, -0.5F, -0.5F,
    0.5F,  -0.5F, -0.5F,   0.5F, 0.5F,  -0.5F,
    0.5F,  0.5F,  -0.5F,  -0.5F, 0.5F,  -0.5F,
    -0.5F, 0.5F,  -0.5F,   -0.5F, -0.5F, -0.5F,
    // Front face
    -0.5F, -0.5F, 0.5F,    0.5F, -0.5F, 0.5F,
    0.5F,  -0.5F, 0.5F,    0.5F, 0.5F,  0.5F,
    0.5F,  0.5F,  0.5F,    -0.5F, 0.5F,  0.5F,
    -0.5F, 0.5F,  0.5F,    -0.5F, -0.5F, 0.5F,
    // Connections
    -0.5F, -0.5F, -0.5F,   -0.5F, -0.5F, 0.5F,
    0.5F,  -0.5F, -0.5F,    0.5F,  -0.5F, 0.5F,
    0.5F,   0.5F,  -0.5F,    0.5F,   0.5F, 0.5F,
    -0.5F,  0.5F,  -0.5F,   -0.5F,  0.5F, 0.5F,
};

constexpr float kVolumeCubeVertices[] = {
    // Back face
    -0.5F, -0.5F, -0.5F,  0.5F, 0.5F,  -0.5F,  0.5F, -0.5F, -0.5F,
    -0.5F, -0.5F, -0.5F,  -0.5F, 0.5F,  -0.5F,  0.5F, 0.5F,  -0.5F,
    // Front face
    -0.5F, -0.5F, 0.5F,   0.5F, -0.5F, 0.5F,   0.5F, 0.5F,   0.5F,
    -0.5F, -0.5F, 0.5F,   0.5F, 0.5F,   0.5F,   -0.5F, 0.5F,  0.5F,
    // Left face
    -0.5F, -0.5F, -0.5F,  -0.5F, 0.5F,   0.5F,   -0.5F, 0.5F,  -0.5F,
    -0.5F, -0.5F, -0.5F,  -0.5F, -0.5F, 0.5F,    -0.5F, 0.5F,  0.5F,
    // Right face
    0.5F, -0.5F, -0.5F,   0.5F, 0.5F,   -0.5F,   0.5F, 0.5F,  0.5F,
    0.5F, -0.5F, -0.5F,   0.5F, 0.5F,    0.5F,    0.5F, -0.5F, 0.5F,
    // Top face
    -0.5F, 0.5F, -0.5F,   0.5F, 0.5F,    0.5F,    0.5F, 0.5F,  -0.5F,
    -0.5F, 0.5F, -0.5F,   -0.5F, 0.5F,   0.5F,    0.5F, 0.5F,   0.5F,
    // Bottom face
    -0.5F, -0.5F, -0.5F,  0.5F, -0.5F,  -0.5F,   0.5F, -0.5F, 0.5F,
    -0.5F, -0.5F, -0.5F,  0.5F, -0.5F,   0.5F,    -0.5F, -0.5F, 0.5F,
};

QQuaternion rotationBetweenVectors(const QVector3D& from, const QVector3D& to)
{
  const QVector3D fromNormalized = from.normalized();
  const QVector3D toNormalized = to.normalized();
  const float dot = QVector3D::dotProduct(fromNormalized, toNormalized);

  if (dot >= 1.0F - 1e-6F)
  {
    return QQuaternion(1.0F, 0.0F, 0.0F, 0.0F);
  }

  if (dot <= -1.0F + 1e-6F)
  {
    QVector3D orthogonalAxis = QVector3D::crossProduct(QVector3D(1.0F, 0.0F, 0.0F), fromNormalized);
    if (orthogonalAxis.lengthSquared() < 1e-6F)
    {
      orthogonalAxis =
          QVector3D::crossProduct(QVector3D(0.0F, 1.0F, 0.0F), fromNormalized);
    }
    orthogonalAxis.normalize();
    return QQuaternion::fromAxisAndAngle(orthogonalAxis, 180.0F);
  }

  const QVector3D axis = QVector3D::crossProduct(fromNormalized, toNormalized);
  QQuaternion delta(1.0F + dot, axis.x(), axis.y(), axis.z());
  delta.normalize();
  return delta;
}

QVector3D mapToArcballPoint(const QPoint& position, int width, int height)
{
  if (width <= 0 || height <= 0)
  {
    return QVector3D(0.0F, 0.0F, 1.0F);
  }

  const float minDimension = static_cast<float>(std::min(width, height));
  const float x = (2.0F * static_cast<float>(position.x()) - static_cast<float>(width)) /
                  minDimension;
  const float y = (static_cast<float>(height) - 2.0F * static_cast<float>(position.y())) /
                  minDimension;
  const float lengthSquared = x * x + y * y;

  if (lengthSquared > 1.0F)
  {
    const float invLength = 1.0F / std::sqrt(lengthSquared);
    return QVector3D(x * invLength, y * invLength, 0.0F);
  }

  return QVector3D(x, y, std::sqrt(1.0F - lengthSquared));
}
}

namespace qvp
{

OpenGLVolumeRendererWidget::OpenGLVolumeRendererWidget(QWidget* parent) : QOpenGLWidget(parent)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

OpenGLVolumeRendererWidget::~OpenGLVolumeRendererWidget()
{
  if (context())
  {
    makeCurrent();
    destroyRenderingResources();
    doneCurrent();
  }
}

void OpenGLVolumeRendererWidget::setVolume(std::shared_ptr<const VolumeData> volume)
{
  if (!volume || !volume->isValid())
  {
    currentVolume_.reset();
    volumeTextureDirty_ = true;
    volumeTextureReady_ = false;
    update();
    return;
  }

  currentVolume_ = std::move(volume);
  volumeTextureDirty_ = true;
  volumeTextureReady_ = false;
  update();
}

void OpenGLVolumeRendererWidget::initializeGL()
{
  initializeOpenGLFunctions();
  glClearColor(0.08F, 0.08F, 0.10F, 1.0F);
  glEnable(GL_DEPTH_TEST);
  initializeRenderingResources();
}

void OpenGLVolumeRendererWidget::resizeGL(int width, int height)
{
  glViewport(0, 0, width, height);
}

void OpenGLVolumeRendererWidget::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    isRotating_ = true;
    lastMousePosition_ = event->position().toPoint();
    event->accept();
    return;
  }

  QOpenGLWidget::mousePressEvent(event);
}

void OpenGLVolumeRendererWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (isRotating_ && (event->buttons() & Qt::LeftButton))
  {
    const QPoint currentMousePosition = event->position().toPoint();
    const QVector3D previousArcball = mapToArcball(lastMousePosition_);
    const QVector3D currentArcball = mapToArcball(currentMousePosition);
    const QQuaternion deltaRotation = rotationBetweenVectors(previousArcball, currentArcball);

    volumeRotation_ = (deltaRotation * volumeRotation_).normalized();
    lastMousePosition_ = currentMousePosition;
    update();
    event->accept();
    return;
  }

  QOpenGLWidget::mouseMoveEvent(event);
}

void OpenGLVolumeRendererWidget::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton && isRotating_)
  {
    isRotating_ = false;
    event->accept();
    return;
  }

  QOpenGLWidget::mouseReleaseEvent(event);
}

void OpenGLVolumeRendererWidget::wheelEvent(QWheelEvent* event)
{
  float scrollSteps = 0.0F;
  if (!event->angleDelta().isNull())
  {
    scrollSteps = static_cast<float>(event->angleDelta().y()) / 120.0F;
  }
  else if (!event->pixelDelta().isNull())
  {
    scrollSteps = static_cast<float>(event->pixelDelta().y()) / 120.0F;
  }

  if (scrollSteps != 0.0F)
  {
    const float zoomFactor = std::pow(kZoomStepFactor, scrollSteps);
    const float nextCameraDistance = cameraDistance_ * zoomFactor;
    cameraDistance_ = std::clamp(nextCameraDistance, kMinCameraDistance, kMaxCameraDistance);
    if (!std::isfinite(cameraDistance_))
    {
      cameraDistance_ = kDefaultCameraDistance;
    }
    update();
    event->accept();
    return;
  }

  QOpenGLWidget::wheelEvent(event);
}

void OpenGLVolumeRendererWidget::paintGL()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  uploadVolumeTextureIfNeeded();

  if (shaderProgram_ == 0 || vao_ == 0 || vbo_ == 0)
  {
    return;
  }

  QMatrix4x4 projection;
  const float aspectRatio = height() > 0 ? static_cast<float>(width()) / static_cast<float>(height())
                                         : 1.0F;
  projection.perspective(45.0F, aspectRatio, 0.1F, 100.0F);

  QMatrix4x4 view;
  view.translate(0.0F, 0.0F, -cameraDistance_);

  QMatrix4x4 model;

  float volumeScaleX = 1.0F;
  float volumeScaleY = 1.0F;
  float volumeScaleZ = 1.0F;

  if (currentVolume_ && currentVolume_->isValid())
  {
    const float physicalWidth = static_cast<float>(currentVolume_->width()) *
                                static_cast<float>(currentVolume_->spacingX());
    const float physicalHeight = static_cast<float>(currentVolume_->height()) *
                                 static_cast<float>(currentVolume_->spacingY());
    const float physicalDepth = static_cast<float>(currentVolume_->depth()) *
                                static_cast<float>(currentVolume_->spacingZ());

    if (physicalWidth > 0.0F && physicalHeight > 0.0F && physicalDepth > 0.0F &&
        std::isfinite(physicalWidth) && std::isfinite(physicalHeight) &&
        std::isfinite(physicalDepth))
    {
      const float maxPhysicalExtent =
          std::max({physicalWidth, physicalHeight, physicalDepth});
      if (maxPhysicalExtent > 0.0F)
      {
        volumeScaleX = physicalWidth / maxPhysicalExtent;
        volumeScaleY = physicalHeight / maxPhysicalExtent;
        volumeScaleZ = physicalDepth / maxPhysicalExtent;
      }
    }
  }

  model.rotate(volumeRotation_);
  model.rotate(28.0F, 1.0F, 0.0F, 0.0F);
  model.rotate(38.0F, 0.0F, 1.0F, 0.0F);
  model.scale(volumeScaleX, volumeScaleY, volumeScaleZ);

  const QMatrix4x4 mvpMatrix = projection * view * model;
  const QMatrix4x4 modelView = view * model;
  bool invertible = false;
  const QMatrix4x4 inverseModelView = modelView.inverted(&invertible);

  float stepSize = 0.0F;
  if (currentVolume_ && currentVolume_->isValid())
  {
    const std::size_t width = currentVolume_->width();
    const std::size_t height = currentVolume_->height();
    const std::size_t depth = currentVolume_->depth();
    const std::size_t maxDimension = std::max({width, height, depth});
    if (maxDimension > 0)
    {
      stepSize = 1.0F / static_cast<float>(maxDimension);
    }
  }

  if (invertible)
  {
    if (volumeTextureReady_ && volumeShaderProgram_ != 0 && volumeVao_ != 0 && volumeVbo_ != 0 &&
        volumeTextureId_ != 0 && stepSize > 0.0F)
    {
      const QVector3D cameraPositionObject =
          inverseModelView.map(QVector3D(0.0F, 0.0F, 0.0F));
      const QVector3D cameraPositionTexture =
          cameraPositionObject + QVector3D(0.5F, 0.5F, 0.5F);

      GLint previousActiveTexture = GL_TEXTURE0;
      glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);

      glEnable(GL_CULL_FACE);
      glCullFace(GL_BACK);
      glFrontFace(GL_CCW);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_3D, volumeTextureId_);

      glUseProgram(volumeShaderProgram_);
      const GLint volumeMvpLocation = glGetUniformLocation(volumeShaderProgram_, "mvpMatrix");
      const GLint volumeTextureLocation =
          glGetUniformLocation(volumeShaderProgram_, "volumeTexture");
      const GLint cameraPositionLocation =
          glGetUniformLocation(volumeShaderProgram_, "cameraPositionTexture");
      const GLint stepSizeLocation = glGetUniformLocation(volumeShaderProgram_, "stepSize");
      glUniformMatrix4fv(volumeMvpLocation, 1, GL_FALSE, mvpMatrix.constData());
      glUniform1i(volumeTextureLocation, 0);
      glUniform3f(cameraPositionLocation,
                  cameraPositionTexture.x(),
                  cameraPositionTexture.y(),
                  cameraPositionTexture.z());
      glUniform1f(stepSizeLocation, stepSize);

      auto* extraFunctions = QOpenGLContext::currentContext()->extraFunctions();
      extraFunctions->glBindVertexArray(volumeVao_);
      glDrawArrays(GL_TRIANGLES, 0, 36);
      extraFunctions->glBindVertexArray(0);

      glUseProgram(0);
      glBindTexture(GL_TEXTURE_3D, 0);
      glActiveTexture(static_cast<GLenum>(previousActiveTexture));
      glDisable(GL_CULL_FACE);
    }
  }

  GLint previousDepthFunc = GL_LESS;
  glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);
  glDepthFunc(GL_LEQUAL);

  glUseProgram(shaderProgram_);
  const GLint mvpLocation = glGetUniformLocation(shaderProgram_, "mvpMatrix");
  glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, mvpMatrix.constData());

  auto* extraFunctions = QOpenGLContext::currentContext()->extraFunctions();
  extraFunctions->glBindVertexArray(vao_);
  glDrawArrays(GL_LINES, 0, 24);
  extraFunctions->glBindVertexArray(0);

  glUseProgram(0);
  glDepthFunc(static_cast<GLenum>(previousDepthFunc));
}

QVector3D OpenGLVolumeRendererWidget::mapToArcball(const QPoint& position) const
{
  return mapToArcballPoint(position, width(), height());
}

QQuaternion OpenGLVolumeRendererWidget::rotationBetweenVectors(const QVector3D& from,
                                                               const QVector3D& to) const
{
  return ::rotationBetweenVectors(from, to);
}

void OpenGLVolumeRendererWidget::uploadVolumeTextureIfNeeded()
{
  if (!volumeTextureDirty_)
  {
    return;
  }

  volumeTextureDirty_ = false;
  destroyVolumeTexture();

  if (!currentVolume_ || !currentVolume_->isValid())
  {
    volumeTextureReady_ = false;
    return;
  }

  const std::size_t width = currentVolume_->width();
  const std::size_t height = currentVolume_->height();
  const std::size_t depth = currentVolume_->depth();

  auto emitFailure = [this](const QString& message) {
    volumeTextureReady_ = false;
    emit volumeTextureUploadFailed(message);
  };

  if (width == 0 || height == 0 || depth == 0)
  {
    emitFailure(QStringLiteral("Volume dimensions must be positive"));
    return;
  }

  if (width > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
      height > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
      depth > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
  {
    emitFailure(QStringLiteral("Volume dimensions exceed GLsizei limits"));
    return;
  }

  GLint maxTextureSize = 0;
  glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &maxTextureSize);
  if (maxTextureSize <= 0)
  {
    emitFailure(QStringLiteral("Unable to query GL_MAX_3D_TEXTURE_SIZE"));
    return;
  }

  if (width > static_cast<std::size_t>(maxTextureSize) ||
      height > static_cast<std::size_t>(maxTextureSize) ||
      depth > static_cast<std::size_t>(maxTextureSize))
  {
    emitFailure(QString("Volume exceeds GPU 3D texture limit (%1)").arg(maxTextureSize));
    return;
  }

  const auto& voxels = currentVolume_->voxels();
  if (voxels.size() != width * height * depth)
  {
    emitFailure(QStringLiteral("Voxel buffer size does not match volume dimensions"));
    return;
  }

  const auto [minIt, maxIt] = std::minmax_element(voxels.begin(), voxels.end());
  const float minimum = *minIt;
  const float maximum = *maxIt;

  std::vector<float> normalizedVoxels(voxels.size(), 0.0F);
  if (maximum > minimum)
  {
    const float range = maximum - minimum;
    for (std::size_t index = 0; index < voxels.size(); ++index)
    {
      const float normalized = (voxels[index] - minimum) / range;
      normalizedVoxels[index] = std::clamp(normalized, 0.0F, 1.0F);
    }
  }

  while (glGetError() != GL_NO_ERROR)
  {
  }

  glGenTextures(1, &volumeTextureId_);
  if (volumeTextureId_ == 0)
  {
    emitFailure(QStringLiteral("Failed to allocate 3D texture"));
    return;
  }

  glBindTexture(GL_TEXTURE_3D, volumeTextureId_);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  GLint previousUnpackAlignment = 0;
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  auto* extraFunctions = QOpenGLContext::currentContext()->extraFunctions();
  extraFunctions->glTexImage3D(GL_TEXTURE_3D,
                               0,
                               GL_R32F,
                               static_cast<GLsizei>(width),
                               static_cast<GLsizei>(height),
                               static_cast<GLsizei>(depth),
                               0,
                               GL_RED,
                               GL_FLOAT,
                               normalizedVoxels.data());

  glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);

  const GLenum glError = glGetError();
  glBindTexture(GL_TEXTURE_3D, 0);

  if (glError != GL_NO_ERROR)
  {
    destroyVolumeTexture();
    emitFailure(QString("OpenGL error 0x%1 while uploading 3D texture")
                    .arg(QString::number(static_cast<unsigned int>(glError), 16).toUpper()));
    return;
  }

  volumeTextureReady_ = true;
  emit volumeTextureUploaded(static_cast<int>(width), static_cast<int>(height),
                             static_cast<int>(depth));
}

void OpenGLVolumeRendererWidget::initializeRenderingResources()
{
  static constexpr char kVertexShaderSource[] = R"(
#version 330 core
layout (location = 0) in vec3 position;

uniform mat4 mvpMatrix;

void main()
{
  gl_Position = mvpMatrix * vec4(position, 1.0);
}
)";

  static constexpr char kFragmentShaderSource[] = R"(
#version 330 core

out vec4 outputColor;

void main()
{
  outputColor = vec4(0.85, 0.86, 0.88, 1.0);
}
)";

  static constexpr char kVolumeVertexShaderSource[] = R"(
#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 mvpMatrix;

out vec3 texturePosition;

void main()
{
  texturePosition = position + vec3(0.5);
  gl_Position = mvpMatrix * vec4(position, 1.0);
}
)";

  static constexpr char kVolumeFragmentShaderSource[] = R"(
#version 330 core

in vec3 texturePosition;

out vec4 outputColor;

uniform sampler3D volumeTexture;
uniform vec3 cameraPositionTexture;
uniform float stepSize;

bool insideVolume(vec3 position)
{
  return all(greaterThanEqual(position, vec3(0.0))) &&
         all(lessThanEqual(position, vec3(1.0)));
}

void main()
{
  vec3 rayDirection =
      normalize(texturePosition - cameraPositionTexture);

  vec3 samplePosition =
      texturePosition + rayDirection * stepSize * 0.5;

  vec3 accumulatedColor = vec3(0.0);
  float accumulatedAlpha = 0.0;

  const int kMaxSteps = 2048;

  for (int step = 0; step < kMaxSteps; ++step)
  {
    if (!insideVolume(samplePosition))
    {
      break;
    }

    float sampleValue =
        texture(volumeTexture, samplePosition).r;

    float sampleIntensity =
        smoothstep(0.10, 0.80, sampleValue);

    float sampleAlpha =
        sampleIntensity * 0.08;

    accumulatedColor +=
        (1.0 - accumulatedAlpha) *
        vec3(sampleIntensity) *
        sampleAlpha;

    accumulatedAlpha +=
        (1.0 - accumulatedAlpha) *
        sampleAlpha;

    if (accumulatedAlpha >= 0.98)
    {
      break;
    }

    samplePosition += rayDirection * stepSize;
  }

  if (accumulatedAlpha <= 0.01)
  {
    discard;
  }

  outputColor =
      vec4(accumulatedColor, accumulatedAlpha);
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

  const GLuint volumeVertexShader = compileShader(GL_VERTEX_SHADER, kVolumeVertexShaderSource);
  const GLuint volumeFragmentShader = compileShader(GL_FRAGMENT_SHADER, kVolumeFragmentShaderSource);
  if (volumeVertexShader == 0 || volumeFragmentShader == 0)
  {
    glDeleteShader(volumeVertexShader);
    glDeleteShader(volumeFragmentShader);
  }
  else
  {
    volumeShaderProgram_ = glCreateProgram();
    if (volumeShaderProgram_ != 0)
    {
      glAttachShader(volumeShaderProgram_, volumeVertexShader);
      glAttachShader(volumeShaderProgram_, volumeFragmentShader);
      glLinkProgram(volumeShaderProgram_);

      GLint linkStatus = GL_FALSE;
      glGetProgramiv(volumeShaderProgram_, GL_LINK_STATUS, &linkStatus);

      glDetachShader(volumeShaderProgram_, volumeVertexShader);
      glDetachShader(volumeShaderProgram_, volumeFragmentShader);

      if (linkStatus != GL_TRUE)
      {
        glDeleteProgram(volumeShaderProgram_);
        volumeShaderProgram_ = 0;
      }
    }
    else
    {
      volumeShaderProgram_ = 0;
    }

    glDeleteShader(volumeVertexShader);
    glDeleteShader(volumeFragmentShader);
  }

  auto* extraFunctions = QOpenGLContext::currentContext()->extraFunctions();
  extraFunctions->glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);

  extraFunctions->glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<qopengl_GLsizeiptr>(sizeof(kCubeVertices)),
               kCubeVertices,
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  extraFunctions->glBindVertexArray(0);

  extraFunctions->glGenVertexArrays(1, &volumeVao_);
  glGenBuffers(1, &volumeVbo_);

  extraFunctions->glBindVertexArray(volumeVao_);
  glBindBuffer(GL_ARRAY_BUFFER, volumeVbo_);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<qopengl_GLsizeiptr>(sizeof(kVolumeCubeVertices)),
               kVolumeCubeVertices,
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  extraFunctions->glBindVertexArray(0);
}

void OpenGLVolumeRendererWidget::destroyRenderingResources()
{
  destroyVolumeTexture();

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

  if (volumeVbo_ != 0)
  {
    glDeleteBuffers(1, &volumeVbo_);
    volumeVbo_ = 0;
  }

  if (volumeVao_ != 0)
  {
    QOpenGLContext::currentContext()->extraFunctions()->glDeleteVertexArrays(1, &volumeVao_);
    volumeVao_ = 0;
  }

  if (shaderProgram_ != 0)
  {
    glDeleteProgram(shaderProgram_);
    shaderProgram_ = 0;
  }

  if (volumeShaderProgram_ != 0)
  {
    glDeleteProgram(volumeShaderProgram_);
    volumeShaderProgram_ = 0;
  }
}

void OpenGLVolumeRendererWidget::destroyVolumeTexture()
{
  if (volumeTextureId_ != 0)
  {
    glDeleteTextures(1, &volumeTextureId_);
    volumeTextureId_ = 0;
  }

  volumeTextureReady_ = false;
}

GLuint OpenGLVolumeRendererWidget::compileShader(GLenum shaderType, const char* source)
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

} // namespace qvp
