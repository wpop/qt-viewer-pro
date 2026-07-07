#include "qtviewerpro/ui/ImageViewer2D.h"

#include <QColor>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QTransform>
#include <QUrl>
#include <QWheelEvent>
#include <QWidget>

#include <algorithm>
#include <cmath>

namespace
{
constexpr double kZoomFactor = 1.25;

double sanitizePixelSpacing(float spacing)
{
  if (!std::isfinite(spacing) || spacing <= 0.0F)
  {
    return 1.0;
  }

  return static_cast<double>(spacing);
}
}

namespace qvp
{

std::optional<QPointF> ImageViewer2D::imagePositionFromViewportPosition(const QPointF& viewportPosition) const
{
  if (!pixmapItem_ || pixmapItem_->pixmap().isNull())
  {
    return std::nullopt;
  }

  const QPointF scenePosition = mapToScene(viewportPosition.toPoint());
  const QPointF imagePosition = pixmapItem_->mapFromScene(scenePosition);
  const QSize imageSize = pixmapItem_->pixmap().size();

  if (imagePosition.x() < 0.0 || imagePosition.y() < 0.0 ||
      imagePosition.x() >= static_cast<double>(imageSize.width()) ||
      imagePosition.y() >= static_cast<double>(imageSize.height()))
  {
    return std::nullopt;
  }

  return imagePosition;
}

ImageViewer2D::ImageViewer2D(QWidget* parent) : QGraphicsView(parent)
{
  setRenderHint(QPainter::Antialiasing);
  setDragMode(QGraphicsView::ScrollHandDrag);
  auto* graphicsScene = new QGraphicsScene(this);
  setScene(graphicsScene);
  pixmapItem_ = graphicsScene->addPixmap(QPixmap());
  setResizeAnchor(QGraphicsView::AnchorViewCenter);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

  setMouseTracking(true);
  setAcceptDrops(true);
  viewport()->setMouseTracking(true);
  viewport()->setAcceptDrops(true);
}

void ImageViewer2D::setImage(const QImage& image)
{
  pixmapItem_->setRotation(0.0);
  pixmapItem_->setPixmap(QPixmap::fromImage(image));
  updatePixmapDisplayGeometry();
  updateSceneRect();
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
  const int verticalDelta = event->angleDelta().y();
  if (verticalDelta == 0)
  {
    event->ignore();
    return;
  }

  if (sliceNavigationEnabled_)
  {
    const int delta = verticalDelta > 0 ? 1 : -1;
    emit sliceNavigationRequested(delta);
    event->accept();
    return;
  }

  if (verticalDelta > 0)
  {
    zoomIn();
  }
  else
  {
    zoomOut();
  }

  event->accept();
}

void ImageViewer2D::mouseMoveEvent(QMouseEvent* event)
{
  const auto imagePosition = imagePositionFromViewportPosition(event->position());
  if (imagePosition.has_value())
  {
    currentImageMousePosition_ = imagePosition;
    viewport()->update();
    emit imageMousePositionChanged(static_cast<int>(imagePosition->x()),
                                   static_cast<int>(imagePosition->y()));
  }
  else if (currentImageMousePosition_.has_value())
  {
    currentImageMousePosition_.reset();
    viewport()->update();
  }

  QGraphicsView::mouseMoveEvent(event);
}

void ImageViewer2D::leaveEvent(QEvent* event)
{
  if (currentImageMousePosition_.has_value())
  {
    currentImageMousePosition_.reset();
    viewport()->update();
  }

  QGraphicsView::leaveEvent(event);
}

void ImageViewer2D::mousePressEvent(QMouseEvent* event)
{
  if (imageClickEnabled_ && event->button() == Qt::LeftButton)
  {
    const auto imagePosition = imagePositionFromViewportPosition(event->position());
    if (imagePosition.has_value())
    {
      emit imageClicked(static_cast<int>(imagePosition->x()),
                        static_cast<int>(imagePosition->y()));
      event->accept();
      return;
    }
  }

  QGraphicsView::mousePressEvent(event);
}

void ImageViewer2D::drawForeground(QPainter* painter, const QRectF& rect)
{
  QGraphicsView::drawForeground(painter, rect);

  if (!pixmapItem_ || pixmapItem_->pixmap().isNull())
  {
    return;
  }

  if (persistentCrosshairPosition_.has_value())
  {
    drawCrosshair(painter, persistentCrosshairPosition_.value());
  }

  if (hoverCrosshairEnabled_ && currentImageMousePosition_.has_value())
  {
    drawCrosshair(painter, currentImageMousePosition_.value());
  }
}

void ImageViewer2D::drawCrosshair(QPainter* painter, const QPointF& imagePosition) const
{
  const QRectF imageBounds = pixmapItem_->boundingRect();
  const QPointF top = pixmapItem_->mapToScene(QPointF(imagePosition.x(), imageBounds.top()));
  const QPointF bottom = pixmapItem_->mapToScene(QPointF(imagePosition.x(), imageBounds.bottom()));
  const QPointF left = pixmapItem_->mapToScene(QPointF(imageBounds.left(), imagePosition.y()));
  const QPointF right = pixmapItem_->mapToScene(QPointF(imageBounds.right(), imagePosition.y()));

  QPen outlinePen(QColor(0, 0, 0, 100));
  outlinePen.setWidth(2);
  outlinePen.setCosmetic(true);
  painter->setPen(outlinePen);
  painter->drawLine(top, bottom);
  painter->drawLine(left, right);

  QPen innerPen(QColor(255, 255, 255, 230));
  innerPen.setWidth(1);
  innerPen.setCosmetic(true);
  painter->setPen(innerPen);
  painter->drawLine(top, bottom);
  painter->drawLine(left, right);
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

void ImageViewer2D::setSliceNavigationEnabled(bool enabled)
{
  sliceNavigationEnabled_ = enabled;
}

void ImageViewer2D::setHoverCrosshairEnabled(bool enabled)
{
  hoverCrosshairEnabled_ = enabled;
  if (!hoverCrosshairEnabled_ && currentImageMousePosition_.has_value())
  {
    currentImageMousePosition_.reset();
  }
  viewport()->update();
}

void ImageViewer2D::setCrosshairPosition(const std::optional<QPointF>& position)
{
  persistentCrosshairPosition_ = position;
  viewport()->update();
}

void ImageViewer2D::setImageClickEnabled(bool enabled)
{
  imageClickEnabled_ = enabled;
}

void ImageViewer2D::setPixelSpacing(float spacingX, float spacingY)
{
  if (pixelSpacingX_ == spacingX && pixelSpacingY_ == spacingY)
  {
    return;
  }

  pixelSpacingX_ = spacingX;
  pixelSpacingY_ = spacingY;
  updatePixmapDisplayGeometry();
  updateSceneRect();

  if (fitMode_)
  {
    fitToWindow();
  }
}

void ImageViewer2D::updatePixmapDisplayGeometry()
{
  const double spacingX = sanitizePixelSpacing(pixelSpacingX_);
  const double spacingY = sanitizePixelSpacing(pixelSpacingY_);
  const double baseSpacing = std::min(spacingX, spacingY);
  pixmapItem_->setTransform(
      QTransform::fromScale(spacingX / baseSpacing, spacingY / baseSpacing));
}

void ImageViewer2D::updateSceneRect()
{
  if (!scene())
  {
    return;
  }

  scene()->setSceneRect(pixmapItem_->sceneBoundingRect());
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
  updateSceneRect();

  if (fitMode_)
    fitToWindow();
}

void ImageViewer2D::rotateRight()
{
  pixmapItem_->setRotation(pixmapItem_->rotation() + 90.0);
  updateSceneRect();

  if (fitMode_)
    fitToWindow();
}

void ImageViewer2D::flipHorizontal()
{
  const QPixmap pixmap = pixmapItem_->pixmap();

  QTransform transform;
  transform.scale(-1.0, 1.0);

  pixmapItem_->setPixmap(pixmap.transformed(transform));
  updatePixmapDisplayGeometry();
  updateSceneRect();

  if (fitMode_)
    fitToWindow();
}

void ImageViewer2D::flipVertical()
{
  const QPixmap pixmap = pixmapItem_->pixmap();

  QTransform transform;
  transform.scale(1.0, -1.0);

  pixmapItem_->setPixmap(pixmap.transformed(transform));
  updatePixmapDisplayGeometry();
  updateSceneRect();

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
