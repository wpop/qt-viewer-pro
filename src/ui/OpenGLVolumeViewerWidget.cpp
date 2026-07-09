#include "qtviewerpro/ui/OpenGLVolumeViewerWidget.h"

#include "qtviewerpro/core/SliceExtractor.h"
#include "qtviewerpro/io/MedicalVolumeLoaderRegistry.h"
#include "qtviewerpro/io/RawVolumeLoader.h"
#include "qtviewerpro/processing/SliceImageConverter.h"
#include "qtviewerpro/ui/MessageBoxUtils.h"
#include "qtviewerpro/render/OpenGLSliceViewer.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace qvp
{

namespace
{
constexpr int kDefaultWindow = 255;
constexpr int kDefaultLevel = 127;
constexpr int kSoftTissueWindow = 400;
constexpr int kSoftTissueLevel = 40;
constexpr int kLungWindow = 1500;
constexpr int kLungLevel = -600;
constexpr int kBoneWindow = 2000;
constexpr int kBoneLevel = 300;
constexpr int kPresetIndexNone = 0;
constexpr int kPresetIndexSoftTissue = 1;
constexpr int kPresetIndexLung = 2;
constexpr int kPresetIndexBone = 3;
constexpr int kPresetIndexReset = 4;

void setWindowLevelControls(QSpinBox* windowSpinBox,
                            QSpinBox* levelSpinBox,
                            int window,
                            int level)
{
  const QSignalBlocker windowBlocker(windowSpinBox);
  const QSignalBlocker levelBlocker(levelSpinBox);
  windowSpinBox->setValue(window);
  levelSpinBox->setValue(level);
}

void setWindowLevelPresetIndex(QComboBox* presetComboBox, int presetIndex)
{
  const QSignalBlocker presetBlocker(presetComboBox);
  presetComboBox->setCurrentIndex(presetIndex);
}

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

QString validateRawMetadataFile(const QString& metadataPath)
{
  QFile metadataFile(metadataPath);
  if (!metadataFile.open(QIODevice::ReadOnly))
  {
    return QStringLiteral("Unable to open metadata file");
  }

  QJsonParseError parseError;
  const QJsonDocument metadataDocument =
      QJsonDocument::fromJson(metadataFile.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError)
  {
    return QStringLiteral("Metadata JSON could not be parsed: %1").arg(parseError.errorString());
  }

  if (!metadataDocument.isObject())
  {
    return QStringLiteral("Metadata JSON must be a valid object");
  }

  const QJsonObject metadata = metadataDocument.object();

  auto requiredValueError = [](const QJsonObject& object, const QString& fieldName) -> QString {
    if (!object.contains(fieldName))
    {
      return QStringLiteral("Missing metadata field: %1").arg(fieldName);
    }

    return QString();
  };

  auto positiveDimensionError = [&](const QString& fieldName) -> QString {
    const QString requiredError = requiredValueError(metadata, fieldName);
    if (!requiredError.isEmpty())
    {
      return requiredError;
    }

    const QJsonValue value = metadata.value(fieldName);
    if (!value.isDouble())
    {
      return QStringLiteral("Metadata field must be numeric: %1").arg(fieldName);
    }

    const double number = value.toDouble();
    if (!std::isfinite(number) || number <= 0.0 || std::floor(number) != number)
    {
      return QStringLiteral("Metadata dimension must be a positive integer: %1").arg(fieldName);
    }

    if (number > static_cast<double>(std::numeric_limits<std::size_t>::max()))
    {
      return QStringLiteral("Metadata dimension is too large: %1").arg(fieldName);
    }

    return QString();
  };

  auto positiveSpacingError = [&](const QString& fieldName) -> QString {
    const QString requiredError = requiredValueError(metadata, fieldName);
    if (!requiredError.isEmpty())
    {
      return requiredError;
    }

    const QJsonValue value = metadata.value(fieldName);
    if (!value.isDouble())
    {
      return QStringLiteral("Metadata field must be numeric: %1").arg(fieldName);
    }

    const double number = value.toDouble();
    if (!std::isfinite(number) || number <= 0.0 ||
        number > static_cast<double>(std::numeric_limits<float>::max()))
    {
      return QStringLiteral("Metadata spacing must be positive: %1").arg(fieldName);
    }

    return QString();
  };

  for (const QString& fieldName :
       {QStringLiteral("width"), QStringLiteral("height"), QStringLiteral("depth")})
  {
    const QString error = positiveDimensionError(fieldName);
    if (!error.isEmpty())
    {
      return error;
    }
  }

  for (const QString& fieldName :
       {QStringLiteral("spacingX"), QStringLiteral("spacingY"), QStringLiteral("spacingZ")})
  {
    const QString error = positiveSpacingError(fieldName);
    if (!error.isEmpty())
    {
      return error;
    }
  }

  return QString();
}

void showRawVolumeLoadError(QWidget* parent, const QString& details)
{
  qvp::showStyledWarning(
      parent,
      QStringLiteral("RAW Volume Load Error"),
      QStringLiteral("Select the RAW JSON metadata file only."),
      QStringLiteral("MetaImage/LUNA16 .raw files should be opened via the .mhd file using "
                     "File -> Open Medical Volume...\n\n"
                     "The RAW JSON workflow expects a sibling volume.raw file unless the metadata "
                     "explicitly provides rawFile.\n\n"
                     "Details: %1")
          .arg(details));
}

} // namespace

OpenGLVolumeViewerWidget::OpenGLVolumeViewerWidget(QWidget* parent) : QWidget(parent)
{
  createUi();
  connectSignals();
  loadInitialDemoImage();
}

void OpenGLVolumeViewerWidget::setVolume(VolumeData volume)
{
  setVolume(std::make_shared<const VolumeData>(std::move(volume)));
}

void OpenGLVolumeViewerWidget::setVolume(std::shared_ptr<const VolumeData> volume)
{
  if (!volume || !volume->isValid())
  {
    showStyledWarning(this, "Volume Load Error", "Loaded volume data is invalid.");
    return;
  }

  currentVolume_ = std::move(volume);
  maskVolume_.reset();
  {
    const QSignalBlocker blocker(showMaskOverlayCheckBox_);
    showMaskOverlayCheckBox_->setChecked(false);
  }
  updateMaskOpacityControls();
  const float cachedMinimumIntensity = currentVolume_->intensityMinimum();
  const float cachedMaximumIntensity = currentVolume_->intensityMaximum();
  currentVolumeRange_ = std::make_pair(cachedMinimumIntensity, cachedMaximumIntensity);

  setWindowLevelControls(windowSpinBox_, levelSpinBox_, kDefaultWindow, kDefaultLevel);
  setWindowLevelPresetIndex(windowLevelPresetComboBox_, kPresetIndexReset);

  applyCtWindowLevelPresetIfNeeded(*currentVolume_);
  resetToAxialMiddleSlice();
  updateVolumeSlice();
  openGLViewer_->resetView();
}

void OpenGLVolumeViewerWidget::createUi()
{
  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(12);

  openImageButton_ = new QPushButton("Open Image...", this);
  openMaskOverlayButton_ = new QPushButton("Open Mask Overlay...", this);
  loadSyntheticSliceButton_ = new QPushButton("Load Synthetic Test Volume", this);
  loadRawSliceButton_ = new QPushButton("Load Custom RAW Test Volume...", this);
  resetViewButton_ = new QPushButton("Reset View", this);
  resetCrosshairButton_ = new QPushButton("Reset Crosshair", this);
  showCrosshairCheckBox_ = new QCheckBox("Show Crosshair", this);
  showImageBorderCheckBox_ = new QCheckBox("Show Image Border", this);
  showMaskOverlayCheckBox_ = new QCheckBox("Show Mask Overlay", this);
  showCrosshairCheckBox_->setChecked(true);
  showImageBorderCheckBox_->setChecked(true);
  showMaskOverlayCheckBox_->setChecked(false);

  maskOpacityValueLabel_ = new QLabel(QStringLiteral("%1%").arg(maskOpacityPercent_), this);
  maskOpacitySlider_ = new QSlider(Qt::Horizontal, this);
  maskOpacitySlider_->setRange(0, 100);
  maskOpacitySlider_->setValue(maskOpacityPercent_);
  maskOpacitySlider_->setEnabled(false);

  orientationComboBox_ = new QComboBox(this);
  orientationComboBox_->addItems({"Axial", "Coronal", "Sagittal"});

  windowSpinBox_ = new QSpinBox(this);
  windowSpinBox_->setRange(1, 4096);
  windowSpinBox_->setValue(kDefaultWindow);
  levelSpinBox_ = new QSpinBox(this);
  levelSpinBox_->setRange(-2048, 4096);
  levelSpinBox_->setValue(kDefaultLevel);
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

  openGLViewer_ = new OpenGLSliceViewer(this);
  openGLViewer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  auto* viewerLayout = new QVBoxLayout();
  viewerLayout->setContentsMargins(0, 0, 0, 0);
  viewerLayout->setSpacing(8);
  viewerLayout->addWidget(openGLViewer_, 1);

  auto* viewerStatusLayout = new QHBoxLayout();
  viewerStatusLayout->setSpacing(8);
  viewerStatusLayout->addStretch();
  viewerStatusLayout->addWidget(crosshairPositionLabel_);
  viewerLayout->addLayout(viewerStatusLayout);

  auto* controlsLayout = new QVBoxLayout();
  controlsLayout->setContentsMargins(0, 0, 0, 0);
  controlsLayout->setSpacing(10);

  auto* filesGroup = new QGroupBox("Files", this);
  auto* filesLayout = new QVBoxLayout(filesGroup);
  filesLayout->setSpacing(6);
  filesLayout->addWidget(openImageButton_);
  filesLayout->addWidget(openMaskOverlayButton_);
  filesLayout->addWidget(loadSyntheticSliceButton_);
  filesLayout->addWidget(loadRawSliceButton_);
  filesLayout->addWidget(resetViewButton_);
  filesLayout->addWidget(resetCrosshairButton_);

  auto* sliceGroup = new QGroupBox("Slice / Orientation", this);
  auto* sliceGroupLayout = new QVBoxLayout(sliceGroup);
  sliceGroupLayout->setSpacing(6);
  auto* orientationLayout = new QHBoxLayout();
  orientationLayout->setSpacing(6);
  orientationLayout->addWidget(new QLabel("Orientation", this));
  orientationLayout->addWidget(orientationComboBox_, 1);
  sliceGroupLayout->addLayout(orientationLayout);
  auto* sliceNavigationLayout = new QHBoxLayout();
  sliceNavigationLayout->setSpacing(6);
  sliceNavigationLayout->addWidget(previousSliceButton_);
  sliceNavigationLayout->addWidget(sliceSlider_, 1);
  sliceNavigationLayout->addWidget(nextSliceButton_);
  sliceGroupLayout->addLayout(sliceNavigationLayout);
  sliceGroupLayout->addWidget(sliceIndexLabel_);

  auto* windowLevelGroup = new QGroupBox("Window / Level", this);
  auto* windowLevelGroupLayout = new QVBoxLayout(windowLevelGroup);
  windowLevelGroupLayout->setSpacing(6);
  auto* windowLayout = new QHBoxLayout();
  windowLayout->setSpacing(6);
  windowLayout->addWidget(new QLabel("Window", this));
  windowLayout->addWidget(windowSpinBox_, 1);
  windowLevelGroupLayout->addLayout(windowLayout);
  auto* levelLayout = new QHBoxLayout();
  levelLayout->setSpacing(6);
  levelLayout->addWidget(new QLabel("Level", this));
  levelLayout->addWidget(levelSpinBox_, 1);
  windowLevelGroupLayout->addLayout(levelLayout);
  windowLevelGroupLayout->addWidget(windowLevelPresetComboBox_);
  windowLevelGroupLayout->addWidget(resetWindowLevelButton_);

  auto* overlaysGroup = new QGroupBox("Overlays", this);
  auto* overlaysLayout = new QVBoxLayout(overlaysGroup);
  overlaysLayout->setSpacing(6);
  overlaysLayout->addWidget(showCrosshairCheckBox_);
  overlaysLayout->addWidget(showImageBorderCheckBox_);
  overlaysLayout->addWidget(showMaskOverlayCheckBox_);
  auto* maskOpacityLayout = new QHBoxLayout();
  maskOpacityLayout->setSpacing(6);
  maskOpacityLayout->addWidget(new QLabel("Mask Opacity", this));
  maskOpacityLayout->addWidget(maskOpacitySlider_, 1);
  maskOpacityLayout->addWidget(maskOpacityValueLabel_);
  overlaysLayout->addLayout(maskOpacityLayout);

  auto* metadataGroup = new QGroupBox("Metadata", this);
  auto* metadataLayout = new QVBoxLayout(metadataGroup);
  metadataLayout->setSpacing(6);
  metadataLayout->addWidget(volumeMetadataLabel_);

  auto* controlsPanel = new QWidget(this);
  controlsPanel->setMinimumWidth(280);
  controlsPanel->setMaximumWidth(360);
  controlsPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  controlsPanel->setLayout(controlsLayout);

  controlsLayout->addWidget(filesGroup);
  controlsLayout->addWidget(sliceGroup);
  controlsLayout->addWidget(windowLevelGroup);
  controlsLayout->addWidget(overlaysGroup);
  controlsLayout->addWidget(metadataGroup, 1);

  layout->addLayout(viewerLayout, 1);
  layout->addWidget(controlsPanel);
}

void OpenGLVolumeViewerWidget::connectSignals()
{
  connect(showCrosshairCheckBox_, &QCheckBox::toggled, openGLViewer_,
          &OpenGLSliceViewer::setCrosshairVisible);
  connect(showImageBorderCheckBox_, &QCheckBox::toggled, openGLViewer_,
          &OpenGLSliceViewer::setImageBorderVisible);
  connect(openImageButton_, &QPushButton::clicked, this, &OpenGLVolumeViewerWidget::openImage);
  connect(openMaskOverlayButton_, &QPushButton::clicked, this, &OpenGLVolumeViewerWidget::openMaskOverlay);
  connect(loadSyntheticSliceButton_, &QPushButton::clicked, this,
          &OpenGLVolumeViewerWidget::loadSyntheticSlice);
  connect(loadRawSliceButton_, &QPushButton::clicked, this, &OpenGLVolumeViewerWidget::loadRawSlice);
  connect(sliceSlider_, &QSlider::valueChanged, this, &OpenGLVolumeViewerWidget::setSliceIndex);
  connect(orientationComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this,
          &OpenGLVolumeViewerWidget::setSliceOrientation);
  connect(previousSliceButton_, &QPushButton::clicked, this, &OpenGLVolumeViewerWidget::previousSlice);
  connect(nextSliceButton_, &QPushButton::clicked, this, &OpenGLVolumeViewerWidget::nextSlice);
  connect(windowSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this,
          &OpenGLVolumeViewerWidget::updateWindowLevel);
  connect(levelSpinBox_, qOverload<int>(&QSpinBox::valueChanged), this,
          &OpenGLVolumeViewerWidget::updateWindowLevel);
  connect(windowLevelPresetComboBox_, qOverload<int>(&QComboBox::activated), this,
          &OpenGLVolumeViewerWidget::applyWindowLevelPreset);
  connect(resetWindowLevelButton_, &QPushButton::clicked, this,
          &OpenGLVolumeViewerWidget::resetWindowLevel);
  connect(maskOpacitySlider_, &QSlider::valueChanged, this,
          [this](int value) { updateMaskOpacity(value); });
  connect(openGLViewer_, &OpenGLSliceViewer::crosshairPositionValueChanged, this,
          &OpenGLVolumeViewerWidget::updateCrosshairPosition);
  connect(openGLViewer_, &OpenGLSliceViewer::crosshairPositionChanged, this,
          &OpenGLVolumeViewerWidget::updateCurrentCrosshairPosition);
  connect(resetCrosshairButton_, &QPushButton::clicked, openGLViewer_,
          &OpenGLSliceViewer::resetCrosshair);
  connect(resetViewButton_, &QPushButton::clicked, openGLViewer_, &OpenGLSliceViewer::resetView);
  connect(showMaskOverlayCheckBox_, &QCheckBox::toggled, this,
          [this](bool) { updateVolumeSlice(); });
}

void OpenGLVolumeViewerWidget::loadInitialDemoImage()
{
  const QImage openGLDemoImage = createDemoImage();
  currentDisplaySize_ = openGLDemoImage.size();
  openGLViewer_->setOrientation(SliceOrientation::Axial);
  openGLViewer_->setImage(openGLDemoImage);
  updateVolumeMetadataLabel();
}

void OpenGLVolumeViewerWidget::openImage()
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
    showStyledWarning(this, "Open Failed", "Could not load the selected image.");
    return;
  }

  hasCurrentVolumeSlice_ = false;
  currentVolume_.reset();
  maskVolume_.reset();
  currentVolumeRange_.reset();
  showMaskOverlayCheckBox_->setChecked(false);
  updateMaskOpacityControls();
  currentDisplaySize_ = image.size();
  openGLViewer_->setOrientation(SliceOrientation::Axial);
  openGLViewer_->setImage(image);
  updateSliceLabel();
  updateVolumeMetadataLabel();
}

