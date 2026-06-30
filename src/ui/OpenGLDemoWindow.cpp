#include "qtviewerpro/ui/OpenGLDemoWindow.h"

#include "qtviewerpro/core/SliceExtractor.h"
#include "qtviewerpro/io/MedicalVolumeLoaderRegistry.h"
#include "qtviewerpro/io/RawVolumeLoader.h"
#include "qtviewerpro/processing/SliceImageConverter.h"
#include "qtviewerpro/render/OpenGLSliceViewer.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <exception>
#include <utility>
#include <vector>

namespace qvp
{

namespace
{

QString orientationName(SliceOrientation orientation)
{
  switch (orientation)
  {
  case SliceOrientation::Coronal:
    return QStringLiteral("Coronal");
  case SliceOrientation::Sagittal:
    return QStringLiteral("Sagittal");
  case SliceOrientation::Axial:
    return QStringLiteral("Axial");
  }

  return QStringLiteral("Axial");
}

} // namespace

OpenGLDemoWindow::OpenGLDemoWindow(QWidget* parent) : QWidget(parent)
{
  setWindowTitle("OpenGL Slice Viewer Demo");
  resize(640, 480);

  createUi();
  connectSignals();
  loadInitialDemoImage();
}

void OpenGLDemoWindow::setVolume(VolumeData volume)
{
  if (!volume.isValid())
  {
    QMessageBox::warning(this, "Volume Load Error", "Loaded volume data is invalid.");
    return;
  }

  currentVolume_ = std::move(volume);
  maskVolume_.reset();
  {
    const QSignalBlocker blocker(showMaskOverlayCheckBox_);
    showMaskOverlayCheckBox_->setChecked(false);
  }
  const auto [minIt, maxIt] =
      std::minmax_element(currentVolume_->voxels().begin(), currentVolume_->voxels().end());
  currentVolumeRange_ = std::make_pair(*minIt, *maxIt);
  applyCtWindowLevelPresetIfNeeded(currentVolume_.value());
  resetToAxialMiddleSlice();
  updateVolumeSlice();
  openGLViewer_->resetView();
}

void OpenGLDemoWindow::createUi()
{
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);

  auto* fileLayout = new QHBoxLayout();
  fileLayout->setSpacing(4);
  auto* sliceLayout = new QHBoxLayout();
  sliceLayout->setSpacing(4);
  auto* windowLevelLayout = new QHBoxLayout();
  windowLevelLayout->setSpacing(4);
  auto* viewLayout = new QHBoxLayout();
  viewLayout->setSpacing(4);
  auto* overlayLayout = new QHBoxLayout();
  overlayLayout->setSpacing(4);
  auto* statusLayout = new QHBoxLayout();
  statusLayout->setSpacing(8);

  openImageButton_ = new QPushButton("Open Image...", this);
  openMaskOverlayButton_ = new QPushButton("Open Mask Overlay...", this);
  loadSyntheticSliceButton_ = new QPushButton("Load Synthetic Slice", this);
  loadRawSliceButton_ = new QPushButton("Load RAW Slice", this);
  resetViewButton_ = new QPushButton("Reset View", this);
  resetCrosshairButton_ = new QPushButton("Reset Crosshair", this);
  showCrosshairCheckBox_ = new QCheckBox("Show Crosshair", this);
  showImageBorderCheckBox_ = new QCheckBox("Show Image Border", this);
  showMaskOverlayCheckBox_ = new QCheckBox("Show Mask Overlay", this);
  showCrosshairCheckBox_->setChecked(true);
  showImageBorderCheckBox_->setChecked(true);
  showMaskOverlayCheckBox_->setChecked(false);

  orientationComboBox_ = new QComboBox(this);
  orientationComboBox_->addItems({"Axial", "Coronal", "Sagittal"});

