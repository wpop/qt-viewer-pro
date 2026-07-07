#pragma once

#include "qtviewerpro/ui/MprCoordinateMapper.h"
#include "qtviewerpro/core/SliceOrientation.h"
#include "qtviewerpro/core/VolumeData.h"

#include <QWidget>

#include <cstddef>
#include <memory>

class QLabel;
class QSlider;

namespace qvp
{

class ImageViewer2D;

class MprViewerWidget : public QWidget
{
  Q_OBJECT

public:
  explicit MprViewerWidget(QWidget* parent = nullptr);
  ~MprViewerWidget() override = default;

  void setVolume(VolumeData volume);
  void setVolume(std::shared_ptr<const VolumeData> volume);

private:
  struct SlicePane
  {
    SliceOrientation orientation = SliceOrientation::Axial;
    QLabel* titleLabel = nullptr;
    QLabel* coordinateLabel = nullptr;
    QSlider* sliceSlider = nullptr;
    QLabel* sliceValueLabel = nullptr;
    ImageViewer2D* viewer = nullptr;
  };

  void createUi();
  void connectSignals();
  void refreshAllSlices();
  void refreshSlicePane(SlicePane& pane);
  void setDefaultWindowLevel();
  void updatePositionForOrientation(SliceOrientation orientation, int delta);
  void updatePositionFromImageClick(SliceOrientation orientation, int imageX, int imageY);
  std::size_t sliceCountForOrientation(SliceOrientation orientation) const;
  std::size_t currentSliceIndexForOrientation(SliceOrientation orientation) const;

  SlicePane axialPane_;
  SlicePane sagittalPane_;
  SlicePane coronalPane_;
  std::shared_ptr<const VolumeData> currentVolume_;
  MprVoxelPosition currentPosition_{};
  float window_ = 255.0F;
  float level_ = 127.0F;
};

} // namespace qvp
