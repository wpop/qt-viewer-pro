#pragma once

#include <QGraphicsView>
#include <QImage>
#include <QSize>
#include <QString>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QGraphicsPixmapItem;
class QResizeEvent;
class QWheelEvent;

class ImageViewer : public QGraphicsView
{
  Q_OBJECT

public:
  explicit ImageViewer(QWidget *parent = nullptr);

  void setImage(const QImage& image);
  QImage image() const;

  void fitToWindow();
  void actualSize();
  void zoomIn();
  void zoomOut();

  void rotateLeft();
  void rotateRight();
  void flipHorizontal();
  void flipVertical();

  QSize imageSize() const;
  double zoomFactor() const;

protected:
  void resizeEvent(QResizeEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

signals:
  void imageDropped(const QString& fileName);

private:
  QGraphicsPixmapItem *pixmapItem_ = nullptr;
  bool fitMode_ = true;
};
