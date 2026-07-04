#include "qtviewerpro/ui/Volume3DViewerWidget.h"

#include "qtviewerpro/render/OpenGLVolumeRendererWidget.h"

#include <QLabel>
#include <QVBoxLayout>

namespace qvp
{

Volume3DViewerWidget::Volume3DViewerWidget(QWidget* parent) : QWidget(parent)
{
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(12);

  auto* titleLabel = new QLabel("3D Volume Viewer", this);
  auto titleFont = titleLabel->font();
  titleFont.setPointSize(titleFont.pointSize() + 8);
  titleFont.setBold(true);
  titleLabel->setFont(titleFont);
  titleLabel->setAlignment(Qt::AlignCenter);

  layout->addWidget(titleLabel);

  auto* rendererWidget = new OpenGLVolumeRendererWidget(this);
  layout->addWidget(rendererWidget, 1);

  auto* statusLabel = new QLabel("OpenGL renderer ready", this);
  auto statusFont = statusLabel->font();
  statusFont.setPointSize(statusFont.pointSize() - 1);
  statusLabel->setFont(statusFont);
  statusLabel->setAlignment(Qt::AlignCenter);
  layout->addWidget(statusLabel);
}

} // namespace qvp
