#pragma once

#include "qtviewerpro/core/SliceOrientation.h"
#include "qtviewerpro/core/VolumeData.h"

#include <QDialog>

class QLabel;
class QSlider;

namespace qvp
{

class VolumeToolsWindow : public QDialog
{
  Q_OBJECT

public:
  explicit VolumeToolsWindow(QWidget* parent = nullptr);

  void setVolume(const VolumeData* volume);
  void clearVolume();
  void setSliceNavigationState(SliceOrientation orientation, int currentIndex, int maximumIndex);

signals:
  void medicalViewRequested();
  void mprViewRequested();
  void volume3DViewRequested();
  void sliceNavigationRequested(qvp::SliceOrientation orientation, int sliceIndex);

private:
  struct NavigationRow
  {
    QSlider* slider = nullptr;
    QLabel* valueLabel = nullptr;
  };

  void createUi();
  void resetInformationLabels();
  void resetNavigationControls();
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

  NavigationRow axialNavigationRow_;
  NavigationRow sagittalNavigationRow_;
  NavigationRow coronalNavigationRow_;
};

} // namespace qvp
