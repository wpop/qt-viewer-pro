#include "qtviewerpro/ui/Volume3DViewerWidget.h"

#include "qtviewerpro/render/OpenGLVolumeRendererWidget.h"

#include <QLabel>
#include <QString>
#include <QVBoxLayout>
#include <QDebug>

#include <chrono>
#include <utility>

namespace qvp
{

namespace
{
using Clock = std::chrono::steady_clock;

double durationMilliseconds(const Clock::duration& duration)
{
  return std::chrono::duration<double, std::milli>(duration).count();
}
} // namespace

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
  const auto totalStart = Clock::now();
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

  const auto ownershipStart = Clock::now();
  currentVolume_ = std::move(volume);
  const auto ownershipEnd = Clock::now();

  const auto rendererStart = Clock::now();
  if (rendererWidget_)
  {
    rendererWidget_->setVolume(currentVolume_);
  }
  const auto rendererEnd = Clock::now();

  const auto statusStart = Clock::now();
  if (statusLabel_)
  {
    statusLabel_->setText(QString("Volume ready: %1 x %2 x %3 voxels")
                              .arg(currentVolume_->width())
                              .arg(currentVolume_->height())
                              .arg(currentVolume_->depth()));
  }
  const auto statusEnd = Clock::now();

  const auto totalEnd = Clock::now();

  qDebug().noquote()
      << QStringLiteral("3D viewer timings:\n"
                        "  ownership move:        %1 ms\n"
                        "  renderer setVolume:    %2 ms\n"
                        "  status label update:   %3 ms\n"
                        "  setVolume total:       %4 ms")
             .arg(QString::number(durationMilliseconds(ownershipEnd - ownershipStart), 'f', 1))
             .arg(QString::number(durationMilliseconds(rendererEnd - rendererStart), 'f', 1))
             .arg(QString::number(durationMilliseconds(statusEnd - statusStart), 'f', 1))
             .arg(QString::number(durationMilliseconds(totalEnd - totalStart), 'f', 1));
}

void Volume3DViewerWidget::setRenderPreset(VolumeRenderPreset preset)
{
  if (rendererWidget_)
  {
    rendererWidget_->setRenderPreset(preset);
  }
}

void Volume3DViewerWidget::setGlobalOpacity(float opacity)
{
  if (rendererWidget_)
  {
    rendererWidget_->setGlobalOpacity(opacity);
  }
}

void Volume3DViewerWidget::setManualIntensityRange(float minimum, float maximum)
{
  if (rendererWidget_)
  {
    rendererWidget_->setManualIntensityRange(minimum, maximum);
  }
}

VolumeTransferFunctionState Volume3DViewerWidget::transferFunctionState() const
{
  if (rendererWidget_)
  {
    return rendererWidget_->transferFunctionState();
  }

  return {};
}

void Volume3DViewerWidget::resetView()
{
  if (rendererWidget_)
  {
    rendererWidget_->resetView();
  }
}

} // namespace qvp
