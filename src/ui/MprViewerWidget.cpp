#include "qtviewerpro/ui/MprViewerWidget.h"

#include "qtviewerpro/core/SliceExtractor.h"
#include "qtviewerpro/processing/SliceImageConverter.h"
#include "qtviewerpro/ui/MprCoordinateMapper.h"
#include "qtviewerpro/ui/ImageViewer2D.h"

#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>
#include <QSplitter>
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
    auto resetPane = [](SlicePane& pane) {
      const QSignalBlocker blocker(pane.sliceSlider);
      pane.sliceSlider->setRange(0, 0);
      pane.sliceSlider->setValue(0);
      pane.sliceSlider->setEnabled(false);
      pane.sliceValueLabel->setText(QStringLiteral("0 / 0"));
    };

    currentVolume_.reset();
    currentPosition_ = {};
    resetPane(axialPane_);
    axialPane_.viewer->setPixelSpacing(1.0F, 1.0F);
    axialPane_.viewer->setImage(QImage());
    axialPane_.viewer->setCrosshairPosition(std::nullopt);
    axialPane_.coordinateLabel->setText(QStringLiteral("No volume loaded"));
    resetPane(sagittalPane_);
    sagittalPane_.viewer->setPixelSpacing(1.0F, 1.0F);
    sagittalPane_.viewer->setImage(QImage());
    sagittalPane_.viewer->setCrosshairPosition(std::nullopt);
    sagittalPane_.coordinateLabel->setText(QStringLiteral("No volume loaded"));
    resetPane(coronalPane_);
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

  auto* splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);
  rootLayout->addWidget(splitter, 1);

  auto createPane = [this, splitter](SlicePane& pane) {
    auto* container = new QWidget(splitter);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(6, 0, 6, 0);
    layout->setSpacing(5);

    pane.titleLabel = new QLabel(orientationName(pane.orientation), container);
    pane.coordinateLabel = new QLabel(QStringLiteral("No volume loaded"), container);
    pane.sliceSlider = new QSlider(Qt::Horizontal, container);
    pane.sliceSlider->setRange(0, 0);
    pane.sliceSlider->setValue(0);
    pane.sliceSlider->setEnabled(false);
    pane.sliceValueLabel = new QLabel(QStringLiteral("0 / 0"), container);
    pane.sliceValueLabel->setMinimumWidth(56);
    pane.sliceValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    pane.viewer = new ImageViewer2D(container);
    pane.viewer->setSliceNavigationEnabled(true);
    pane.viewer->setHoverCrosshairEnabled(false);
    pane.viewer->setImageClickEnabled(true);
    auto* navigationLayout = new QHBoxLayout();
    navigationLayout->setContentsMargins(0, 0, 0, 0);
    navigationLayout->setSpacing(8);
    navigationLayout->addWidget(new QLabel(QStringLiteral("Slice"), container));
    navigationLayout->addWidget(pane.sliceSlider, 1);
    navigationLayout->addWidget(pane.sliceValueLabel);
    layout->addWidget(pane.titleLabel);
    layout->addWidget(pane.coordinateLabel);
    layout->addWidget(pane.viewer, 1);
    layout->addLayout(navigationLayout);

    splitter->addWidget(container);
  };

  createPane(axialPane_);
  createPane(sagittalPane_);
  createPane(coronalPane_);
  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 1);
  splitter->setStretchFactor(2, 1);
}

void MprViewerWidget::connectSignals()
{
  auto connectSlider = [this](SlicePane& pane) {
    connect(pane.sliceSlider, &QSlider::valueChanged, this, [this, orientation = pane.orientation](int value) {
      if (!currentVolume_)
      {
        return;
      }

      const auto sliceIndex = static_cast<std::size_t>(value);
      switch (orientation)
      {
      case SliceOrientation::Axial:
        currentPosition_.z = sliceIndex;
        break;
      case SliceOrientation::Sagittal:
        currentPosition_.x = sliceIndex;
        break;
      case SliceOrientation::Coronal:
        currentPosition_.y = sliceIndex;
        break;
      }

      refreshAllSlices();
    });
  };

  connectSlider(axialPane_);
  connectSlider(sagittalPane_);
  connectSlider(coronalPane_);

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
  const std::size_t sliceCount = sliceCountForOrientation(pane.orientation);
  const SliceData slice = SliceExtractor::extract(*currentVolume_, pane.orientation, sliceIndex);
  const QSignalBlocker blocker(pane.sliceSlider);
  pane.sliceSlider->setRange(0, static_cast<int>(sliceCount - 1));
  pane.sliceSlider->setEnabled(true);
  pane.sliceSlider->setValue(static_cast<int>(sliceIndex));
  pane.viewer->setPixelSpacing(slice.spacingX(), slice.spacingY());
  pane.viewer->setImage(SliceImageConverter::toGrayscaleImage(slice, window_, level_));
  const MprImagePoint imagePoint =
      MprCoordinateMapper::crosshairImagePoint(pane.orientation, currentPosition_);
  pane.viewer->setCrosshairPosition(
      QPointF(static_cast<double>(imagePoint.x) + 0.5, static_cast<double>(imagePoint.y) + 0.5));
  pane.titleLabel->setText(orientationName(pane.orientation));
  pane.sliceValueLabel->setText(QStringLiteral("%1 / %2").arg(sliceIndex).arg(sliceCount - 1));
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
