#include "qtviewerpro/ui/MprViewerWidget.h"

#include "qtviewerpro/core/SliceExtractor.h"
#include "qtviewerpro/processing/SliceImageConverter.h"
#include "qtviewerpro/ui/MprCoordinateMapper.h"
#include "qtviewerpro/ui/ImageViewer2D.h"

#include <QGridLayout>
#include <QImage>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{

bool looksLikeCtVolume(const qvp::VolumeData& volume)
{
  return volume.isValid() && volume.hasIntensityRange() && volume.intensityMinimum() < -500.0F &&
         volume.intensityMaximum() > 500.0F;
}

QString orientationName(qvp::SliceOrientation orientation)
{
  switch (orientation)
  {
  case qvp::SliceOrientation::Axial:
    return QStringLiteral("Axial");
  case qvp::SliceOrientation::Sagittal:
    return QStringLiteral("Sagittal");
  case qvp::SliceOrientation::Coronal:
    return QStringLiteral("Coronal");
  }

  return QStringLiteral("Unknown");
}

} // namespace

namespace qvp
{

MprViewerWidget::MprViewerWidget(QWidget* parent) : QWidget(parent)
{
  createUi();
  connectSignals();
}

void MprViewerWidget::setVolume(VolumeData volume)
{
  setVolume(std::make_shared<const VolumeData>(std::move(volume)));
}

void MprViewerWidget::setVolume(std::shared_ptr<const VolumeData> volume)
{
  if (!volume || !volume->isValid())
  {
    currentVolume_.reset();
    currentPosition_ = {};
    axialPane_.viewer->setPixelSpacing(1.0F, 1.0F);
    axialPane_.viewer->setImage(QImage());
    axialPane_.viewer->setCrosshairPosition(std::nullopt);
    axialPane_.coordinateLabel->setText(QStringLiteral("No volume loaded"));
    sagittalPane_.viewer->setPixelSpacing(1.0F, 1.0F);
    sagittalPane_.viewer->setImage(QImage());
    sagittalPane_.viewer->setCrosshairPosition(std::nullopt);
    sagittalPane_.coordinateLabel->setText(QStringLiteral("No volume loaded"));
    coronalPane_.viewer->setPixelSpacing(1.0F, 1.0F);
    coronalPane_.viewer->setImage(QImage());
    coronalPane_.viewer->setCrosshairPosition(std::nullopt);
    coronalPane_.coordinateLabel->setText(QStringLiteral("No volume loaded"));
    axialPane_.titleLabel->setText(QStringLiteral("Axial"));
    sagittalPane_.titleLabel->setText(QStringLiteral("Sagittal"));
    coronalPane_.titleLabel->setText(QStringLiteral("Coronal"));
    return;
  }

  currentVolume_ = std::move(volume);
  currentPosition_.x = currentVolume_->width() / 2;
  currentPosition_.y = currentVolume_->height() / 2;
  currentPosition_.z = currentVolume_->depth() / 2;
  setDefaultWindowLevel();
  refreshAllSlices();
}

void MprViewerWidget::createUi()
{
  axialPane_.orientation = SliceOrientation::Axial;
  sagittalPane_.orientation = SliceOrientation::Sagittal;
  coronalPane_.orientation = SliceOrientation::Coronal;

  auto* rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(8, 8, 8, 8);
  rootLayout->setSpacing(8);

  auto* gridLayout = new QGridLayout();
  gridLayout->setContentsMargins(0, 0, 0, 0);
  gridLayout->setHorizontalSpacing(8);
  gridLayout->setVerticalSpacing(8);
  rootLayout->addLayout(gridLayout, 1);

  auto createPane = [this, gridLayout](SlicePane& pane, int row, int column) {
    auto* container = new QWidget(this);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    pane.titleLabel = new QLabel(orientationName(pane.orientation), container);
    pane.coordinateLabel = new QLabel(QStringLiteral("No volume loaded"), container);
    pane.viewer = new ImageViewer2D(container);
    pane.viewer->setSliceNavigationEnabled(true);
    pane.viewer->setHoverCrosshairEnabled(false);
    pane.viewer->setImageClickEnabled(true);
    layout->addWidget(pane.titleLabel);
    layout->addWidget(pane.coordinateLabel);
    layout->addWidget(pane.viewer, 1);

    gridLayout->addWidget(container, row, column);
  };

  createPane(axialPane_, 0, 0);
  createPane(sagittalPane_, 0, 1);
  createPane(coronalPane_, 1, 0);
  gridLayout->setColumnStretch(0, 1);
  gridLayout->setColumnStretch(1, 1);
  gridLayout->setRowStretch(0, 1);
  gridLayout->setRowStretch(1, 1);
}

void MprViewerWidget::connectSignals()
{
  connect(axialPane_.viewer, &ImageViewer2D::sliceNavigationRequested, this, [this](int delta) {
    updatePositionForOrientation(SliceOrientation::Axial, delta);
  });
  connect(axialPane_.viewer, &ImageViewer2D::imageClicked, this, [this](int x, int y) {
    updatePositionFromImageClick(SliceOrientation::Axial, x, y);
  });
  connect(sagittalPane_.viewer, &ImageViewer2D::sliceNavigationRequested, this, [this](int delta) {
    updatePositionForOrientation(SliceOrientation::Sagittal, delta);
  });
  connect(sagittalPane_.viewer, &ImageViewer2D::imageClicked, this, [this](int x, int y) {
    updatePositionFromImageClick(SliceOrientation::Sagittal, x, y);
  });
  connect(coronalPane_.viewer, &ImageViewer2D::sliceNavigationRequested, this, [this](int delta) {
    updatePositionForOrientation(SliceOrientation::Coronal, delta);
  });
  connect(coronalPane_.viewer, &ImageViewer2D::imageClicked, this, [this](int x, int y) {
    updatePositionFromImageClick(SliceOrientation::Coronal, x, y);
  });
}

void MprViewerWidget::refreshAllSlices()
{
  if (!currentVolume_)
  {
    return;
  }

  refreshSlicePane(axialPane_);
  refreshSlicePane(sagittalPane_);
  refreshSlicePane(coronalPane_);
}

void MprViewerWidget::refreshSlicePane(SlicePane& pane)
{
  const std::size_t sliceIndex = currentSliceIndexForOrientation(pane.orientation);
  const SliceData slice = SliceExtractor::extract(*currentVolume_, pane.orientation, sliceIndex);
  pane.viewer->setPixelSpacing(slice.spacingX(), slice.spacingY());
  pane.viewer->setImage(SliceImageConverter::toGrayscaleImage(slice, window_, level_));
  const MprImagePoint imagePoint =
      MprCoordinateMapper::crosshairImagePoint(pane.orientation, currentPosition_);
  pane.viewer->setCrosshairPosition(
      QPointF(static_cast<double>(imagePoint.x) + 0.5, static_cast<double>(imagePoint.y) + 0.5));
  pane.titleLabel->setText(QStringLiteral("%1  slice %2 / %3")
                               .arg(orientationName(pane.orientation))
                               .arg(sliceIndex)
                               .arg(sliceCountForOrientation(pane.orientation) - 1));
  pane.coordinateLabel->setText(QStringLiteral("x=%1  y=%2  z=%3")
                                    .arg(currentPosition_.x)
                                    .arg(currentPosition_.y)
                                    .arg(currentPosition_.z));
}

void MprViewerWidget::setDefaultWindowLevel()
{
  if (!currentVolume_ || !currentVolume_->hasIntensityRange())
  {
    window_ = 255.0F;
    level_ = 127.0F;
    return;
  }

  if (looksLikeCtVolume(*currentVolume_))
  {
    window_ = 1500.0F;
    level_ = -600.0F;
    return;
  }

  const float minimum = currentVolume_->intensityMinimum();
  const float maximum = currentVolume_->intensityMaximum();
  window_ = std::max(1.0F, maximum - minimum);
  level_ = minimum + (window_ / 2.0F);
}

void MprViewerWidget::updatePositionForOrientation(SliceOrientation orientation, int delta)
{
  if (!currentVolume_ || delta == 0)
  {
    return;
  }

  auto adjustCoordinate = [delta](std::size_t& coordinate, std::size_t count) {
    if (count == 0)
    {
      return;
    }

    const long long updated =
        static_cast<long long>(coordinate) + static_cast<long long>(delta);
    const long long clamped =
        std::clamp(updated, 0LL, static_cast<long long>(count) - 1LL);
    coordinate = static_cast<std::size_t>(clamped);
  };

  switch (orientation)
  {
  case SliceOrientation::Axial:
    adjustCoordinate(currentPosition_.z, currentVolume_->depth());
    break;
  case SliceOrientation::Sagittal:
    adjustCoordinate(currentPosition_.x, currentVolume_->width());
    break;
  case SliceOrientation::Coronal:
    adjustCoordinate(currentPosition_.y, currentVolume_->height());
    break;
  }

  refreshAllSlices();
}

void MprViewerWidget::updatePositionFromImageClick(SliceOrientation orientation,
                                                   int imageX,
                                                   int imageY)
{
  if (!currentVolume_ || imageX < 0 || imageY < 0)
  {
    return;
  }

  currentPosition_ =
      MprCoordinateMapper::voxelPositionFromImagePoint(*currentVolume_,
                                                       orientation,
                                                       static_cast<std::size_t>(imageX),
                                                       static_cast<std::size_t>(imageY),
                                                       currentPosition_);
  refreshAllSlices();
}

std::size_t MprViewerWidget::sliceCountForOrientation(SliceOrientation orientation) const
{
  if (!currentVolume_)
  {
    return 0;
  }

  switch (orientation)
  {
  case SliceOrientation::Axial:
    return currentVolume_->depth();
  case SliceOrientation::Sagittal:
    return currentVolume_->width();
  case SliceOrientation::Coronal:
    return currentVolume_->height();
  }

  throw std::invalid_argument("Unknown MPR orientation");
}

std::size_t MprViewerWidget::currentSliceIndexForOrientation(SliceOrientation orientation) const
{
  switch (orientation)
  {
  case SliceOrientation::Axial:
    return currentPosition_.z;
  case SliceOrientation::Sagittal:
    return currentPosition_.x;
  case SliceOrientation::Coronal:
    return currentPosition_.y;
  }

  throw std::invalid_argument("Unknown MPR orientation");
}

} // namespace qvp