  windowSpinBox_ = new QSpinBox(this);
  windowSpinBox_->setRange(1, 4096);
  windowSpinBox_->setValue(255);
  levelSpinBox_ = new QSpinBox(this);
  levelSpinBox_->setRange(-2048, 4096);
  levelSpinBox_->setValue(127);
  resetWindowLevelButton_ = new QPushButton("Reset W/L", this);
  windowLevelPresetComboBox_ = new QComboBox(this);
  windowLevelPresetComboBox_->addItems({"Preset", "Soft Tissue", "Lung", "Bone", "Reset"});

  previousSliceButton_ = new QPushButton("Z-", this);
  sliceSlider_ = new QSlider(Qt::Horizontal, this);
  sliceSlider_->setRange(0, 0);
  sliceSlider_->setValue(0);
  sliceIndexLabel_ = new QLabel("Slice: - / -", this);
  crosshairPositionLabel_ = new QLabel("Crosshair: x=0.000 y=0.000", this);
  volumeMetadataLabel_ = new QLabel("No volume loaded", this);
  volumeMetadataLabel_->setTextFormat(Qt::PlainText);
  volumeMetadataLabel_->setWordWrap(true);
  volumeMetadataLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  nextSliceButton_ = new QPushButton("Z+", this);

  fileLayout->addWidget(openImageButton_);
  fileLayout->addWidget(openMaskOverlayButton_);
  fileLayout->addWidget(loadSyntheticSliceButton_);
  fileLayout->addWidget(loadRawSliceButton_);
  fileLayout->addStretch();
  layout->addLayout(fileLayout);

  sliceLayout->addWidget(new QLabel("Orientation", this));
  sliceLayout->addWidget(orientationComboBox_);
  sliceLayout->addWidget(previousSliceButton_);
  sliceLayout->addWidget(sliceSlider_);
  sliceLayout->addWidget(nextSliceButton_);
  sliceLayout->addStretch();
  layout->addLayout(sliceLayout);

  windowLevelLayout->addWidget(new QLabel("Window", this));
  windowLevelLayout->addWidget(windowSpinBox_);
  windowLevelLayout->addWidget(new QLabel("Level", this));
  windowLevelLayout->addWidget(levelSpinBox_);
  windowLevelLayout->addWidget(resetWindowLevelButton_);
  windowLevelLayout->addWidget(windowLevelPresetComboBox_);
  windowLevelLayout->addStretch();
  layout->addLayout(windowLevelLayout);

  viewLayout->addWidget(resetViewButton_);
  viewLayout->addWidget(resetCrosshairButton_);
  viewLayout->addStretch();
  layout->addLayout(viewLayout);

  overlayLayout->addWidget(showCrosshairCheckBox_);
  overlayLayout->addWidget(showImageBorderCheckBox_);
  overlayLayout->addWidget(showMaskOverlayCheckBox_);
  overlayLayout->addStretch();
  layout->addLayout(overlayLayout);

  layout->addWidget(volumeMetadataLabel_);

  openGLViewer_ = new OpenGLSliceViewer(this);
  layout->addWidget(openGLViewer_);
  layout->setStretchFactor(openGLViewer_, 1);

  statusLayout->addWidget(sliceIndexLabel_);
  statusLayout->addStretch();
  statusLayout->addWidget(crosshairPositionLabel_);
  layout->addLayout(statusLayout);
}