void OpenGLVolumeViewerWidget::openMaskOverlay()
{
  if (!currentVolume_ || !currentVolume_->isValid())
  {
    showStyledWarning(this, "Mask Overlay Error", "Load a base volume before opening a mask overlay.");
    return;
  }

  const QString fileName =
      QFileDialog::getOpenFileName(
          this, "Open Mask Overlay", QString(),
          "Medical Volumes (*.nii *.nii.gz *.mhd *.mha *.dcm *.nrrd *.nhdr);;"
          "NIfTI Volumes (*.nii *.nii.gz);;"
          "MetaImage Volumes (*.mhd *.mha);;"
          "DICOM Files (*.dcm);;"
          "NRRD Volumes (*.nrrd *.nhdr);;"
          "All Files (*)");
  if (fileName.isEmpty())
  {
    return;
  }

  VolumeLoadResult result = loadMedicalVolume(fileName);
  if (!result.success)
  {
    showStyledWarning(this, "Mask Overlay Error", result.errorMessage);
    return;
  }

  if (!result.volume.isValid())
  {
    showStyledWarning(this, "Mask Overlay Error", "Loaded mask volume data is invalid.");
    return;
  }

  if (!maskMatchesCurrentVolume(result.volume))
  {
    showStyledWarning(this,
                      "Mask Overlay Error",
                      "Mask volume dimensions must match the currently loaded volume.");
    return;
  }

  maskVolume_ = std::move(result.volume);
  updateMaskOpacityControls();
  showMaskOverlayCheckBox_->setChecked(true);
  updateVolumeSlice();
}

