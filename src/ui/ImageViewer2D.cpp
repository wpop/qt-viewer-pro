#include "qtviewerpro/ui/ImageViewer2D.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QTransform>
#include <QUrl>
#include <QWheelEvent>
#include <QWidget>

namespace
{
constexpr double kZoomFactor = 1.25;
}

namespace qvp
{

ImageViewer2D::ImageViewer2D(QWidget* parent) : QGraphicsView(parent)
{
  setRenderHint(QPainter::Antialiasing);
  setDragMode(QGraphicsView::ScrollHandDrag);
  auto* graphicsScene = new QGraphicsScene(this);
  setScene(graphicsScene);
  pixmapItem_ = graphicsScene->addPixmap(QPixmap());
  setResizeAnchor(QGraphicsView::AnchorViewCenter);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

  setAcceptDrops(true);
  viewport()->setAcceptDrops(true);
}

void ImageViewer2D::setImage(const QImage& image)
{
  pixmapItem_->setRotation(0.0);
  pixmapItem_->setPixmap(QPixmap::fromImage(image));
  scene()->setSceneRect(pixmapItem_->boundingRect());
  fitMode_ = true;
  fitToWindow();
}

void ImageViewer2D::resizeEvent(QResizeEvent* event)
{
  QGraphicsView::resizeEvent(event);

  if (fitMode_)
    fitToWindow();
}

void ImageViewer2D::wheelEvent(QWheelEvent* event)
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

void ImageViewer2D::fitToWindow()
{
  if (!scene() || scene()->items().isEmpty())
    return;

  fitMode_ = true;

  resetTransform();
  fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

void ImageViewer2D::actualSize()
{
  fitMode_ = false;
  resetTransform();
}

QSize ImageViewer2D::imageSize() const
{
  return pixmapItem_->pixmap().size();
}

double ImageViewer2D::zoomFactor() const
{
  return transform().m11();
}

void ImageViewer2D::zoomIn()
{
  fitMode_ = false;
  scale(kZoomFactor, kZoomFactor);
}

void ImageViewer2D::zoomOut()
{
  fitMode_ = false;
  scale(1.0 / kZoomFactor, 1.0 / kZoomFactor);
}

void ImageViewer2D::dragEnterEvent(QDragEnterEvent* event)
{
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
}

void ImageViewer2D::dragMoveEvent(QDragMoveEvent* event)
{
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
}

void ImageViewer2D::dropEvent(QDropEvent* event)
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

void ImageViewer2D::rotateLeft()
{
  pixmapItem_->setRotation(pixmapItem_->rotation() - 90.0);
  scene()->setSceneRect(pixmapItem_->sceneBoundingRect());

  if (fitMode_)
    fitToWindow();
}

void ImageViewer2D::rotateRight()
{
  pixmapItem_->setRotation(pixmapItem_->rotation() + 90.0);
  scene()->setSceneRect(pixmapItem_->sceneBoundingRect());

  if (fitMode_)
    fitToWindow();
}

void ImageViewer2D::flipHorizontal()
{
  const QPixmap pixmap = pixmapItem_->pixmap();

  QTransform transform;
  transform.scale(-1.0, 1.0);

  pixmapItem_->setPixmap(pixmap.transformed(transform));

  scene()->setSceneRect(pixmapItem_->boundingRect());

  if (fitMode_)
    fitToWindow();
}

void ImageViewer2D::flipVertical()
{
  const QPixmap pixmap = pixmapItem_->pixmap();

  QTransform transform;
  transform.scale(1.0, -1.0);

  pixmapItem_->setPixmap(pixmap.transformed(transform));

  scene()->setSceneRect(pixmapItem_->boundingRect());

  if (fitMode_)
    fitToWindow();
}

QImage ImageViewer2D::image() const
{
  if (!pixmapItem_)
    return QImage();

  return pixmapItem_->pixmap().toImage();
}

} // namespace qvp