void OpenGLDemoWindow::connectSignals()
{
  connect(showCrosshairCheckBox_, &QCheckBox::toggled, openGLViewer_,
          &OpenGLSliceViewer::setCrosshairVisible);
  connect(showImageBorderCheckBox_, &QCheckBox::toggled, openGLViewer_,
          &OpenGLSliceViewer::setImageBorderVisible);
  connect(openImageButton_, &QPushButton::clicked, this, &OpenGLDemoWindow::openImage);
  connect(openMaskOverlayButton_, &QPushButton::clicked, this, &OpenGLDemoWindow::openMaskOverlay);
  connect(loadSyntheticSliceButton_, &QPushButton::clicked, this,
          &OpenGLDemoWindow::loadSyntheticSlice);
  connect(loadRawSliceButton_, &QPushButton::clicked, this, &OpenGLDemoWindow::loadRawSlice);
  connect(sliceSlider_, &QSlider::valueChanged, this, &OpenGLDemoWindow::setSliceIndex);
  connect(orientationComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this,
          &OpenGLDemoWindow::setSliceOrientation);
  connect(previousSliceButton_, &QPushButton::clicked, this, &OpenGLDemoWindow::previousSlice);
  connect(nextSliceButton_, &QPushButton::clicked, this, &OpenGLDemoWindow::nextSlice);
  connect(windowSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this,
          &OpenGLDemoWindow::updateWindowLevel);
  connect(levelSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this,
          &OpenGLDemoWindow::updateWindowLevel);
  connect(windowLevelPresetComboBox_, qOverload<int>(&QComboBox::activated), this,
          &OpenGLDemoWindow::applyWindowLevelPreset);
  connect(resetWindowLevelButton_, &QPushButton::clicked, this,
          &OpenGLDemoWindow::resetWindowLevel);
  connect(openGLViewer_, &OpenGLSliceViewer::crosshairPositionValueChanged, this,
          &OpenGLDemoWindow::updateCrosshairPosition);
  connect(openGLViewer_, &OpenGLSliceViewer::crosshairPositionChanged, this,
          &OpenGLDemoWindow::updateCurrentCrosshairPosition);
  connect(resetCrosshairButton_, &QPushButton::clicked, openGLViewer_,
          &OpenGLSliceViewer::resetCrosshair);
  connect(resetViewButton_, &QPushButton::clicked, openGLViewer_, &OpenGLSliceViewer::resetView);
  connect(showMaskOverlayCheckBox_, &QCheckBox::toggled, this,
          [this](bool) { updateVolumeSlice(); });
}

void OpenGLDemoWindow::loadInitialDemoImage()
{
  const QImage openGLDemoImage = createDemoImage();
  currentDisplaySize_ = openGLDemoImage.size();
  openGLViewer_->setOrientation(SliceOrientation::Axial);
  openGLViewer_->setImage(openGLDemoImage);
  updateVolumeMetadataLabel();
}

void OpenGLDemoWindow::openImage()
{
  const QString fileName =
      QFileDialog::getOpenFileName(this, "Open Image", QString(), "Images (*.png *.jpg *.jpeg *.bmp)");

  if (fileName.isEmpty())
  {
    return;
  }

  const QImage image(fileName);
  if (image.isNull())
  {
    QMessageBox::warning(this, "Open Failed", "Could not load the selected image.");
    return;
  }

  hasCurrentVolumeSlice_ = false;
  currentVolume_.reset();
  maskVolume_.reset();
  currentVolumeRange_.reset();
  showMaskOverlayCheckBox_->setChecked(false);
  currentDisplaySize_ = image.size();
  openGLViewer_->setOrientation(SliceOrientation::Axial);
  openGLViewer_->setImage(image);
  updateSliceLabel();
  updateVolumeMetadataLabel();
}

void OpenGLDemoWindow::openMaskOverlay()
{
  if (!currentVolume_.has_value() || !currentVolume_->isValid())
  {
    QMessageBox::warning(this, "Mask Overlay Error", "Load a base volume before opening a mask overlay.");
    return;
  }

  const QString fileName =
      QFileDialog::getOpenFileName(this, "Open Mask Overlay", QString(), "NIfTI Volumes (*.nii *.nii.gz)");
  if (fileName.isEmpty())
  {
    return;
  }

  VolumeLoadResult result = loadMedicalVolume(fileName);
  if (!result.success)
  {
    QMessageBox::warning(this, "Mask Overlay Error", result.errorMessage);
    return;
  }

  if (!result.volume.isValid())
  {
    QMessageBox::warning(this, "Mask Overlay Error", "Loaded mask volume data is invalid.");
    return;
  }

  if (!maskMatchesCurrentVolume(result.volume))
  {
    QMessageBox::warning(this,
                         "Mask Overlay Error",
                         "Mask volume dimensions must match the currently loaded volume.");
    return;
  }

  maskVolume_ = std::move(result.volume);
  showMaskOverlayCheckBox_->setChecked(true);
  updateVolumeSlice();
}

