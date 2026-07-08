#include "qtviewerpro/ui/VolumeToolsWindow.h"

#include "qtviewerpro/core/AnatomicalOrientation.h"
#include "qtviewerpro/core/VolumeInformation.h"

#include <QFontDatabase>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QSignalBlocker>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

namespace
{

QString volumeToolsWindowStyleSheet()
{
  return QStringLiteral(R"(
QDialog {
  background-color: #1E1E1E;
  color: #E6E6E6;
}

QGroupBox {
  color: #E6E6E6;
  border: 1px solid #3A3A3A;
  border-radius: 4px;
  margin-top: 12px;
  padding-top: 8px;
}

QGroupBox::title {
  subcontrol-origin: margin;
  left: 8px;
  padding: 0 4px;
  color: #E6E6E6;
}

QLabel {
  color: #E6E6E6;
}

QComboBox, QDoubleSpinBox {
  background-color: #333333;
  color: #E6E6E6;
  border: 1px solid #3A3A3A;
  border-radius: 4px;
  padding: 3px 6px;
}

QComboBox:focus, QDoubleSpinBox:focus {
  border-color: #4FC3F7;
}

QComboBox QAbstractItemView {
  background-color: #252526;
  color: #E6E6E6;
  selection-background-color: #315A6D;
  selection-color: #FFFFFF;
}

QPushButton {
  background-color: #333333;
  color: #E6E6E6;
  border: 1px solid #3A3A3A;
  border-radius: 4px;
  padding: 4px 8px;
}

QPushButton:hover {
  background-color: #3A3A3A;
}

QPushButton:pressed {
  background-color: #444444;
  color: #FFFFFF;
}

QPushButton:disabled {
  color: #777777;
  background-color: #252526;
}

QSlider::groove:horizontal {
  height: 4px;
  background: #3A3A3A;
  border-radius: 2px;
}

QSlider::handle:horizontal {
  width: 14px;
  margin: -5px 0;
  background: #4FC3F7;
  border-radius: 7px;
}

QSlider:disabled {
  background: transparent;
}

QSlider::groove:horizontal:disabled {
  background: #252526;
}

QSlider::handle:horizontal:disabled {
  background: #777777;
}

QComboBox:disabled, QDoubleSpinBox:disabled {
  color: #777777;
  background-color: #252526;
}
)");
}

QString formatDimensions(const qvp::VolumeInformation& information)
{
  return QStringLiteral("%1 × %2 × %3")
      .arg(qulonglong(information.width))
      .arg(qulonglong(information.height))
      .arg(qulonglong(information.depth));
}

QString formatSpacing(const qvp::VolumeInformation& information)
{
  return QStringLiteral("%1 × %2 × %3 mm")
      .arg(information.spacingX, 0, 'f', 3)
      .arg(information.spacingY, 0, 'f', 3)
      .arg(information.spacingZ, 0, 'f', 3);
}

QString formatVoxelCount(const qvp::VolumeInformation& information)
{
  return QLocale().toString(qulonglong(information.voxelCount));
}

QString formatMemory(std::size_t memoryBytes)
{
  const double memoryMiB = static_cast<double>(memoryBytes) / (1024.0 * 1024.0);
  const int decimals = memoryMiB < 10.0 ? 2 : 1;
  return QStringLiteral("%1 MiB").arg(memoryMiB, 0, 'f', decimals);
}

QString formatIntensityRange(const qvp::VolumeInformation& information)
{
  if (!information.hasIntensityRange)
  {
    return QStringLiteral("Not available");
  }

  return QStringLiteral("%1 to %2")
      .arg(information.intensityMinimum, 0, 'f', 1)
      .arg(information.intensityMaximum, 0, 'f', 1);
}

QString formatCoordinateSystem(qvp::VolumeData::CoordinateSystem coordinateSystem)
{
  switch (coordinateSystem)
  {
  case qvp::VolumeData::CoordinateSystem::LPS:
    return QStringLiteral("LPS");
  case qvp::VolumeData::CoordinateSystem::RAS:
    return QStringLiteral("RAS");
  case qvp::VolumeData::CoordinateSystem::Unknown:
    return QStringLiteral("Unknown");
  }

  return QStringLiteral("Unknown");
}

QString formatPatientWorldOrientation(bool trusted)
{
  return trusted ? QStringLiteral("Trusted") : QStringLiteral("Untrusted");
}

QString formatVoxelAxisAnatomy(const std::optional<qvp::VoxelAxisAnatomy>& anatomy)
{
  if (!anatomy.has_value())
  {
    return QStringLiteral("Not available");
  }

  const auto acronym = qvp::anatomicalOrientationAcronym(*anatomy);
  if (!acronym.has_value())
  {
    return QStringLiteral("Not available");
  }

  return QString::fromStdString(*acronym);
}

QString formatOrigin(const qvp::VolumeInformation& information)
{
  return QStringLiteral("(%1, %2, %3) mm")
      .arg(information.origin[0], 0, 'f', 3)
      .arg(information.origin[1], 0, 'f', 3)
      .arg(information.origin[2], 0, 'f', 3);
}

QString formatDirection(const qvp::VolumeInformation& information)
{
  return QStringLiteral("[ %1  %2  %3 ]\n"
                        "[ %4  %5  %6 ]\n"
                        "[ %7  %8  %9 ]")
      .arg(information.direction[0], 6, 'f', 3)
      .arg(information.direction[1], 6, 'f', 3)
      .arg(information.direction[2], 6, 'f', 3)
      .arg(information.direction[3], 6, 'f', 3)
      .arg(information.direction[4], 6, 'f', 3)
      .arg(information.direction[5], 6, 'f', 3)
      .arg(information.direction[6], 6, 'f', 3)
      .arg(information.direction[7], 6, 'f', 3)
      .arg(information.direction[8], 6, 'f', 3);
}

QLabel* createValueLabel(QWidget* parent)
{
  auto* label = new QLabel(parent);
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  label->setWordWrap(true);
  label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  return label;
}

QString formatOpacityValue(int opacityPercent)
{
  return QStringLiteral("%1%").arg(opacityPercent);
}

} // namespace