void OpenGLVolumeViewerWidget::loadSyntheticSlice()
{
  setVolume(createSyntheticVolume());
}

void OpenGLVolumeViewerWidget::loadRawSlice()
{
  const QString metadataPath = QFileDialog::getOpenFileName(
      this, "Open RAW JSON Metadata", QString(), "JSON Metadata (*.json)");
  if (metadataPath.isEmpty())
  {
    return;
  }

  const QString metadataError = validateRawMetadataFile(metadataPath);
  if (!metadataError.isEmpty())
  {
    showRawVolumeLoadError(this, metadataError);
    return;
  }

  try
  {
    setVolume(RawVolumeLoader::load(metadataPath));
  }
  catch (const std::exception& error)
  {
    showRawVolumeLoadError(this, QString::fromUtf8(error.what()));
  }
}

void OpenGLVolumeViewerWidget::setSliceIndex(int sliceIndex)
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

void OpenGLVolumeViewerWidget::setSliceOrientation(int orientationIndex)
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
  const auto slice = SliceExtractor::extract(*currentVolume_, currentOrientation_, currentSliceIndex_);
  currentDisplaySize_ = QSize(static_cast<int>(slice.width()), static_cast<int>(slice.height()));
  configureSliceSlider();
  updateSliceLabel();
  updateVolumeSlice();
}