void OpenGLDemoWindow::loadSyntheticSlice()
{
  setVolume(createSyntheticVolume());
}

void OpenGLDemoWindow::loadRawSlice()
{
  const QString metadataPath = QFileDialog::getOpenFileName(
      this, "Open RAW Volume Metadata", QString(), "JSON Metadata (*.json);;All Files (*)");
  if (metadataPath.isEmpty())
  {
    return;
  }

  const QString rawPath = QFileDialog::getOpenFileName(
      this, "Open RAW Volume Data", QString(), "RAW Volume Data (*.raw);;All Files (*)");
  if (rawPath.isEmpty())
  {
    return;
  }

  try
  {
    setVolume(RawVolumeLoader::load(metadataPath, rawPath));
  }
  catch (const std::exception& error)
  {
    QMessageBox::warning(this, "RAW Volume Load Error", error.what());
  }
}

void OpenGLDemoWindow::setSliceIndex(int sliceIndex)
{
  if (!hasCurrentVolumeSlice_ || sliceIndex < 0 ||
      static_cast<std::size_t>(sliceIndex) >= currentSliceCount())
  {
    return;
  }

  currentSliceIndex_ = static_cast<std::size_t>(sliceIndex);
  updateSliceLabel();
  updateVolumeSlice();
}

void OpenGLDemoWindow::setSliceOrientation(int orientationIndex)
{
  if (!hasCurrentVolumeSlice_)
  {
    return;
  }

  switch (orientationIndex)
  {
  case 1:
    currentOrientation_ = SliceOrientation::Coronal;
    break;
  case 2:
    currentOrientation_ = SliceOrientation::Sagittal;
    break;
  default:
    currentOrientation_ = SliceOrientation::Axial;
    break;
  }

  openGLViewer_->setOrientation(currentOrientation_);

  const std::size_t sliceCount = currentSliceCount();
  if (sliceCount == 0)
  {
    updateSliceLabel();
    return;
  }

  currentSliceIndex_ = sliceCount / 2;
  const auto slice = SliceExtractor::extract(currentVolume_.value(), currentOrientation_, currentSliceIndex_);
  currentDisplaySize_ = QSize(static_cast<int>(slice.width()), static_cast<int>(slice.height()));
  configureSliceSlider();
  updateSliceLabel();
  updateVolumeSlice();
}

void OpenGLDemoWindow::previousSlice()
{
  if (!hasCurrentVolumeSlice_ || currentSliceCount() == 0 || currentSliceIndex_ == 0)
  {
    return;
  }

  --currentSliceIndex_;
  const auto slice = SliceExtractor::extract(currentVolume_.value(), currentOrientation_, currentSliceIndex_);
  currentDisplaySize_ = QSize(static_cast<int>(slice.width()), static_cast<int>(slice.height()));
  const QSignalBlocker blocker(sliceSlider_);
  sliceSlider_->setValue(static_cast<int>(currentSliceIndex_));
  updateSliceLabel();
  updateVolumeSlice();
}

void OpenGLDemoWindow::nextSlice()
{
  if (!hasCurrentVolumeSlice_)
  {
    return;
  }

  const std::size_t sliceCount = currentSliceCount();
  if (sliceCount == 0)
  {
    return;
  }

  const std::size_t maxSliceIndex = sliceCount - 1;
  if (currentSliceIndex_ >= maxSliceIndex)
  {
    return;
  }

  ++currentSliceIndex_;
  const auto slice = SliceExtractor::extract(currentVolume_.value(), currentOrientation_, currentSliceIndex_);
  currentDisplaySize_ = QSize(static_cast<int>(slice.width()), static_cast<int>(slice.height()));
  const QSignalBlocker blocker(sliceSlider_);
  sliceSlider_->setValue(static_cast<int>(currentSliceIndex_));
  updateSliceLabel();
  updateVolumeSlice();
}

