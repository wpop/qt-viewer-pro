#include "qtviewerpro/render/OpenGLVolumeRendererWidget.h"

#include "qtviewerpro/core/VolumeData.h"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QSizePolicy>
#include <QString>
#include <QMatrix4x4>

#include <cstddef>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace
{
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
  view.translate(0.0F, 0.0F, -2.5F);

  QMatrix4x4 model;
  model.rotate(28.0F, 1.0F, 0.0F, 0.0F);
  model.rotate(38.0F, 0.0F, 1.0F, 0.0F);

  const QMatrix4x4 mvpMatrix = projection * view * model;

  glUseProgram(shaderProgram_);
  const GLint mvpLocation = glGetUniformLocation(shaderProgram_, "mvpMatrix");
  glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, mvpMatrix.constData());

  auto* extraFunctions = QOpenGLContext::currentContext()->extraFunctions();
  extraFunctions->glBindVertexArray(vao_);
  glDrawArrays(GL_LINES, 0, 24);
  extraFunctions->glBindVertexArray(0);

  glUseProgram(0);
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
               static_cast<qopengl_GLsizeiptr>(sizeof(kCubeVertices)),
               kCubeVertices,
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

  if (shaderProgram_ != 0)
  {
    glDeleteProgram(shaderProgram_);
    shaderProgram_ = 0;
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
