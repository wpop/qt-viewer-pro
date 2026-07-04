#pragma once

#include <QWidget>

namespace qvp
{

class Volume3DViewerWidget : public QWidget
{
  Q_OBJECT

public:
  explicit Volume3DViewerWidget(QWidget* parent = nullptr);
};

} // namespace qvp