void OpenGLDemoWindow::updateWindowLevel()
{
  updateVolumeSlice();
  updateVolumeMetadataLabel();
}

void OpenGLDemoWindow::resetWindowLevel()
{
  const QSignalBlocker windowBlocker(windowSpinBox_);
  const QSignalBlocker levelBlocker(levelSpinBox_);
  windowSpinBox_->setValue(255);
  levelSpinBox_->setValue(127);
  updateVolumeSlice();
  updateVolumeMetadataLabel();
}

void OpenGLDemoWindow::applyWindowLevelPreset(int presetIndex)
{
  int window = windowSpinBox_->value();
  int level = levelSpinBox_->value();

  switch (presetIndex)
  {
  case 1:
    window = 400;
    level = 40;
    break;
  case 2:
    window = 1500;
    level = -600;
    break;
  case 3:
    window = 2000;
    level = 300;
    break;
  case 4:
    window = 255;
    level = 127;
    break;
  default:
    return;
  }

  const QSignalBlocker windowBlocker(windowSpinBox_);
  const QSignalBlocker levelBlocker(levelSpinBox_);
  windowSpinBox_->setValue(window);
  levelSpinBox_->setValue(level);
  updateVolumeSlice();
  updateVolumeMetadataLabel();
}

void OpenGLDemoWindow::updateCrosshairPosition(QPointF position, int value)
{
  currentCrosshairPosition_ = position;
  updateCrosshairLabel(value);
}

void OpenGLDemoWindow::updateCurrentCrosshairPosition(QPointF position)
{
  currentCrosshairPosition_ = position;
}

void OpenGLDemoWindow::updateCrosshairLabel(int value)
{
  const std::size_t pixelWidth = static_cast<std::size_t>(std::max(0, currentDisplaySize_.width()));
  const std::size_t pixelHeight = static_cast<std::size_t>(std::max(0, currentDisplaySize_.height()));
  if (pixelWidth == 0 || pixelHeight == 0)
  {
    crosshairPositionLabel_->setText(QString("x=%1 y=%2 | val=%3")
                                         .arg(currentCrosshairPosition_.x(), 0, 'f', 3)
                                         .arg(currentCrosshairPosition_.y(), 0, 'f', 3)
                                         .arg(value));
    return;
  }

  const std::size_t pixelX = normalizeToPixelIndex(currentCrosshairPosition_.x(), pixelWidth);
  const std::size_t pixelY = normalizeToPixelIndex(-currentCrosshairPosition_.y(), pixelHeight);

  const QString labelText = QString("x=%1 y=%2 | px=(%3,%4) | val=%5")
                                .arg(currentCrosshairPosition_.x(), 0, 'f', 3)
                                .arg(currentCrosshairPosition_.y(), 0, 'f', 3)
                                .arg(pixelX)
                                .arg(pixelY)
                                .arg(value);

  if (!hasCurrentVolumeSlice_ || !currentVolume_.has_value())
  {
    crosshairPositionLabel_->setText(labelText);
    return;
  }

  const auto [sliceWidth, sliceHeight] = sliceDimensions();
  if (sliceWidth == 0 || sliceHeight == 0)
  {
    crosshairPositionLabel_->setText(labelText);
    return;
  }

  std::size_t voxelX = pixelX;
  std::size_t voxelY = pixelY;
  std::size_t voxelZ = currentSliceIndex_;

  switch (currentOrientation_)
  {
  case SliceOrientation::Coronal:
    voxelY = currentSliceIndex_;
    voxelZ = pixelY;
    break;
  case SliceOrientation::Sagittal:
    voxelX = currentSliceIndex_;
    voxelY = pixelX;
    voxelZ = pixelY;
    break;
  case SliceOrientation::Axial:
    voxelX = pixelX;
    voxelY = pixelY;
    voxelZ = currentSliceIndex_;
    break;
  }

  const VolumeData& volume = currentVolume_.value();
  voxelX = std::min(voxelX, volume.width() == 0 ? std::size_t{0} : volume.width() - 1);
  voxelY = std::min(voxelY, volume.height() == 0 ? std::size_t{0} : volume.height() - 1);
  voxelZ = std::min(voxelZ, volume.depth() == 0 ? std::size_t{0} : volume.depth() - 1);

  crosshairPositionLabel_->setText(QString("%1 | voxel=(%2,%3,%4)")
                                       .arg(labelText)
                                       .arg(voxelX)
                                       .arg(voxelY)
                                       .arg(voxelZ));
}

