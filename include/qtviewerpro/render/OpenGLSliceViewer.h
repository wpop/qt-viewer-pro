#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

namespace qvp
{

class OpenGLSliceViewer : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
  explicit OpenGLSliceViewer(QWidget* parent = nullptr);
  ~OpenGLSliceViewer() override;

protected:
  void initializeGL() override;
  void resizeGL(int width, int height) override;
  void paintGL() override;
};

} // namespace qvp