namespace qvp
{

VolumeToolsWindow::VolumeToolsWindow(QWidget* parent)
    : QDialog(parent, Qt::Tool | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                          Qt::WindowCloseButtonHint)
{
  setWindowTitle(QStringLiteral("Volume Tools"));
  setModal(false);
  resize(560, 640);
  setStyleSheet(volumeToolsWindowStyleSheet());
  createUi();
  clearVolume();
}

void VolumeToolsWindow::setVolume(const VolumeData* volume)
{
  if (volume == nullptr || !volume->isValid())
  {
    clearVolume();
    return;
  }

  const VolumeInformation information = makeVolumeInformation(*volume);
  dimensionsValueLabel_->setText(formatDimensions(information));
  spacingValueLabel_->setText(formatSpacing(information));
  voxelCountValueLabel_->setText(formatVoxelCount(information));
  memoryValueLabel_->setText(formatMemory(information.memoryBytes));
  intensityRangeValueLabel_->setText(formatIntensityRange(information));
  coordinateSystemValueLabel_->setText(formatCoordinateSystem(information.coordinateSystem));
  patientWorldOrientationValueLabel_->setText(
      formatPatientWorldOrientation(information.patientWorldOrientationTrusted));
  voxelAxisAnatomyValueLabel_->setText(formatVoxelAxisAnatomy(information.voxelAxisAnatomy));
  originValueLabel_->setText(formatOrigin(information));
  directionValueLabel_->setText(formatDirection(information));

  if (information.hasIntensityRange)
  {
    const QSignalBlocker minimumBlocker(intensityMinimumSpinBox_);
    const QSignalBlocker maximumBlocker(intensityMaximumSpinBox_);
    intensityMinimumSpinBox_->setEnabled(true);
    intensityMaximumSpinBox_->setEnabled(true);
    intensityMinimumSpinBox_->setRange(information.intensityMinimum, information.intensityMaximum);
    intensityMaximumSpinBox_->setRange(information.intensityMinimum, information.intensityMaximum);
  }
  else
  {
    intensityMinimumSpinBox_->setEnabled(false);
    intensityMaximumSpinBox_->setEnabled(false);
  }

  renderPresetComboBox_->setEnabled(true);
  opacitySlider_->setEnabled(true);
}

void VolumeToolsWindow::setTransferFunctionState(const VolumeTransferFunctionState& state)
{
  const int opacityPercent = static_cast<int>(std::lround(state.globalOpacity * 100.0F));
  const QSignalBlocker presetBlocker(renderPresetComboBox_);
  const QSignalBlocker opacityBlocker(opacitySlider_);
  const QSignalBlocker minimumBlocker(intensityMinimumSpinBox_);
  const QSignalBlocker maximumBlocker(intensityMaximumSpinBox_);

  renderPresetComboBox_->setCurrentIndex(static_cast<int>(state.renderPreset));
  opacitySlider_->setValue(std::clamp(opacityPercent, 0, 100));
  updateOpacityValueLabel(std::clamp(opacityPercent, 0, 100));
  intensityMinimumSpinBox_->setValue(state.intensityMinimum);
  intensityMaximumSpinBox_->setValue(state.intensityMaximum);
}

void VolumeToolsWindow::clearVolume()
{
  resetInformationLabels();
  resetTransferFunctionControls();
  resetNavigationControls();
}

void VolumeToolsWindow::setSliceNavigationState(SliceOrientation orientation,
                                                int currentIndex,
                                                int maximumIndex)
{
  updateNavigationRow(navigationRowForOrientation(orientation), currentIndex, maximumIndex);
}

void VolumeToolsWindow::createUi()
{
  auto* rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(10, 10, 10, 10);
  rootLayout->setSpacing(10);

  auto* informationGroupBox = new QGroupBox(QStringLiteral("Information"), this);
  auto* informationLayout = new QFormLayout(informationGroupBox);
  informationLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  informationLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignTop);
  informationLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);

  dimensionsValueLabel_ = createValueLabel(informationGroupBox);
  spacingValueLabel_ = createValueLabel(informationGroupBox);
  voxelCountValueLabel_ = createValueLabel(informationGroupBox);
  memoryValueLabel_ = createValueLabel(informationGroupBox);
  intensityRangeValueLabel_ = createValueLabel(informationGroupBox);
  coordinateSystemValueLabel_ = createValueLabel(informationGroupBox);
  patientWorldOrientationValueLabel_ = createValueLabel(informationGroupBox);
  voxelAxisAnatomyValueLabel_ = createValueLabel(informationGroupBox);
  originValueLabel_ = createValueLabel(informationGroupBox);
  directionValueLabel_ = createValueLabel(informationGroupBox);
  directionValueLabel_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

  informationLayout->addRow(QStringLiteral("Dimensions"), dimensionsValueLabel_);
  informationLayout->addRow(QStringLiteral("Spacing"), spacingValueLabel_);
  informationLayout->addRow(QStringLiteral("Voxel count"), voxelCountValueLabel_);
  informationLayout->addRow(QStringLiteral("Memory"), memoryValueLabel_);
  informationLayout->addRow(QStringLiteral("Intensity range"), intensityRangeValueLabel_);
  informationLayout->addRow(QStringLiteral("Coordinate system"), coordinateSystemValueLabel_);
  informationLayout->addRow(QStringLiteral("Patient-world orientation"),
                            patientWorldOrientationValueLabel_);
  informationLayout->addRow(QStringLiteral("Voxel-axis anatomy"), voxelAxisAnatomyValueLabel_);
  informationLayout->addRow(QStringLiteral("Origin"), originValueLabel_);
  informationLayout->addRow(QStringLiteral("Direction"), directionValueLabel_);
  rootLayout->addWidget(informationGroupBox);

  auto* transferGroupBox = new QGroupBox(QStringLiteral("3D Transfer"), this);
  auto* transferLayout = new QFormLayout(transferGroupBox);
  transferLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  transferLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignTop);
  transferLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);

  renderPresetComboBox_ = new QComboBox(transferGroupBox);
  renderPresetComboBox_->addItems(
      {QStringLiteral("Default"), QStringLiteral("CT Bone"), QStringLiteral("CT Lung"),
       QStringLiteral("Custom")});

  opacitySlider_ = new QSlider(Qt::Horizontal, transferGroupBox);
  opacitySlider_->setRange(0, 100);
  opacitySlider_->setValue(100);
  opacitySlider_->setSingleStep(1);
  opacityValueLabel_ = new QLabel(QStringLiteral("100%"), transferGroupBox);
  opacityValueLabel_->setMinimumWidth(48);
  opacityValueLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  auto* opacityRowWidget = new QWidget(transferGroupBox);
  auto* opacityRowLayout = new QHBoxLayout(opacityRowWidget);
  opacityRowLayout->setContentsMargins(0, 0, 0, 0);
  opacityRowLayout->setSpacing(8);
  opacityRowLayout->addWidget(opacitySlider_, 1);
  opacityRowLayout->addWidget(opacityValueLabel_);

  intensityMinimumSpinBox_ = new QDoubleSpinBox(transferGroupBox);
  intensityMinimumSpinBox_->setDecimals(1);
  intensityMinimumSpinBox_->setSingleStep(1.0);
  intensityMinimumSpinBox_->setKeyboardTracking(false);
  intensityMinimumSpinBox_->setRange(0.0, 0.0);

  intensityMaximumSpinBox_ = new QDoubleSpinBox(transferGroupBox);
  intensityMaximumSpinBox_->setDecimals(1);
  intensityMaximumSpinBox_->setSingleStep(1.0);
  intensityMaximumSpinBox_->setKeyboardTracking(false);
  intensityMaximumSpinBox_->setRange(0.0, 0.0);

  transferLayout->addRow(QStringLiteral("Mode / Preset"), renderPresetComboBox_);
  transferLayout->addRow(QStringLiteral("Opacity"), opacityRowWidget);
  transferLayout->addRow(QStringLiteral("Intensity Minimum"), intensityMinimumSpinBox_);
  transferLayout->addRow(QStringLiteral("Intensity Maximum"), intensityMaximumSpinBox_);
  rootLayout->addWidget(transferGroupBox);

  auto* viewGroupBox = new QGroupBox(QStringLiteral("View"), this);
  auto* viewLayout = new QHBoxLayout(viewGroupBox);
  auto* medicalViewButton = new QPushButton(QStringLiteral("Medical"), viewGroupBox);
  auto* mprViewButton = new QPushButton(QStringLiteral("MPR"), viewGroupBox);
  auto* volume3DViewButton = new QPushButton(QStringLiteral("3D"), viewGroupBox);
  medicalViewButton->setAutoDefault(false);
  medicalViewButton->setDefault(false);
  mprViewButton->setAutoDefault(false);
  mprViewButton->setDefault(false);
  volume3DViewButton->setAutoDefault(false);
  volume3DViewButton->setDefault(false);
  viewLayout->addWidget(medicalViewButton);
  viewLayout->addWidget(mprViewButton);
  viewLayout->addWidget(volume3DViewButton);
  rootLayout->addWidget(viewGroupBox);

  connect(medicalViewButton, &QPushButton::clicked, this, &VolumeToolsWindow::medicalViewRequested);
  connect(mprViewButton, &QPushButton::clicked, this, &VolumeToolsWindow::mprViewRequested);
  connect(volume3DViewButton, &QPushButton::clicked, this, &VolumeToolsWindow::volume3DViewRequested);
  connect(renderPresetComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this,
          [this](int presetIndex) {
            if (presetIndex < 0 ||
                presetIndex > static_cast<int>(VolumeRenderPreset::Custom))
            {
              return;
            }

            emit renderPresetRequested(static_cast<VolumeRenderPreset>(presetIndex));
          });
  connect(opacitySlider_, &QSlider::valueChanged, this, [this](int value) {
    updateOpacityValueLabel(value);
    emit globalOpacityRequested(value);
  });
  connect(intensityMinimumSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
          [this](double value) {
            if (value > intensityMaximumSpinBox_->value())
            {
              const QSignalBlocker blocker(intensityMinimumSpinBox_);
              intensityMinimumSpinBox_->setValue(intensityMaximumSpinBox_->value());
            }
            emitManualIntensityRangeRequested();
          });
  connect(intensityMaximumSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
          [this](double value) {
            if (value < intensityMinimumSpinBox_->value())
            {
              const QSignalBlocker blocker(intensityMaximumSpinBox_);
              intensityMaximumSpinBox_->setValue(intensityMinimumSpinBox_->value());
            }
            emitManualIntensityRangeRequested();
          });

  auto* navigationGroupBox = new QGroupBox(QStringLiteral("Navigation"), this);
  auto* navigationLayout = new QVBoxLayout(navigationGroupBox);

  auto createNavigationRow = [this, navigationGroupBox, navigationLayout](const QString& title,
                                                                           SliceOrientation orientation,
                                                                           NavigationRow& row) {
    auto* rowLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel(title, navigationGroupBox);
    titleLabel->setMinimumWidth(60);
    row.slider = new QSlider(Qt::Horizontal, navigationGroupBox);
    row.slider->setRange(0, 0);
    row.slider->setEnabled(false);
    row.valueLabel = new QLabel(QStringLiteral("0 / 0"), navigationGroupBox);
    row.valueLabel->setMinimumWidth(72);
    row.valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    rowLayout->addWidget(titleLabel);
    rowLayout->addWidget(row.slider, 1);
    rowLayout->addWidget(row.valueLabel);
    navigationLayout->addLayout(rowLayout);

    connect(row.slider, &QSlider::valueChanged, this, [this, orientation](int value) {
      emit sliceNavigationRequested(orientation, value);
    });
  };

  createNavigationRow(QStringLiteral("Axial"), SliceOrientation::Axial, axialNavigationRow_);
  createNavigationRow(QStringLiteral("Sagittal"), SliceOrientation::Sagittal, sagittalNavigationRow_);
  createNavigationRow(QStringLiteral("Coronal"), SliceOrientation::Coronal, coronalNavigationRow_);
  rootLayout->addWidget(navigationGroupBox);

  auto* closeButton = new QPushButton(QStringLiteral("Close"), this);
  closeButton->setAutoDefault(false);
  closeButton->setDefault(false);
  connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
  rootLayout->addWidget(closeButton, 0, Qt::AlignRight);
}

