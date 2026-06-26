#pragma once

#include <QGraphicsView>
#include <QImage>
#include <QSize>
#include <QString>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QGraphicsPixmapItem;
class QMouseEvent;
class QResizeEvent;
class QWheelEvent;

namespace qvp
{

/**
 * @brief Graphics-view based widget for displaying and manipulating images.
 *
 * ImageViewer2D is responsible for presentation-level image operations such as
 * zooming, fitting, rotating, flipping, and accepting dropped image files.
 */
class ImageViewer2D : public QGraphicsView
{
  Q_OBJECT

public:
  /**
   * @brief Constructs an image viewer widget.
   * @param parent Optional Qt parent widget.
   */
  explicit ImageViewer2D(QWidget* parent = nullptr);

  /**
   * @brief Replaces the displayed image and fits it to the current view.
   * @param image Image to display. A null image clears the current pixmap.
   */
  void setImage(const QImage& image);

  /**
   * @brief Returns the currently displayed image.
   * @return Current image, or a null image when nothing is displayed.
   */
  QImage image() const;

  /**
   * @brief Scales the displayed image to fit within the viewport.
   */
  void fitToWindow();

  /**
   * @brief Resets the view transform so the image is shown at native scale.
   */
  void actualSize();

  /**
   * @brief Increases the current zoom level.
   */
  void zoomIn();

  /**
   * @brief Decreases the current zoom level.
   */
  void zoomOut();

  /**
   * @brief Rotates the displayed image 90 degrees counterclockwise.
   */
  void rotateLeft();

  /**
   * @brief Rotates the displayed image 90 degrees clockwise.
   */
  void rotateRight();

  /**
   * @brief Mirrors the displayed image horizontally.
   */
  void flipHorizontal();

  /**
   * @brief Mirrors the displayed image vertically.
   */
  void flipVertical();

  /**
   * @brief Returns the pixel dimensions of the displayed image.
   * @return Current image size, or an empty size when no image is displayed.
   */
  QSize imageSize() const;

  /**
   * @brief Returns the current view zoom factor.
   * @return Horizontal scale factor from the active view transform.
   */
  double zoomFactor() const;

protected:
  void mouseMoveEvent(QMouseEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragMoveEvent(QDragMoveEvent* event) override;
  void dropEvent(QDropEvent* event) override;

signals:
  /**
   * @brief Emitted when a local image file is dropped onto the viewer.
   * @param fileName Local file path from the drop event.
   */
  void imageDropped(const QString& fileName);

  /**
   * @brief Emitted when the mouse moves over a displayed image pixel.
   * @param x Image pixel coordinate along the horizontal axis.
   * @param y Image pixel coordinate along the vertical axis.
   */
  void imageMousePositionChanged(int x, int y);

private:
  QGraphicsPixmapItem* pixmapItem_ = nullptr;
  bool fitMode_ = true;
};

} // namespace qvp