void OpenGLVolumeViewerWidget::previousSlice()
{
  if (!hasCurrentVolumeSlice_ || currentSliceCount() == 0 || currentSliceIndex_ == 0)
  {
    return;
  }

  --currentSliceIndex_;
  const auto slice = SliceExtractor::extract(*currentVolume_, currentOrientation_, currentSliceIndex_);
  currentDisplaySize_ = QSize(static_cast<int>(slice.width()), static_cast<int>(slice.height()));
  const QSignalBlocker blocker(sliceSlider_);
  sliceSlider_->setValue(static_cast<int>(currentSliceIndex_));
  updateSliceLabel();
  updateVolumeSlice();
}

void OpenGLVolumeViewerWidget::nextSlice()
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
  const auto slice = SliceExtractor::extract(*currentVolume_, currentOrientation_, currentSliceIndex_);
  currentDisplaySize_ = QSize(static_cast<int>(slice.width()), static_cast<int>(slice.height()));
  const QSignalBlocker blocker(sliceSlider_);
  sliceSlider_->setValue(static_cast<int>(currentSliceIndex_));
  updateSliceLabel();
  updateVolumeSlice();
}

void OpenGLVolumeViewerWidget::updateWindowLevel()
{
  setWindowLevelPresetIndex(windowLevelPresetComboBox_, kPresetIndexNone);
  updateVolumeSlice();
  updateVolumeMetadataLabel();
}