void OpenGLDemoWindow::updateVolumeSlice()
{
  if (!hasCurrentVolumeSlice_ || !currentVolume_.has_value() || !currentVolume_->isValid())
  {
    return;
  }

  try
  {
    const auto slice =
        SliceExtractor::extract(currentVolume_.value(), currentOrientation_, currentSliceIndex_);
    QImage image = SliceImageConverter::toGrayscaleImage(
        slice, static_cast<float>(windowSpinBox_->value()), static_cast<float>(levelSpinBox_->value()));

    if (maskVolume_.has_value() && showMaskOverlayCheckBox_->isChecked())
    {
      const auto maskSlice =
          SliceExtractor::extract(maskVolume_.value(), currentOrientation_, currentSliceIndex_);
      image = applyMaskOverlay(image, maskSlice);
    }

    openGLViewer_->setSliceImage(image);
  }
  catch (const std::exception& error)
  {
    QMessageBox::warning(this, "Slice Update Error", error.what());
  }
}

void OpenGLDemoWindow::configureSliceSlider()
{
  const std::size_t sliceCount = currentSliceCount();
  if (sliceCount == 0)
  {
    return;
  }

  const QSignalBlocker blocker(sliceSlider_);
  sliceSlider_->setRange(0, static_cast<int>(sliceCount - 1));
  sliceSlider_->setValue(static_cast<int>(currentSliceIndex_));
}

void OpenGLDemoWindow::updateSliceLabel()
{
  const std::size_t sliceCount = currentSliceCount();
  if (sliceCount == 0)
  {
    sliceIndexLabel_->setText("Slice: - / -");
    updateVolumeMetadataLabel();
    return;
  }

  sliceIndexLabel_->setText(QString("Slice: %1 / %2").arg(currentSliceIndex_ + 1).arg(sliceCount));
  updateVolumeMetadataLabel();
}

void OpenGLDemoWindow::updateVolumeMetadataLabel()
{
  volumeMetadataLabel_->setText(formatVolumeMetadata());
}

void OpenGLDemoWindow::resetToAxialMiddleSlice()
{
  currentOrientation_ = SliceOrientation::Axial;
  currentSliceIndex_ = currentVolume_.value().depth() / 2;
  hasCurrentVolumeSlice_ = true;
  const auto slice = SliceExtractor::extract(currentVolume_.value(), currentOrientation_, currentSliceIndex_);
  currentDisplaySize_ = QSize(static_cast<int>(slice.width()), static_cast<int>(slice.height()));
  const QSignalBlocker blocker(orientationComboBox_);
  orientationComboBox_->setCurrentIndex(0);
  openGLViewer_->setOrientation(currentOrientation_);
  configureSliceSlider();
  updateSliceLabel();
}

bool OpenGLDemoWindow::looksLikeCtVolume(const VolumeData& volume) const
{
  if (!volume.isValid())
  {
    return false;
  }

  const auto& voxels = volume.voxels();
  if (voxels.empty())
  {
    return false;
  }

  const auto [minIt, maxIt] = std::minmax_element(voxels.begin(), voxels.end());
  return *minIt < -500.0F && *maxIt > 500.0F;
}

