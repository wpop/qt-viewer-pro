#pragma once

#include "qtviewerpro/core/SliceOrientation.h"
#include "qtviewerpro/core/VolumeData.h"
#include "qtviewerpro/render/VolumeTransferFunction.h"

#include <QDialog>

class QLabel;
class QComboBox;
class QDoubleSpinBox;
class QSlider;

namespace qvp
{

class VolumeToolsWindow : public QDialog
{
  Q_OBJECT

public:
  explicit VolumeToolsWindow(QWidget* parent = nullptr);

  void setVolume(const VolumeData* volume);
  void setTransferFunctionState(const VolumeTransferFunctionState& state);
  void clearVolume();
  void setSliceNavigationState(SliceOrientation orientation, int currentIndex, int maximumIndex);

signals:
  void medicalViewRequested();
  void mprViewRequested();
  void volume3DViewRequested();
  void renderPresetRequested(qvp::VolumeRenderPreset preset);
  void globalOpacityRequested(int opacityPercent);
  void manualIntensityRangeRequested(double minimum, double maximum);
  void sliceNavigationRequested(qvp::SliceOrientation orientation, int sliceIndex);

private:
  struct NavigationRow
  {
    QSlider* slider = nullptr;
    QLabel* valueLabel = nullptr;
  };

  void createUi();
  void resetInformationLabels();
  void resetTransferFunctionControls();
  void resetNavigationControls();
  void updateOpacityValueLabel(int opacityPercent);
  void emitManualIntensityRangeRequested();
  void updateNavigationRow(NavigationRow& row, int currentIndex, int maximumIndex);
  NavigationRow& navigationRowForOrientation(SliceOrientation orientation);

  QLabel* dimensionsValueLabel_ = nullptr;
  QLabel* spacingValueLabel_ = nullptr;
  QLabel* voxelCountValueLabel_ = nullptr;
  QLabel* memoryValueLabel_ = nullptr;
  QLabel* intensityRangeValueLabel_ = nullptr;
  QLabel* coordinateSystemValueLabel_ = nullptr;
  QLabel* patientWorldOrientationValueLabel_ = nullptr;
  QLabel* voxelAxisAnatomyValueLabel_ = nullptr;
  QLabel* originValueLabel_ = nullptr;
  QLabel* directionValueLabel_ = nullptr;
  QComboBox* renderPresetComboBox_ = nullptr;
  QSlider* opacitySlider_ = nullptr;
  QLabel* opacityValueLabel_ = nullptr;
  QDoubleSpinBox* intensityMinimumSpinBox_ = nullptr;
  QDoubleSpinBox* intensityMaximumSpinBox_ = nullptr;

  NavigationRow axialNavigationRow_;
  NavigationRow sagittalNavigationRow_;
  NavigationRow coronalNavigationRow_;
};

} // namespace qvp