void OpenGLVolumeViewerWidget::resetWindowLevel()
{
  setWindowLevelControls(windowSpinBox_, levelSpinBox_, kDefaultWindow, kDefaultLevel);
  setWindowLevelPresetIndex(windowLevelPresetComboBox_, kPresetIndexReset);
  updateVolumeSlice();
  updateVolumeMetadataLabel();
}

void OpenGLVolumeViewerWidget::applyWindowLevelPreset(int presetIndex)
{
  int window = windowSpinBox_->value();
  int level = levelSpinBox_->value();

  switch (presetIndex)
  {
  case kPresetIndexSoftTissue:
    window = kSoftTissueWindow;
    level = kSoftTissueLevel;
    break;
  case kPresetIndexLung:
    window = kLungWindow;
    level = kLungLevel;
    break;
  case kPresetIndexBone:
    window = kBoneWindow;
    level = kBoneLevel;
    break;
  case kPresetIndexReset:
    window = kDefaultWindow;
    level = kDefaultLevel;
    break;
  default:
    return;
  }

  setWindowLevelControls(windowSpinBox_, levelSpinBox_, window, level);
  updateVolumeSlice();
  updateVolumeMetadataLabel();
}

void OpenGLVolumeViewerWidget::updateCrosshairPosition(QPointF position, int value)
{
  currentCrosshairPosition_ = position;
  updateCrosshairLabel(value);
}

