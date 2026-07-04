#include "qtviewerpro/render/OpenGLVolumeRendererWidget.h"

#include <QMatrix4x4>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QSizePolicy>

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
