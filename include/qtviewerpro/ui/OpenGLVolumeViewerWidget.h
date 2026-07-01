#pragma once

#include "qtviewerpro/core/SliceOrientation.h"
#include "qtviewerpro/core/VolumeData.h"

#include <QPointF>
#include <QSize>
#include <QString>
#include <QWidget>

#include <cstddef>
#include <optional>
#include <utility>

class QCheckBox;
class QComboBox;
class QImage;
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;

namespace qvp
{

class OpenGLSliceViewer;
class SliceData;

class OpenGLVolumeViewerWidget : public QWidget
{
  Q_OBJECT

public:
  explicit OpenGLVolumeViewerWidget(QWidget* parent = nullptr);
  ~OpenGLVolumeViewerWidget() override = default;

  void setVolume(VolumeData volume);
  void openMaskOverlay();

private:
  void createUi();
  void connectSignals();
  void loadInitialDemoImage();
  void openImage();
  void loadSyntheticSlice();
  void loadRawSlice();
  void setSliceIndex(int sliceIndex);
  void setSliceOrientation(int orientationIndex);
  void previousSlice();
  void nextSlice();
  void updateWindowLevel();
  void resetWindowLevel();
  void applyWindowLevelPreset(int presetIndex);
  void updateCrosshairPosition(QPointF position, int value);
  void updateCurrentCrosshairPosition(QPointF position);
  void updateCrosshairLabel(int value);
  bool voxelCoordinatesInBounds(const VolumeData& volume, std::size_t x, std::size_t y, std::size_t z) const;
  float voxelValueAt(const VolumeData& volume, std::size_t x, std::size_t y, std::size_t z) const;
  void updateMaskOpacity(int opacityPercent);
  void updateMaskOpacityControls();
  void updateVolumeSlice();
  void configureSliceSlider();
  void updateSliceLabel();
  void updateVolumeMetadataLabel();
  void resetToAxialMiddleSlice();
  bool looksLikeCtVolume(const VolumeData& volume) const;
  void applyCtWindowLevelPresetIfNeeded(const VolumeData& volume);
  bool maskMatchesCurrentVolume(const VolumeData& mask) const;
  QImage applyMaskOverlay(const QImage& baseImage, const SliceData& maskSlice) const;
  QString formatVolumeMetadata() const;
  std::size_t currentSliceCount() const;
  std::pair<std::size_t, std::size_t> sliceDimensions() const;

  static QImage createDemoImage();
  static VolumeData createSyntheticVolume();
  static std::size_t normalizeToPixelIndex(double normalized, std::size_t size);

  OpenGLSliceViewer* openGLViewer_ = nullptr;
  QPushButton* openImageButton_ = nullptr;
  QPushButton* openMaskOverlayButton_ = nullptr;
  QPushButton* loadSyntheticSliceButton_ = nullptr;
  QPushButton* loadRawSliceButton_ = nullptr;
  QPushButton* resetViewButton_ = nullptr;
  QPushButton* resetCrosshairButton_ = nullptr;
  QCheckBox* showCrosshairCheckBox_ = nullptr;
  QCheckBox* showImageBorderCheckBox_ = nullptr;
  QCheckBox* showMaskOverlayCheckBox_ = nullptr;
  QLabel* maskOpacityValueLabel_ = nullptr;
  QSlider* maskOpacitySlider_ = nullptr;
  QComboBox* orientationComboBox_ = nullptr;
  QSpinBox* windowSpinBox_ = nullptr;
  QSpinBox* levelSpinBox_ = nullptr;
  QPushButton* resetWindowLevelButton_ = nullptr;
  QComboBox* windowLevelPresetComboBox_ = nullptr;
  QPushButton* previousSliceButton_ = nullptr;
  QSlider* sliceSlider_ = nullptr;
  QLabel* sliceIndexLabel_ = nullptr;
  QLabel* crosshairPositionLabel_ = nullptr;
  QLabel* volumeMetadataLabel_ = nullptr;
  QPushButton* nextSliceButton_ = nullptr;

  std::optional<VolumeData> currentVolume_;
  std::optional<VolumeData> maskVolume_;
  std::optional<std::pair<float, float>> currentVolumeRange_;
  std::size_t currentSliceIndex_ = 0;
  SliceOrientation currentOrientation_ = SliceOrientation::Axial;
  bool hasCurrentVolumeSlice_ = false;
  QPointF currentCrosshairPosition_{0.0, 0.0};
  QSize currentDisplaySize_{0, 0};
  int maskOpacityPercent_ = 30;
};

} // namespace qvp