void OpenGLVolumeViewerWidget::updateCurrentCrosshairPosition(QPointF position)
{
  currentCrosshairPosition_ = position;
}

void OpenGLVolumeViewerWidget::updateCrosshairLabel(int value)
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

  const QString imageReadout = QString("x=%1 y=%2 | px=(%3,%4) | val=%5")
                                .arg(currentCrosshairPosition_.x(), 0, 'f', 3)
                                .arg(currentCrosshairPosition_.y(), 0, 'f', 3)
                                .arg(pixelX)
                                .arg(pixelY)
                                .arg(value);

  if (!hasCurrentVolumeSlice_ || !currentVolume_ || !currentVolume_->isValid())
  {
    crosshairPositionLabel_->setText(imageReadout);
    return;
  }

  const auto [sliceWidth, sliceHeight] = sliceDimensions();
  if (sliceWidth == 0 || sliceHeight == 0)
  {
    crosshairPositionLabel_->setText(imageReadout);
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

  const VolumeData& volume = *currentVolume_;
  if (!voxelCoordinatesInBounds(volume, voxelX, voxelY, voxelZ))
  {
    crosshairPositionLabel_->setText(imageReadout);
    return;
  }

  QString volumeReadout = QString("x=%1 y=%2 | px=(%3,%4) | voxel=(%5,%6,%7) | value=%8")
                              .arg(currentCrosshairPosition_.x(), 0, 'f', 3)
                              .arg(currentCrosshairPosition_.y(), 0, 'f', 3)
                              .arg(pixelX)
                              .arg(pixelY)
                              .arg(voxelX)
                              .arg(voxelY)
                              .arg(voxelZ)
                              .arg(voxelValueAt(volume, voxelX, voxelY, voxelZ), 0, 'f', 1);

  if (maskVolume_.has_value() && maskVolume_->isValid() && maskMatchesCurrentVolume(*maskVolume_))
  {
    volumeReadout += QString(" | mask=%1")
                   .arg(voxelValueAt(maskVolume_.value(), voxelX, voxelY, voxelZ), 0, 'f', 1);
  }

  crosshairPositionLabel_->setText(volumeReadout);
}

