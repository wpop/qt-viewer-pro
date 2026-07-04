#pragma once

#include "qtviewerpro/core/VolumeData.h"

#include <memory>
#include <QWidget>

class QLabel;

namespace qvp
{

class Volume3DViewerWidget : public QWidget
{
  Q_OBJECT

public:
  explicit Volume3DViewerWidget(QWidget* parent = nullptr);

  void setVolume(std::shared_ptr<const VolumeData> volume);

private:
  std::shared_ptr<const VolumeData> currentVolume_;
  QLabel* statusLabel_ = nullptr;
};

} // namespace qvp