void OpenGLDemoWindow::applyCtWindowLevelPresetIfNeeded(const VolumeData& volume)
{
  if (!looksLikeCtVolume(volume))
  {
    return;
  }

  const QSignalBlocker windowBlocker(windowSpinBox_);
  const QSignalBlocker levelBlocker(levelSpinBox_);

  windowSpinBox_->setValue(1500);
  levelSpinBox_->setValue(-600);
}

bool OpenGLDemoWindow::maskMatchesCurrentVolume(const VolumeData& mask) const
{
  return currentVolume_.has_value() && currentVolume_->isValid() && mask.isValid() &&
         currentVolume_->width() == mask.width() && currentVolume_->height() == mask.height() &&
         currentVolume_->depth() == mask.depth();
}

QImage OpenGLDemoWindow::applyMaskOverlay(const QImage& baseImage, const SliceData& maskSlice) const
{
  if (baseImage.isNull() || static_cast<std::size_t>(baseImage.width()) != maskSlice.width() ||
      static_cast<std::size_t>(baseImage.height()) != maskSlice.height())
  {
    return baseImage;
  }

  QImage overlayImage = baseImage.convertToFormat(QImage::Format_RGBA8888);
  const auto& maskPixels = maskSlice.pixels();
  const int imageWidth = overlayImage.width();
  const int imageHeight = overlayImage.height();

  for (int y = 0; y < imageHeight; ++y)
  {
    uchar* scanLine = overlayImage.scanLine(y);
    for (int x = 0; x < imageWidth; ++x)
    {
      const std::size_t pixelIndex =
          static_cast<std::size_t>(y) * static_cast<std::size_t>(imageWidth) + static_cast<std::size_t>(x);
      if (pixelIndex >= maskPixels.size() || maskPixels[pixelIndex] <= 0.5F)
      {
        continue;
      }

      const int channelOffset = x * 4;
      scanLine[channelOffset + 0] =
          static_cast<uchar>(((static_cast<int>(scanLine[channelOffset + 0]) * 70) + (255 * 30)) / 100);
      scanLine[channelOffset + 1] =
          static_cast<uchar>((static_cast<int>(scanLine[channelOffset + 1]) * 70) / 100);
      scanLine[channelOffset + 2] =
          static_cast<uchar>((static_cast<int>(scanLine[channelOffset + 2]) * 70) / 100);
    }
  }

  return overlayImage;
}

QString OpenGLDemoWindow::formatVolumeMetadata() const
{
  if (!currentVolume_.has_value() || !currentVolume_->isValid() || !currentVolumeRange_.has_value())
  {
    return QStringLiteral("No volume loaded\nWindow / Level: %1 / %2")
        .arg(windowSpinBox_->value())
        .arg(levelSpinBox_->value());
  }

  const auto [minValue, maxValue] = currentVolumeRange_.value();
  return QStringLiteral(
             "Volume size: %1 x %2 x %3\n"
             "Voxel spacing: %4 x %5 x %6\n"
             "Voxel range: %7 .. %8\n"
             "Current orientation: %9\n"
             "Current slice: %10 / %11\n"
             "Window / Level: %12 / %13")
      .arg(currentVolume_->width())
      .arg(currentVolume_->height())
      .arg(currentVolume_->depth())
      .arg(currentVolume_->spacingX(), 0, 'f', 3)
      .arg(currentVolume_->spacingY(), 0, 'f', 3)
      .arg(currentVolume_->spacingZ(), 0, 'f', 3)
      .arg(minValue, 0, 'f', 1)
      .arg(maxValue, 0, 'f', 1)
      .arg(orientationName(currentOrientation_))
      .arg(currentSliceCount() == 0 ? 0 : static_cast<int>(currentSliceIndex_ + 1))
      .arg(currentSliceCount())
      .arg(windowSpinBox_->value())
      .arg(levelSpinBox_->value());
}