bool OpenGLVolumeViewerWidget::voxelCoordinatesInBounds(const VolumeData& volume,
                                                        std::size_t x,
                                                        std::size_t y,
                                                        std::size_t z) const
{
  return volume.isValid() && x < volume.width() && y < volume.height() && z < volume.depth();
}

float OpenGLVolumeViewerWidget::voxelValueAt(const VolumeData& volume,
                                             std::size_t x,
                                             std::size_t y,
                                             std::size_t z) const
{
  const std::size_t index =
      (z * volume.height() * volume.width()) + (y * volume.width()) + x;
  return volume.voxels().at(index);
}

void OpenGLVolumeViewerWidget::updateMaskOpacity(int opacityPercent)
{
  const int clampedOpacity = std::clamp(opacityPercent, 0, 100);
  if (maskOpacityPercent_ == clampedOpacity)
  {
    if (maskOpacityValueLabel_)
    {
      maskOpacityValueLabel_->setText(QStringLiteral("%1%").arg(maskOpacityPercent_));
    }
    updateVolumeSlice();
    return;
  }

  maskOpacityPercent_ = clampedOpacity;
  if (maskOpacityValueLabel_)
  {
    maskOpacityValueLabel_->setText(QStringLiteral("%1%").arg(maskOpacityPercent_));
  }
  updateVolumeSlice();
}

void OpenGLVolumeViewerWidget::updateMaskOpacityControls()
{
  if (maskOpacityValueLabel_)
  {
    maskOpacityValueLabel_->setText(QStringLiteral("%1%").arg(maskOpacityPercent_));
  }

  if (maskOpacitySlider_)
  {
    const QSignalBlocker blocker(maskOpacitySlider_);
    maskOpacitySlider_->setValue(maskOpacityPercent_);
    maskOpacitySlider_->setEnabled(maskVolume_.has_value());
  }
}

void OpenGLVolumeViewerWidget::updateVolumeSlice()
{
  if (!hasCurrentVolumeSlice_ || !currentVolume_ || !currentVolume_->isValid())
  {
    return;
  }

  try
  {
    const auto slice = SliceExtractor::extract(*currentVolume_, currentOrientation_, currentSliceIndex_);
    QImage image = SliceImageConverter::toGrayscaleImage(
        slice, static_cast<float>(windowSpinBox_->value()), static_cast<float>(levelSpinBox_->value()));
    if (maskVolume_.has_value() && showMaskOverlayCheckBox_->isChecked())
    {
      const auto maskSlice =
          SliceExtractor::extract(maskVolume_.value(), currentOrientation_, currentSliceIndex_);
      image = applyMaskOverlay(image, maskSlice);
    }
    openGLViewer_->setSliceImage(image, static_cast<float>(slice.spacingX()),
                                 static_cast<float>(slice.spacingY()));
  }
  catch (const std::exception& error)
  {
    showStyledWarning(this, "Slice Update Error", QString::fromUtf8(error.what()));
  }
}

