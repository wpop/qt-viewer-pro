#pragma once

#include "qtviewerpro/core/SliceOrientation.h"
#include "qtviewerpro/core/VolumeData.h"

#include <QWidget>

#include <cstddef>
#include <memory>

class QLabel;

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
  struct VoxelPosition
  {
    std::size_t x = 0;
    std::size_t y = 0;
    std::size_t z = 0;
  };

  struct SlicePane
  {
    SliceOrientation orientation = SliceOrientation::Axial;
    QLabel* titleLabel = nullptr;
    ImageViewer2D* viewer = nullptr;
  };

  void createUi();
  void connectSignals();
  void refreshAllSlices();
  void refreshSlicePane(SlicePane& pane);
  void updateSharedPositionLabel();
  void setDefaultWindowLevel();
  void updatePositionForOrientation(SliceOrientation orientation, int delta);
  std::size_t sliceCountForOrientation(SliceOrientation orientation) const;
  std::size_t currentSliceIndexForOrientation(SliceOrientation orientation) const;

  SlicePane axialPane_;
  SlicePane sagittalPane_;
  SlicePane coronalPane_;
  QLabel* sharedPositionLabel_ = nullptr;
  std::shared_ptr<const VolumeData> currentVolume_;
  VoxelPosition currentPosition_{};
  float window_ = 255.0F;
  float level_ = 127.0F;
};

} // namespace qvp
