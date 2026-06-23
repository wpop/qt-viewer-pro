#include "ImageViewer.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMimeData>
#include <QPainter>
#include <QUrl>
#include <QWheelEvent>
#include <QWidget>
#include <QPixmap>
#include <QTransform>

namespace
{
constexpr double kZoomFactor = 1.25;
}

ImageViewer::ImageViewer(QWidget *parent)
    : QGraphicsView(parent)
{
  setRenderHint(QPainter::Antialiasing);
  setDragMode(QGraphicsView::ScrollHandDrag);
  auto *graphicsScene = new QGraphicsScene(this);
  setScene(graphicsScene);
  pixmapItem_ = graphicsScene->addPixmap(QPixmap());
  setResizeAnchor(QGraphicsView::AnchorViewCenter);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

  setAcceptDrops(true);
  viewport()->setAcceptDrops(true);
}

void ImageViewer::setImage(const QImage& image)
{
  pixmapItem_->setRotation(0.0);
  pixmapItem_->setPixmap(QPixmap::fromImage(image));
  scene()->setSceneRect(pixmapItem_->boundingRect());
  fitMode_ = true;
  fitToWindow();
}

void ImageViewer::resizeEvent(QResizeEvent *event)
{
  QGraphicsView::resizeEvent(event);

  if (fitMode_)
    fitToWindow();
}

void ImageViewer::wheelEvent(QWheelEvent *event)
{
  if (event->angleDelta().y() > 0)
  {
    zoomIn();
  }
  else
  {
    zoomOut();
  }
}

void ImageViewer::fitToWindow()
{
  if (!scene() || scene()->items().isEmpty())
    return;

  fitMode_ = true;

  resetTransform();
  fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

void ImageViewer::actualSize()
{
  fitMode_ = false;
  resetTransform();
}

QSize ImageViewer::imageSize() const
{
  return pixmapItem_->pixmap().size();
}

double ImageViewer::zoomFactor() const
{
  return transform().m11();
}

void ImageViewer::zoomIn()
{
  fitMode_ = false;
  scale(kZoomFactor, kZoomFactor);
}

void ImageViewer::zoomOut()
{
  fitMode_ = false;
  scale(1.0 / kZoomFactor, 1.0 / kZoomFactor);
}

void ImageViewer::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
}

void ImageViewer::dragMoveEvent(QDragMoveEvent *event)
{
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
}

void ImageViewer::dropEvent(QDropEvent *event)
{
  const QList<QUrl> urls = event->mimeData()->urls();

  if (urls.isEmpty())
    return;

  const QString fileName = urls.first().toLocalFile();

  if (fileName.isEmpty())
    return;

  emit imageDropped(fileName);
  event->acceptProposedAction();
}

void ImageViewer::rotateLeft()
{
  pixmapItem_->setRotation(pixmapItem_->rotation() - 90.0);
  scene()->setSceneRect(pixmapItem_->sceneBoundingRect());

  if (fitMode_)
    fitToWindow();
}

void ImageViewer::rotateRight()
{
  pixmapItem_->setRotation(pixmapItem_->rotation() + 90.0);
  scene()->setSceneRect(pixmapItem_->sceneBoundingRect());

  if (fitMode_)
    fitToWindow();
}

void ImageViewer::flipHorizontal()
{
  const QPixmap pixmap = pixmapItem_->pixmap();

  QTransform transform;
  transform.scale(-1.0, 1.0);

  pixmapItem_->setPixmap(pixmap.transformed(transform));

  scene()->setSceneRect(pixmapItem_->boundingRect());

  if (fitMode_)
    fitToWindow();
}

void ImageViewer::flipVertical()
{
  const QPixmap pixmap = pixmapItem_->pixmap();

  QTransform transform;
  transform.scale(1.0, -1.0);

  pixmapItem_->setPixmap(pixmap.transformed(transform));

  scene()->setSceneRect(pixmapItem_->boundingRect());

  if (fitMode_)
    fitToWindow();
}

QImage ImageViewer::image() const
{
  if (!pixmapItem_)
    return QImage();

  return pixmapItem_->pixmap().toImage();
}