void OpenGLVolumeViewerWidget::configureSliceSlider()
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

void OpenGLVolumeViewerWidget::updateSliceLabel()
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

void OpenGLVolumeViewerWidget::updateVolumeMetadataLabel()
{
  volumeMetadataLabel_->setText(formatVolumeMetadata());
}

void OpenGLVolumeViewerWidget::resetToAxialMiddleSlice()
{
  currentOrientation_ = SliceOrientation::Axial;
  currentSliceIndex_ = currentVolume_->depth() / 2;
  hasCurrentVolumeSlice_ = true;
  const auto slice = SliceExtractor::extract(*currentVolume_, currentOrientation_, currentSliceIndex_);
  currentDisplaySize_ = QSize(static_cast<int>(slice.width()), static_cast<int>(slice.height()));
  const QSignalBlocker blocker(orientationComboBox_);
  orientationComboBox_->setCurrentIndex(0);
  openGLViewer_->setOrientation(currentOrientation_);
  configureSliceSlider();
  updateSliceLabel();
}

bool OpenGLVolumeViewerWidget::looksLikeCtVolume(const VolumeData& volume) const
{
  if (!volume.isValid())
  {
    return false;
  }

  if (!volume.hasIntensityRange())
  {
    return false;
  }

  return volume.intensityMinimum() < -500.0F && volume.intensityMaximum() > 500.0F;
}

void OpenGLVolumeViewerWidget::applyCtWindowLevelPresetIfNeeded(const VolumeData& volume)
{
  if (!looksLikeCtVolume(volume))
  {
    return;
  }

  setWindowLevelControls(windowSpinBox_, levelSpinBox_, kLungWindow, kLungLevel);
  setWindowLevelPresetIndex(windowLevelPresetComboBox_, kPresetIndexLung);
}

bool OpenGLVolumeViewerWidget::maskMatchesCurrentVolume(const VolumeData& mask) const
{
  return currentVolume_ && currentVolume_->isValid() && mask.isValid() &&
         currentVolume_->width() == mask.width() && currentVolume_->height() == mask.height() &&
         currentVolume_->depth() == mask.depth();
}

QImage OpenGLVolumeViewerWidget::applyMaskOverlay(const QImage& baseImage,
                                                  const SliceData& maskSlice) const
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
  const int opacity = std::clamp(maskOpacityPercent_, 0, 100);
  const int basePercent = 100 - opacity;

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
          static_cast<uchar>(((static_cast<int>(scanLine[channelOffset + 0]) * basePercent) +
                              (255 * opacity)) /
                             100);
      scanLine[channelOffset + 1] =
          static_cast<uchar>((static_cast<int>(scanLine[channelOffset + 1]) * basePercent) / 100);
      scanLine[channelOffset + 2] =
          static_cast<uchar>((static_cast<int>(scanLine[channelOffset + 2]) * basePercent) / 100);
    }
  }

  return overlayImage;
}

QString OpenGLVolumeViewerWidget::formatVolumeMetadata() const
{
  if (!currentVolume_ || !currentVolume_->isValid() || !currentVolumeRange_.has_value())
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

std::size_t OpenGLVolumeViewerWidget::currentSliceCount() const
{
  if (!hasCurrentVolumeSlice_ || !currentVolume_ || !currentVolume_->isValid())
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

std::pair<std::size_t, std::size_t> OpenGLVolumeViewerWidget::sliceDimensions() const
{
  if (!hasCurrentVolumeSlice_ || !currentVolume_ || !currentVolume_->isValid())
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

QImage OpenGLVolumeViewerWidget::createDemoImage()
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

VolumeData OpenGLVolumeViewerWidget::createSyntheticVolume()
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

std::size_t OpenGLVolumeViewerWidget::normalizeToPixelIndex(double normalized, std::size_t size)
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
