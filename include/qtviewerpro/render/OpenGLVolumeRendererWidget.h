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

protected:
  void initializeGL() override;
  void resizeGL(int width, int height) override;
  void paintGL() override;
};

} // namespace qvp