void VolumeToolsWindow::resetInformationLabels()
{
  const QString unavailable = QStringLiteral("Not available");
  dimensionsValueLabel_->clear();
  spacingValueLabel_->clear();
  voxelCountValueLabel_->clear();
  memoryValueLabel_->clear();
  intensityRangeValueLabel_->setText(unavailable);
  coordinateSystemValueLabel_->setText(QStringLiteral("Unknown"));
  patientWorldOrientationValueLabel_->setText(QStringLiteral("Untrusted"));
  voxelAxisAnatomyValueLabel_->setText(unavailable);
  originValueLabel_->clear();
  directionValueLabel_->clear();
}

void VolumeToolsWindow::resetTransferFunctionControls()
{
  const QSignalBlocker presetBlocker(renderPresetComboBox_);
  const QSignalBlocker opacityBlocker(opacitySlider_);
  const QSignalBlocker minimumBlocker(intensityMinimumSpinBox_);
  const QSignalBlocker maximumBlocker(intensityMaximumSpinBox_);

  renderPresetComboBox_->setCurrentIndex(static_cast<int>(VolumeRenderPreset::Default));
  opacitySlider_->setValue(100);
  updateOpacityValueLabel(100);
  intensityMinimumSpinBox_->setValue(0.0);
  intensityMaximumSpinBox_->setValue(0.0);

  renderPresetComboBox_->setEnabled(false);
  opacitySlider_->setEnabled(false);
  intensityMinimumSpinBox_->setEnabled(false);
  intensityMaximumSpinBox_->setEnabled(false);
}

