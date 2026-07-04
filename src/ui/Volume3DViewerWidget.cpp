#include "qtviewerpro/ui/Volume3DViewerWidget.h"

#include <QLabel>
#include <QVBoxLayout>

namespace qvp
{

Volume3DViewerWidget::Volume3DViewerWidget(QWidget* parent) : QWidget(parent)
{
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(10);

  auto* titleLabel = new QLabel("3D Volume Viewer", this);
  auto titleFont = titleLabel->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  titleLabel->setFont(titleFont);
  titleLabel->setAlignment(Qt::AlignCenter);

  auto* subtitleLabel =
      new QLabel("OpenGL volume rendering will be added in the next step.", this);
  auto subtitleFont = subtitleLabel->font();
  subtitleFont.setPointSize(subtitleFont.pointSize() + 1);
  subtitleLabel->setFont(subtitleFont);
  subtitleLabel->setAlignment(Qt::AlignCenter);
  subtitleLabel->setWordWrap(true);

  layout->addStretch(1);
  layout->addWidget(titleLabel);
  layout->addWidget(subtitleLabel);
  layout->addStretch(2);
}

} // namespace qvp
