#pragma once

#include <QImage>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>

namespace qvp
{

class OpenGLSliceViewer : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
  explicit OpenGLSliceViewer(QWidget* parent = nullptr);
  ~OpenGLSliceViewer() override;

  void setImage(const QImage& image);
  bool hasImage() const;

protected:
  void initializeGL() override;
  void resizeGL(int width, int height) override;
  void paintGL() override;

private:
  QImage image_;
};

} // namespace qvp