void VolumeToolsWindow::resetNavigationControls()
{
  updateNavigationRow(axialNavigationRow_, 0, 0);
  updateNavigationRow(sagittalNavigationRow_, 0, 0);
  updateNavigationRow(coronalNavigationRow_, 0, 0);
}

void VolumeToolsWindow::updateOpacityValueLabel(int opacityPercent)
{
  opacityValueLabel_->setText(formatOpacityValue(std::clamp(opacityPercent, 0, 100)));
}

void VolumeToolsWindow::emitManualIntensityRangeRequested()
{
  emit manualIntensityRangeRequested(intensityMinimumSpinBox_->value(),
                                     intensityMaximumSpinBox_->value());
}

void VolumeToolsWindow::updateNavigationRow(NavigationRow& row, int currentIndex, int maximumIndex)
{
  const int clampedMaximumIndex = std::max(0, maximumIndex);
  const int clampedCurrentIndex = std::clamp(currentIndex, 0, clampedMaximumIndex);
  const QSignalBlocker blocker(row.slider);
  row.slider->setRange(0, clampedMaximumIndex);
  row.slider->setEnabled(clampedMaximumIndex > 0);
  row.slider->setValue(clampedCurrentIndex);
  row.valueLabel->setText(
      QStringLiteral("%1 / %2").arg(clampedCurrentIndex).arg(clampedMaximumIndex));
}

VolumeToolsWindow::NavigationRow&
VolumeToolsWindow::navigationRowForOrientation(SliceOrientation orientation)
{
  switch (orientation)
  {
  case SliceOrientation::Axial:
    return axialNavigationRow_;
  case SliceOrientation::Sagittal:
    return sagittalNavigationRow_;
  case SliceOrientation::Coronal:
    return coronalNavigationRow_;
  }

  throw std::invalid_argument("Unknown slice orientation");
}

} // namespace qvp