std::size_t OpenGLDemoWindow::currentSliceCount() const
{
  if (!hasCurrentVolumeSlice_ || !currentVolume_.has_value() || !currentVolume_->isValid())
  {
    return 0;
  }

  switch (currentOrientation_)
  {
  case SliceOrientation::Coronal:
    return currentVolume_->height();
  case SliceOrientation::Sagittal:
    return currentVolume_->width();
  case SliceOrientation::Axial:
    return currentVolume_->depth();
  }

  return currentVolume_->depth();
}

std::pair<std::size_t, std::size_t> OpenGLDemoWindow::sliceDimensions() const
{
  if (!hasCurrentVolumeSlice_ || !currentVolume_.has_value() || !currentVolume_->isValid())
  {
    return {0, 0};
  }

  switch (currentOrientation_)
  {
  case SliceOrientation::Coronal:
    return {currentVolume_->width(), currentVolume_->depth()};
  case SliceOrientation::Sagittal:
    return {currentVolume_->height(), currentVolume_->depth()};
  case SliceOrientation::Axial:
    return {currentVolume_->width(), currentVolume_->height()};
  }

  return {0, 0};
}

QImage OpenGLDemoWindow::createDemoImage()
{
  constexpr int kImageSize = 256;
  QImage image(kImageSize, kImageSize, QImage::Format_RGBA8888);

  for (int y = 0; y < image.height(); ++y)
  {
    auto* row = image.scanLine(y);
    for (int x = 0; x < image.width(); ++x)
    {
      const int offset = x * 4;
      const int gradient = (x + y) / 2;
      const bool inSquare = x >= 88 && x < 168 && y >= 88 && y < 168;
      const bool inCross = (x >= 124 && x < 132) || (y >= 124 && y < 132);
      const int value = inSquare || inCross ? 240 : 24 + gradient / 2;

      row[offset] = static_cast<uchar>(value);
      row[offset + 1] = static_cast<uchar>(value);
      row[offset + 2] = static_cast<uchar>(value);
      row[offset + 3] = 255;
    }
  }

  return image;
}

VolumeData OpenGLDemoWindow::createSyntheticVolume()
{
  constexpr std::size_t width = 128;
  constexpr std::size_t height = 128;
  constexpr std::size_t depth = 32;
  constexpr float spacing = 1.0F;

  std::vector<float> voxels;
  voxels.reserve(width * height * depth);

  const float centerX = static_cast<float>(width - 1) / 2.0F;
  const float centerY = static_cast<float>(height - 1) / 2.0F;
  const float centerZ = static_cast<float>(depth - 1) / 2.0F;
  constexpr float sphereRadius = 1.02F;
  constexpr float brightValue = 240.0F;
  constexpr float backgroundValue = 35.0F;

  for (std::size_t z = 0; z < depth; ++z)
  {
    const float dz = (static_cast<float>(z) - centerZ) / centerZ;

    for (std::size_t y = 0; y < height; ++y)
    {
      const float dy = (static_cast<float>(y) - centerY) / centerY;

      for (std::size_t x = 0; x < width; ++x)
      {
        const float dx = (static_cast<float>(x) - centerX) / centerX;
        const float distance = std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
        voxels.push_back(distance <= sphereRadius ? brightValue : backgroundValue);
      }
    }
  }

  return VolumeData(width, height, depth, spacing, spacing, spacing, std::move(voxels));
}

std::size_t OpenGLDemoWindow::normalizeToPixelIndex(double normalized, std::size_t size)
{
  if (size == 0)
  {
    return 0;
  }

  const double clamped = std::clamp(normalized, -1.0, 1.0);
  const double mapped = ((clamped + 1.0) * 0.5) * static_cast<double>(size - 1);
  return static_cast<std::size_t>(std::clamp(mapped, 0.0, static_cast<double>(size - 1)));
}

} // namespace qvp
