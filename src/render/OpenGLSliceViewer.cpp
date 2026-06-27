#include "qtviewerpro/render/OpenGLSliceViewer.h"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

namespace qvp
{

OpenGLSliceViewer::OpenGLSliceViewer(QWidget* parent) : QOpenGLWidget(parent)
{
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
  textureDirty_ = true;
  update();
}

bool OpenGLSliceViewer::hasImage() const
{
  return !image_.isNull();
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

  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
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

  static constexpr float kQuadVertices[] = {
      -1.0F, -1.0F, 0.0F, 1.0F,
       1.0F, -1.0F, 1.0F, 1.0F,
       1.0F,  1.0F, 1.0F, 0.0F,
      -1.0F, -1.0F, 0.0F, 1.0F,
       1.0F,  1.0F, 1.0F, 0.0F,
      -1.0F,  1.0F, 0.0F, 0.0F,
  };

  auto* extraFunctions = QOpenGLContext::currentContext()->extraFunctions();
  extraFunctions->glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);

  extraFunctions->glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<qopengl_GLsizeiptr>(sizeof(kQuadVertices)),
               kQuadVertices,
               GL_STATIC_DRAW);

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
