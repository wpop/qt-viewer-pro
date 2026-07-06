#include "qtviewerpro/ui/Volume3DViewerWidget.h"

#include "qtviewerpro/render/OpenGLVolumeRendererWidget.h"

#include <QLabel>
#include <QString>
#include <QVBoxLayout>

#include <utility>

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

  rendererWidget_ = new OpenGLVolumeRendererWidget(this);
  layout->addWidget(rendererWidget_, 1);

  connect(rendererWidget_, &OpenGLVolumeRendererWidget::volumeTextureUploaded, this,
          [this](int width, int height, int depth) {
            if (statusLabel_)
            {
              statusLabel_->setText(QString("GPU texture ready: %1 x %2 x %3")
                                        .arg(width)
                                        .arg(height)
                                        .arg(depth));
            }
          });

  connect(rendererWidget_, &OpenGLVolumeRendererWidget::volumeTextureUploadFailed, this,
          [this](const QString& message) {
            if (statusLabel_)
            {
              statusLabel_->setText(QString("GPU texture upload failed: %1").arg(message));
            }
          });

  statusLabel_ = new QLabel("OpenGL renderer ready", this);
  auto statusFont = statusLabel_->font();
  statusFont.setPointSize(statusFont.pointSize() - 1);
  statusLabel_->setFont(statusFont);
  statusLabel_->setAlignment(Qt::AlignCenter);
  layout->addWidget(statusLabel_);
}

void Volume3DViewerWidget::setVolume(std::shared_ptr<const VolumeData> volume)
{
  if (!volume || !volume->isValid())
  {
    currentVolume_.reset();
    if (rendererWidget_)
    {
      rendererWidget_->setVolume(nullptr);
    }
    if (statusLabel_)
    {
      statusLabel_->setText("OpenGL renderer ready");
    }
    return;
  }

  currentVolume_ = std::move(volume);
  if (rendererWidget_)
  {
    rendererWidget_->setVolume(currentVolume_);
  }
  if (statusLabel_)
  {
    statusLabel_->setText(QString("Volume ready: %1 x %2 x %3 voxels")
                              .arg(currentVolume_->width())
                              .arg(currentVolume_->height())
                              .arg(currentVolume_->depth()));
  }
}

void Volume3DViewerWidget::setRenderPreset(VolumeRenderPreset preset)
{
  if (rendererWidget_)
  {
    rendererWidget_->setRenderPreset(preset);
  }
}

void Volume3DViewerWidget::resetView()
{
  if (rendererWidget_)
  {
    rendererWidget_->resetView();
  }
}

} // namespace qvp
