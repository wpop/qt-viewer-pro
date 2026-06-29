#pragma once

#include "qtviewerpro/core/SliceOrientation.h"
#include "qtviewerpro/core/VolumeData.h"

#include <QPointF>
#include <QSize>
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

class OpenGLDemoWindow : public QWidget
{
  Q_OBJECT

public:
  explicit OpenGLDemoWindow(QWidget* parent = nullptr);
  ~OpenGLDemoWindow() override = default;

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
  void updateVolumeSlice();
  void configureSliceSlider();
  void updateSliceLabel();
  void resetToAxialMiddleSlice();
  std::size_t currentSliceCount() const;
  std::pair<std::size_t, std::size_t> sliceDimensions() const;

  static QImage createDemoImage();
  static VolumeData createSyntheticVolume();
  static std::size_t normalizeToPixelIndex(double normalized, std::size_t size);

  OpenGLSliceViewer* openGLViewer_ = nullptr;
  QPushButton* openImageButton_ = nullptr;
  QPushButton* loadSyntheticSliceButton_ = nullptr;
  QPushButton* loadRawSliceButton_ = nullptr;
  QPushButton* resetViewButton_ = nullptr;
  QPushButton* resetCrosshairButton_ = nullptr;
  QCheckBox* showCrosshairCheckBox_ = nullptr;
  QCheckBox* showImageBorderCheckBox_ = nullptr;
  QComboBox* orientationComboBox_ = nullptr;
  QSpinBox* windowSpinBox_ = nullptr;
  QSpinBox* levelSpinBox_ = nullptr;
  QPushButton* resetWindowLevelButton_ = nullptr;
  QComboBox* windowLevelPresetComboBox_ = nullptr;
  QPushButton* previousSliceButton_ = nullptr;
  QSlider* sliceSlider_ = nullptr;
  QLabel* sliceIndexLabel_ = nullptr;
  QLabel* crosshairPositionLabel_ = nullptr;
  QPushButton* nextSliceButton_ = nullptr;

  std::optional<VolumeData> currentVolume_;
  std::size_t currentSliceIndex_ = 0;
  SliceOrientation currentOrientation_ = SliceOrientation::Axial;
  bool hasCurrentVolumeSlice_ = false;
  QPointF currentCrosshairPosition_{0.0, 0.0};
  QSize currentDisplaySize_{0, 0};
};

} // namespace qvp
