#include "qtviewerpro/ui/MainWindow.h"
#include "qtviewerpro/core/SliceExtractor.h"
#include "qtviewerpro/core/SliceOrientation.h"
#include "qtviewerpro/core/VolumeData.h"
#include "qtviewerpro/io/ImageLoader.h"
#include "qtviewerpro/io/RawVolumeLoader.h"
#include "qtviewerpro/processing/ImageProcessor.h"
#include "qtviewerpro/processing/SliceImageConverter.h"
#include "qtviewerpro/render/OpenGLSliceViewer.h"
#include "qtviewerpro/ui/ImageViewer2D.h"

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include <QSize>
#include <QStyle>

#include <QIcon>
#include <QPainter>
#include <QPixmap>

#include <cmath>
#include <exception>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace
{
constexpr int kMaxRecentFiles = 5;

QIcon createTextIcon(const QString& text)
{
  QPixmap pixmap(24, 24);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);

  QFont font = painter.font();
  font.setBold(true);
  font.setPointSize(16);
  painter.setFont(font);

  painter.drawText(pixmap.rect(), Qt::AlignCenter, text);

  return QIcon(pixmap);
}

QString sliceOrientationName(qvp::SliceOrientation orientation)
{
  switch (orientation)
  {
  case qvp::SliceOrientation::Coronal:
    return "Coronal";
  case qvp::SliceOrientation::Sagittal:
    return "Sagittal";
  case qvp::SliceOrientation::Axial:
    return "Axial";
  }

  return "Axial";
}

QImage createOpenGLDemoImage()
{
  constexpr int kImageSize = 256;
  QImage image(kImageSize, kImageSize, QImage::Format_RGBA8888);

  for (int y = 0; y < image.height(); ++y)
  {
    auto* row = image.scanLine(y);
    for (int x = 0; x < image.width(); ++x)
    {
      const int offset = x * 4;
      const int gradient = (x + y) / 2;
      const bool inSquare = x >= 88 && x < 168 && y >= 88 && y < 168;
      const bool inCross = (x >= 124 && x < 132) || (y >= 124 && y < 132);
      const int value = inSquare || inCross ? 240 : 24 + gradient / 2;

      row[offset] = static_cast<uchar>(value);
      row[offset + 1] = static_cast<uchar>(value);
      row[offset + 2] = static_cast<uchar>(value);
      row[offset + 3] = 255;
    }
  }

  return image;
}

QString darkMedicalStyleSheet()
{
  return QStringLiteral(R"(
QMainWindow {
  background-color: #1E1E1E;
  color: #E6E6E6;
}

QMenuBar, QMenu, QToolBar, QStatusBar, QDockWidget {
  background-color: #252526;
  color: #E6E6E6;
  border-color: #3A3A3A;
}

QMenuBar::item:selected, QMenu::item:selected {
  background-color: #333333;
  color: #4FC3F7;
}

QToolBar {
  spacing: 4px;
  border-bottom: 1px solid #3A3A3A;
}

QToolButton, QPushButton {
  background-color: #333333;
  color: #E6E6E6;
  border: 1px solid #3A3A3A;
  border-radius: 4px;
  padding: 4px 8px;
}

QToolButton:hover, QPushButton:hover {
  background-color: #3A3A3A;
}

QToolButton:checked {
  background-color: #2A4A5A;
  color: #FFFFFF;
  border: 1px solid #4FC3F7;
}

QToolButton:checked:hover {
  background-color: #315A6D;
  color: #FFFFFF;
  border: 1px solid #4FC3F7;
}

QToolButton:pressed {
  background-color: #444444;
  color: #FFFFFF;
}

QPushButton:pressed {
  background-color: #444444;
}

QToolButton:disabled, QPushButton:disabled,
QComboBox:disabled, QSpinBox:disabled, QCheckBox:disabled {
  color: #777777;
  background-color: #252526;
}

QDockWidget::title {
  background-color: #252526;
  color: #E6E6E6;
  padding: 6px;
  border-bottom: 1px solid #3A3A3A;
}

QLabel, QCheckBox {
  color: #E6E6E6;
}

QComboBox, QSpinBox {
  background-color: #333333;
  color: #E6E6E6;
  border: 1px solid #3A3A3A;
  border-radius: 4px;
  padding: 3px 6px;
}

QComboBox:focus, QSpinBox:focus {
  border-color: #4FC3F7;
}

QSlider::groove:horizontal {
  height: 4px;
  background: #3A3A3A;
  border-radius: 2px;
}

QSlider::handle:horizontal {
  width: 14px;
  margin: -5px 0;
  background: #4FC3F7;
  border-radius: 7px;
}

QGraphicsView {
  background-color: #111111;
  border: none;
}
)");
}
} // namespace

namespace qvp
{

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
  setWindowTitle("Qt Viewer");
  resize(1000, 700);

  createViewer();
  createMenus();
  createToolBar();
  createVolumeControlsDock();
  createStatusBar();
  loadSettings();

  setStyleSheet(darkMedicalStyleSheet());
  updateActions();
}

void MainWindow::saveImageAs()
{
  const QImage image = viewer_->image();

  if (image.isNull())
  {
    QMessageBox::information(this, "No Image", "There is no image to save.");
    return;
  }

  QString fileName = QFileDialog::getSaveFileName(this, "Save Image As", "processed_image.png",
                                                  "Images (*.png *.jpg *.jpeg *.bmp)");

  if (fileName.isEmpty())
    return;

  if (!fileName.endsWith(".png", Qt::CaseInsensitive) &&
      !fileName.endsWith(".jpg", Qt::CaseInsensitive) &&
      !fileName.endsWith(".jpeg", Qt::CaseInsensitive) &&
      !fileName.endsWith(".bmp", Qt::CaseInsensitive))
  {
    fileName += ".png";
  }

  if (!image.save(fileName))
  {
    QMessageBox::warning(this, "Save Failed", "Could not save the image.");
  }
}

void MainWindow::openImage()
{
  const QString fileName = QFileDialog::getOpenFileName(this, "Open Image", QString(),
                                                        "Images (*.png *.jpg *.jpeg *.bmp)");

  if (fileName.isEmpty())
    return;

  openImage(fileName);
}

void MainWindow::openImage(const QString& fileName)
{
  ImageLoader loader;

  QImage image = loader.load(fileName);

  if (image.isNull())
  {
    QMessageBox::warning(this, "Error", "Failed to load image.");
    return;
  }

  originalImage_ = image;
  volumeActive_ = false;
  rawVolumeActive_ = false;
  if (cursorValueLabel_)
  {
    cursorValueLabel_->setText("-");
  }
  viewer_->setImage(image);
  updateStatusBar();
  updateActions();
  updateVolumeInfoLabels();

  addRecentFile(fileName);
}

void MainWindow::fitToWindow()
{
  viewer_->fitToWindow();
  updateStatusBar();
}

void MainWindow::actualSize()
{
  viewer_->actualSize();
  updateStatusBar();
}

void MainWindow::createViewer()
{
  viewer_ = new ImageViewer2D(this);
  setCentralWidget(viewer_);

  // Explicitly select openImage(const QString&) because openImage() is overloaded.
  connect(viewer_, &ImageViewer2D::imageDropped, this,
          static_cast<void (MainWindow::*)(const QString&)>(&MainWindow::openImage));
  connect(viewer_, &ImageViewer2D::imageMousePositionChanged, this,
          &MainWindow::updateMouseImagePosition);
}

void MainWindow::createMenus()
{
  createFileMenu();
  createViewMenu();
  createImageMenu();
  createDemoMenu();
  createHelpMenu();
}

void MainWindow::createFileMenu()
{
  QMenu* fileMenu = menuBar()->addMenu("&File");

  openAction_ = fileMenu->addAction("&Open Image...");
  openAction_->setShortcut(QKeySequence::Open);
  openAction_->setStatusTip("Open an image file");

  saveAsAction_ = fileMenu->addAction("Save Image &As...");
  saveAsAction_->setShortcut(QKeySequence::SaveAs);
  saveAsAction_->setStatusTip("Save the current image as a file");
  connect(saveAsAction_, &QAction::triggered, this, &MainWindow::saveImageAs);

  recentMenu_ = fileMenu->addMenu("Open &Recent");
  fileMenu->addSeparator();

  // Explicitly select openImage() because it is overloaded.
  connect(openAction_, &QAction::triggered, this,
          static_cast<void (MainWindow::*)()>(&MainWindow::openImage));
}

void MainWindow::createViewMenu()
{
  QMenu* viewMenu = menuBar()->addMenu("&View");

  zoomInAction_ = viewMenu->addAction("Zoom &In");
  zoomInAction_->setShortcut(QKeySequence::ZoomIn);
  zoomInAction_->setStatusTip("Zoom in");
  connect(zoomInAction_, &QAction::triggered, this, &MainWindow::zoomIn);

  zoomOutAction_ = viewMenu->addAction("Zoom &Out");
  zoomOutAction_->setShortcut(QKeySequence::ZoomOut);
  zoomOutAction_->setStatusTip("Zoom out");
  connect(zoomOutAction_, &QAction::triggered, this, &MainWindow::zoomOut);

  viewMenu->addSeparator();

  fitAction_ = viewMenu->addAction("Fit to &Window");
  fitAction_->setShortcut(QKeySequence("Ctrl+F"));
  fitAction_->setStatusTip("Fit image to window");
  connect(fitAction_, &QAction::triggered, this, &MainWindow::fitToWindow);

  actualSizeAction_ = viewMenu->addAction("&Actual Size");
  actualSizeAction_->setShortcut(QKeySequence("Ctrl+0"));
  actualSizeAction_->setStatusTip("Show image at actual size");
  connect(actualSizeAction_, &QAction::triggered, this, &MainWindow::actualSize);
}

void MainWindow::createImageMenu()
{
  QMenu* imageMenu = menuBar()->addMenu("&Image");

  rotateLeftAction_ = imageMenu->addAction("Rotate &Left");
  rotateLeftAction_->setShortcut(QKeySequence("Ctrl+L"));
  rotateLeftAction_->setStatusTip("Rotate image left");
  connect(rotateLeftAction_, &QAction::triggered, this, &MainWindow::rotateLeft);

  rotateRightAction_ = imageMenu->addAction("Rotate &Right");
  rotateRightAction_->setShortcut(QKeySequence("Ctrl+R"));
  rotateRightAction_->setStatusTip("Rotate image right");
  connect(rotateRightAction_, &QAction::triggered, this, &MainWindow::rotateRight);

  imageMenu->addSeparator();

  flipHorizontalAction_ = imageMenu->addAction("Flip &Horizontal");
  flipHorizontalAction_->setStatusTip("Flip image horizontally");
  connect(flipHorizontalAction_, &QAction::triggered, this, &MainWindow::flipHorizontal);

  flipVerticalAction_ = imageMenu->addAction("Flip &Vertical");
  flipVerticalAction_->setStatusTip("Flip image vertically");
  connect(flipVerticalAction_, &QAction::triggered, this, &MainWindow::flipVertical);

  imageMenu->addSeparator();

  grayscaleAction_ = imageMenu->addAction("&Grayscale");
  grayscaleAction_->setStatusTip("Convert image to grayscale");
  connect(grayscaleAction_, &QAction::triggered, this, &MainWindow::convertToGrayscale);

  imageMenu->addSeparator();

  resetImageAction_ = imageMenu->addAction("&Reset Image");
  resetImageAction_->setStatusTip("Reset image to the original version");
  connect(resetImageAction_, &QAction::triggered, this, &MainWindow::resetImage);
}

void MainWindow::createDemoMenu()
{
  QMenu* demoMenu = menuBar()->addMenu("&Demo");

  openOpenGLViewerDemoAction_ = demoMenu->addAction("Open OpenGL Viewer");
  openOpenGLViewerDemoAction_->setStatusTip("Open the OpenGL slice viewer demo");
  connect(openOpenGLViewerDemoAction_, &QAction::triggered, this,
          &MainWindow::openOpenGLViewerDemo);

  demoMenu->addSeparator();

  openSyntheticVolumeSliceAction_ = demoMenu->addAction("Open Synthetic Volume Slice");
  openSyntheticVolumeSliceAction_->setStatusTip("Display a synthetic axial volume slice");
  connect(openSyntheticVolumeSliceAction_, &QAction::triggered, this,
          &MainWindow::openSyntheticVolumeSlice);

  openRawVolumeAction_ = demoMenu->addAction("Open RAW Volume...");
  openRawVolumeAction_->setStatusTip("Open a RAW float32 volume from metadata and voxel files");
  connect(openRawVolumeAction_, &QAction::triggered, this, &MainWindow::openRawVolume);

  demoMenu->addSeparator();

  previousSliceAction_ = demoMenu->addAction("Previous Slice");
  previousSliceAction_->setStatusTip("Display the previous slice");
  previousSliceAction_->setShortcut(QKeySequence(Qt::Key_PageUp));
  connect(previousSliceAction_, &QAction::triggered, this, &MainWindow::previousSlice);

  nextSliceAction_ = demoMenu->addAction("Next Slice");
  nextSliceAction_->setStatusTip("Display the next slice");
  nextSliceAction_->setShortcut(QKeySequence(Qt::Key_PageDown));
  connect(nextSliceAction_, &QAction::triggered, this, &MainWindow::nextSlice);

  updateSliceActions();
}

void MainWindow::createHelpMenu()
{
  QMenu* helpMenu = menuBar()->addMenu("&Help");

  aboutAction_ = helpMenu->addAction("&About Qt Viewer");
  aboutAction_->setStatusTip("Show information about Qt Viewer");
  connect(aboutAction_, &QAction::triggered, this, &MainWindow::showAboutDialog);
}

void MainWindow::createStatusBar()
{
  updateStatusBar();
}

void MainWindow::updateStatusBar()
{
  const QSize imageSize = viewer_->imageSize();
  const int zoomPercent = static_cast<int>(viewer_->zoomFactor() * 100.0);

  statusBar()->showMessage(QString("Image: %1 × %2    Zoom: %3%")
                               .arg(imageSize.width())
                               .arg(imageSize.height())
                               .arg(zoomPercent));
}

void MainWindow::updateActions()
{
  const bool hasImage = !viewer_->image().isNull();

  saveAsAction_->setEnabled(hasImage);

  zoomInAction_->setEnabled(hasImage);
  zoomOutAction_->setEnabled(hasImage);
  fitAction_->setEnabled(hasImage);
  actualSizeAction_->setEnabled(hasImage);

  rotateLeftAction_->setEnabled(hasImage);
  rotateRightAction_->setEnabled(hasImage);
  flipHorizontalAction_->setEnabled(hasImage);
  flipVerticalAction_->setEnabled(hasImage);
  grayscaleAction_->setEnabled(hasImage);
  resetImageAction_->setEnabled(hasImage);

  updateSliceActions();
}

void MainWindow::zoomIn()
{
  viewer_->zoomIn();
  updateStatusBar();
}

void MainWindow::zoomOut()
{
  viewer_->zoomOut();
  updateStatusBar();
}

void MainWindow::addRecentFile(const QString& fileName)
{
  recentFiles_.removeAll(fileName);
  recentFiles_.prepend(fileName);

  while (recentFiles_.size() > kMaxRecentFiles)
    recentFiles_.removeLast();

  updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu()
{
  recentMenu_->clear();

  for (const QString& fileName : recentFiles_)
  {
    QAction* action = recentMenu_->addAction(fileName);
    action->setData(fileName);

    connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
  }

  if (!recentFiles_.isEmpty())
  {
    recentMenu_->addSeparator();

    QAction* clearAction = recentMenu_->addAction("Clear Recent Files");

    connect(clearAction, &QAction::triggered, this, &MainWindow::clearRecentFiles);
  }

  recentMenu_->setEnabled(!recentFiles_.isEmpty());
}

void MainWindow::openRecentFile()
{
  QAction* action = qobject_cast<QAction*>(sender());
  if (!action)
    return;
  const QString fileName = action->data().toString();
  openImage(fileName);
}

void MainWindow::loadSettings()
{
  QSettings settings;
  recentFiles_ = settings.value("recentFiles").toStringList();
  updateRecentFilesMenu();
}

void MainWindow::saveSettings()
{
  QSettings settings;
  settings.setValue("recentFiles", recentFiles_);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  saveSettings();
  QMainWindow::closeEvent(event);
}

void MainWindow::clearRecentFiles()
{
  recentFiles_.clear();
  updateRecentFilesMenu();
  saveSettings();
}

void MainWindow::openOpenGLViewerDemo()
{
  auto* demoWindow = new QWidget(nullptr);
  demoWindow->setAttribute(Qt::WA_DeleteOnClose);
  demoWindow->setWindowTitle("OpenGL Slice Viewer Demo");
  demoWindow->resize(640, 480);

  auto* layout = new QVBoxLayout(demoWindow);
  layout->setContentsMargins(4, 4, 4, 4);

  auto* buttonLayout = new QHBoxLayout();
  buttonLayout->setSpacing(4);
  auto* statusLayout = new QHBoxLayout();
  statusLayout->setSpacing(8);
  auto* openImageButton = new QPushButton("Open Image...", demoWindow);
  auto* loadSyntheticSliceButton = new QPushButton("Load Synthetic Slice", demoWindow);
  auto* loadRawSliceButton = new QPushButton("Load RAW Slice", demoWindow);
  auto* resetViewButton = new QPushButton("Reset View", demoWindow);
  auto* showCrosshairCheckBox = new QCheckBox("Show Crosshair", demoWindow);
  showCrosshairCheckBox->setChecked(true);
  auto* orientationComboBox = new QComboBox(demoWindow);
  orientationComboBox->addItems({"Axial", "Coronal", "Sagittal"});
  auto* windowSpinBox = new QSpinBox(demoWindow);
  windowSpinBox->setRange(1, 4096);
  windowSpinBox->setValue(255);
  auto* levelSpinBox = new QSpinBox(demoWindow);
  levelSpinBox->setRange(-2048, 4096);
  levelSpinBox->setValue(127);
  auto* resetWindowLevelButton = new QPushButton("Reset W/L", demoWindow);
  auto* windowLevelPresetComboBox = new QComboBox(demoWindow);
  windowLevelPresetComboBox->addItems({"Preset", "Soft Tissue", "Lung", "Bone", "Reset"});
  auto* previousSliceButton = new QPushButton("Z-", demoWindow);
  auto* sliceSlider = new QSlider(Qt::Horizontal, demoWindow);
  sliceSlider->setRange(0, 0);
  sliceSlider->setValue(0);
  auto* sliceIndexLabel = new QLabel("Slice: - / -", demoWindow);
  auto* crosshairPositionLabel = new QLabel("Crosshair: x=0.000 y=0.000", demoWindow);
  auto* nextSliceButton = new QPushButton("Z+", demoWindow);
  buttonLayout->addWidget(openImageButton);
  buttonLayout->addWidget(loadSyntheticSliceButton);
  buttonLayout->addWidget(loadRawSliceButton);
  buttonLayout->addWidget(new QLabel("Orientation", demoWindow));
  buttonLayout->addWidget(orientationComboBox);
  buttonLayout->addWidget(previousSliceButton);
  buttonLayout->addWidget(sliceSlider);
  buttonLayout->addWidget(nextSliceButton);
  buttonLayout->addWidget(new QLabel("Window", demoWindow));
  buttonLayout->addWidget(windowSpinBox);
  buttonLayout->addWidget(new QLabel("Level", demoWindow));
  buttonLayout->addWidget(levelSpinBox);
  buttonLayout->addWidget(resetWindowLevelButton);
  buttonLayout->addWidget(windowLevelPresetComboBox);
  buttonLayout->addWidget(resetViewButton);
  buttonLayout->addWidget(showCrosshairCheckBox);
  buttonLayout->addStretch();
  layout->addLayout(buttonLayout);

  auto* openGLViewer = new OpenGLSliceViewer(demoWindow);
  openGLViewer->setImage(createOpenGLDemoImage());
  connect(showCrosshairCheckBox, &QCheckBox::toggled, openGLViewer,
          &OpenGLSliceViewer::setCrosshairVisible);
  connect(openGLViewer,
          &OpenGLSliceViewer::crosshairPositionChanged,
          demoWindow,
          [crosshairPositionLabel](const QPointF position) {
            crosshairPositionLabel->setText(
                QString("Crosshair: x=%1 y=%2").arg(position.x(), 0, 'f', 3).arg(position.y(), 0, 'f', 3));
          });
  layout->addWidget(openGLViewer);
  layout->setStretchFactor(openGLViewer, 1);

  statusLayout->addWidget(sliceIndexLabel);
  statusLayout->addStretch();
  statusLayout->addWidget(crosshairPositionLabel);
  layout->addLayout(statusLayout);

  auto currentVolume = std::make_shared<std::optional<VolumeData>>();
  auto currentSliceIndex = std::make_shared<std::size_t>(0);
  auto currentOrientation = std::make_shared<SliceOrientation>(SliceOrientation::Axial);
  auto hasCurrentVolumeSlice = std::make_shared<bool>(false);

  auto currentSliceCount = [currentVolume, currentOrientation, hasCurrentVolumeSlice]() -> std::size_t {
    if (!*hasCurrentVolumeSlice || !currentVolume->has_value())
    {
      return 0;
    }

    switch (*currentOrientation)
    {
    case SliceOrientation::Coronal:
      return currentVolume->value().height();
    case SliceOrientation::Sagittal:
      return currentVolume->value().width();
    case SliceOrientation::Axial:
      return currentVolume->value().depth();
    }

    return currentVolume->value().depth();
  };

  auto updateOpenGLVolumeSlice = [demoWindow,
                                  openGLViewer,
                                  windowSpinBox,
                                  levelSpinBox,
                                  currentVolume,
                                  currentSliceIndex,
                                  currentOrientation,
                                  hasCurrentVolumeSlice]() {
    if (!*hasCurrentVolumeSlice || !currentVolume->has_value())
    {
      return;
    }
    try
    {
      const auto slice =
          SliceExtractor::extract(currentVolume->value(), *currentOrientation, *currentSliceIndex);
      const QImage image = SliceImageConverter::toGrayscaleImage(
          slice, static_cast<float>(windowSpinBox->value()), static_cast<float>(levelSpinBox->value()));
      openGLViewer->setSliceImage(image);
    }
    catch (const std::exception& error)
    {
      QMessageBox::warning(demoWindow, "Slice Update Error", error.what());
    }
  };

  auto configureSliceSlider = [sliceSlider, currentSliceIndex, currentSliceCount]() {
    const std::size_t sliceCount = currentSliceCount();
    if (sliceCount == 0)
    {
      return;
    }

    const QSignalBlocker blocker(sliceSlider);
    sliceSlider->setRange(0, static_cast<int>(sliceCount - 1));
    sliceSlider->setValue(static_cast<int>(*currentSliceIndex));
  };

  auto updateSliceLabel = [sliceIndexLabel, currentSliceIndex, currentSliceCount]() {
    const std::size_t sliceCount = currentSliceCount();
    if (sliceCount == 0)
    {
      sliceIndexLabel->setText("Slice: - / -");
      return;
    }

    sliceIndexLabel->setText(QString("Slice: %1 / %2")
                                 .arg(*currentSliceIndex + 1)
                                 .arg(sliceCount));
  };

  connect(openImageButton, &QPushButton::clicked, demoWindow, [demoWindow,
                                                               openGLViewer,
                                                               hasCurrentVolumeSlice,
                                                               updateSliceLabel]() {
    const QString fileName = QFileDialog::getOpenFileName(
        demoWindow, "Open Image", QString(), "Images (*.png *.jpg *.jpeg *.bmp)");

    if (fileName.isEmpty())
    {
      return;
    }

    const QImage image(fileName);
    if (image.isNull())
    {
      QMessageBox::warning(demoWindow, "Open Failed", "Could not load the selected image.");
      return;
    }

    openGLViewer->setImage(image);
    *hasCurrentVolumeSlice = false;
    updateSliceLabel();
  });
  connect(loadSyntheticSliceButton,
          &QPushButton::clicked,
          demoWindow,
          [this,
           openGLViewer,
           orientationComboBox,
           currentVolume,
           currentSliceIndex,
           currentOrientation,
           hasCurrentVolumeSlice,
           configureSliceSlider,
           updateSliceLabel,
           updateOpenGLVolumeSlice]() {
    *currentVolume = createSyntheticVolume();
    *currentOrientation = SliceOrientation::Axial;
    *currentSliceIndex = currentVolume->value().depth() / 2;
    *hasCurrentVolumeSlice = true;
    const QSignalBlocker blocker(orientationComboBox);
    orientationComboBox->setCurrentIndex(0);
    configureSliceSlider();
    updateSliceLabel();
    updateOpenGLVolumeSlice();
    openGLViewer->resetView();
  });
  connect(loadRawSliceButton,
          &QPushButton::clicked,
          demoWindow,
          [demoWindow,
           openGLViewer,
           orientationComboBox,
           currentVolume,
           currentSliceIndex,
           currentOrientation,
           hasCurrentVolumeSlice,
           configureSliceSlider,
           updateSliceLabel,
           updateOpenGLVolumeSlice]() {
    const QString metadataPath =
        QFileDialog::getOpenFileName(demoWindow, "Open RAW Volume Metadata", QString(),
                                     "JSON Metadata (*.json);;All Files (*)");
    if (metadataPath.isEmpty())
    {
      return;
    }

    const QString rawPath =
        QFileDialog::getOpenFileName(demoWindow, "Open RAW Volume Data", QString(),
                                     "RAW Volume Data (*.raw);;All Files (*)");
    if (rawPath.isEmpty())
    {
      return;
    }

    try
    {
      *currentVolume = RawVolumeLoader::load(metadataPath, rawPath);
      *currentOrientation = SliceOrientation::Axial;
      *currentSliceIndex = currentVolume->value().depth() / 2;
      *hasCurrentVolumeSlice = true;
      const QSignalBlocker blocker(orientationComboBox);
      orientationComboBox->setCurrentIndex(0);
      configureSliceSlider();
      updateSliceLabel();
      updateOpenGLVolumeSlice();
      openGLViewer->resetView();
    }
    catch (const std::exception& error)
    {
      QMessageBox::warning(demoWindow, "RAW Volume Load Error", error.what());
    }
  });
  connect(sliceSlider,
          &QSlider::valueChanged,
          demoWindow,
          [currentSliceIndex,
           hasCurrentVolumeSlice,
           updateSliceLabel,
           updateOpenGLVolumeSlice](int sliceIndex) {
    if (!*hasCurrentVolumeSlice)
    {
      return;
    }

    *currentSliceIndex = static_cast<std::size_t>(sliceIndex);
    updateSliceLabel();
    updateOpenGLVolumeSlice();
  });
  connect(orientationComboBox,
          qOverload<int>(&QComboBox::currentIndexChanged),
          demoWindow,
          [currentSliceIndex,
           currentOrientation,
           hasCurrentVolumeSlice,
           currentSliceCount,
           configureSliceSlider,
           updateSliceLabel,
           updateOpenGLVolumeSlice](int orientationIndex) {
    if (!*hasCurrentVolumeSlice)
    {
      return;
    }

    switch (orientationIndex)
    {
    case 1:
      *currentOrientation = SliceOrientation::Coronal;
      break;
    case 2:
      *currentOrientation = SliceOrientation::Sagittal;
      break;
    default:
      *currentOrientation = SliceOrientation::Axial;
      break;
    }

    const std::size_t sliceCount = currentSliceCount();
    if (sliceCount == 0)
    {
      updateSliceLabel();
      return;
    }

    *currentSliceIndex = sliceCount / 2;
    configureSliceSlider();
    updateSliceLabel();
    updateOpenGLVolumeSlice();
  });
  connect(previousSliceButton,
          &QPushButton::clicked,
          demoWindow,
          [sliceSlider,
           currentSliceIndex,
           hasCurrentVolumeSlice,
           currentSliceCount,
           updateSliceLabel,
           updateOpenGLVolumeSlice]() {
    if (!*hasCurrentVolumeSlice)
    {
      return;
    }

    if (currentSliceCount() == 0)
    {
      return;
    }

    if (*currentSliceIndex == 0)
    {
      return;
    }

    --(*currentSliceIndex);
    const QSignalBlocker blocker(sliceSlider);
    sliceSlider->setValue(static_cast<int>(*currentSliceIndex));
    updateSliceLabel();
    updateOpenGLVolumeSlice();
  });
  connect(nextSliceButton,
          &QPushButton::clicked,
          demoWindow,
          [sliceSlider,
           currentSliceIndex,
           hasCurrentVolumeSlice,
           currentSliceCount,
           updateSliceLabel,
           updateOpenGLVolumeSlice]() {
    if (!*hasCurrentVolumeSlice)
    {
      return;
    }

    const std::size_t sliceCount = currentSliceCount();
    if (sliceCount == 0)
    {
      return;
    }

    const std::size_t maxSliceIndex = sliceCount - 1;
    if (*currentSliceIndex >= maxSliceIndex)
    {
      return;
    }

    ++(*currentSliceIndex);
    const QSignalBlocker blocker(sliceSlider);
    sliceSlider->setValue(static_cast<int>(*currentSliceIndex));
    updateSliceLabel();
    updateOpenGLVolumeSlice();
  });
  connect(windowSpinBox, &QSpinBox::valueChanged, demoWindow, updateOpenGLVolumeSlice);
  connect(levelSpinBox, &QSpinBox::valueChanged, demoWindow, updateOpenGLVolumeSlice);
  connect(windowLevelPresetComboBox,
          &QComboBox::activated,
          demoWindow,
          [windowSpinBox, levelSpinBox, updateOpenGLVolumeSlice](int presetIndex) {
            int window = windowSpinBox->value();
            int level = levelSpinBox->value();

            switch (presetIndex)
            {
            case 1:
              window = 400;
              level = 40;
              break;
            case 2:
              window = 1500;
              level = -600;
              break;
            case 3:
              window = 2000;
              level = 300;
              break;
            case 4:
              window = 255;
              level = 127;
              break;
            default:
              return;
            }

            const QSignalBlocker windowBlocker(windowSpinBox);
            const QSignalBlocker levelBlocker(levelSpinBox);
            windowSpinBox->setValue(window);
            levelSpinBox->setValue(level);
            updateOpenGLVolumeSlice();
          });
  connect(resetWindowLevelButton, &QPushButton::clicked, demoWindow, [windowSpinBox,
                                                                      levelSpinBox,
                                                                      updateOpenGLVolumeSlice]() {
    const QSignalBlocker windowBlocker(windowSpinBox);
    const QSignalBlocker levelBlocker(levelSpinBox);
    windowSpinBox->setValue(255);
    levelSpinBox->setValue(127);
    updateOpenGLVolumeSlice();
  });
  connect(resetViewButton, &QPushButton::clicked, openGLViewer, &OpenGLSliceViewer::resetView);

  demoWindow->show();
}

void MainWindow::createToolBar()
{
  QToolBar* toolBar = addToolBar("Main Toolbar");
  toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  toolBar->setIconSize(QSize(24, 24));

  openAction_->setIcon(createTextIcon("O"));
  saveAsAction_->setIcon(createTextIcon("S"));

  zoomInAction_->setIcon(createTextIcon("+"));
  zoomOutAction_->setIcon(createTextIcon("-"));

  fitAction_->setIcon(createTextIcon("F"));
  actualSizeAction_->setIcon(createTextIcon("1:1"));

  rotateLeftAction_->setIcon(createTextIcon("↺"));
  rotateRightAction_->setIcon(createTextIcon("↻"));

  flipHorizontalAction_->setIcon(createTextIcon("H"));
  flipVerticalAction_->setIcon(createTextIcon("V"));

  grayscaleAction_->setIcon(createTextIcon("G"));
  resetImageAction_->setIcon(createTextIcon("R"));

  previousSliceAction_->setIcon(createTextIcon("Z-"));
  nextSliceAction_->setIcon(createTextIcon("Z+"));

  openAction_->setToolTip("Open Image");
  openAction_->setStatusTip("Open Image");
  saveAsAction_->setToolTip("Save Image As");
  saveAsAction_->setStatusTip("Save Image As");

  zoomInAction_->setToolTip("Zoom In");
  zoomInAction_->setStatusTip("Zoom In");
  zoomOutAction_->setToolTip("Zoom Out");
  zoomOutAction_->setStatusTip("Zoom Out");

  fitAction_->setToolTip("Fit to Window");
  fitAction_->setStatusTip("Fit to Window");
  actualSizeAction_->setToolTip("Actual Size");
  actualSizeAction_->setStatusTip("Actual Size");

  rotateLeftAction_->setToolTip("Rotate Left");
  rotateLeftAction_->setStatusTip("Rotate Left");
  rotateRightAction_->setToolTip("Rotate Right");
  rotateRightAction_->setStatusTip("Rotate Right");

  flipHorizontalAction_->setToolTip("Flip Horizontal");
  flipHorizontalAction_->setStatusTip("Flip Horizontal");
  flipVerticalAction_->setToolTip("Flip Vertical");
  flipVerticalAction_->setStatusTip("Flip Vertical");

  grayscaleAction_->setToolTip("Convert to Grayscale");
  grayscaleAction_->setStatusTip("Convert to Grayscale");
  resetImageAction_->setToolTip("Reset Image");
  resetImageAction_->setStatusTip("Reset Image");

  previousSliceAction_->setToolTip("Previous Slice");
  previousSliceAction_->setStatusTip("Previous Slice");
  nextSliceAction_->setToolTip("Next Slice");
  nextSliceAction_->setStatusTip("Next Slice");

  toolBar->addAction(openAction_);
  toolBar->addAction(saveAsAction_);
  toolBar->addSeparator();

  toolBar->addAction(zoomInAction_);
  toolBar->addAction(zoomOutAction_);
  toolBar->addSeparator();

  toolBar->addAction(fitAction_);
  toolBar->addAction(actualSizeAction_);
  toolBar->addSeparator();

  toolBar->addAction(rotateLeftAction_);
  toolBar->addAction(rotateRightAction_);
  toolBar->addSeparator();

  toolBar->addAction(flipHorizontalAction_);
  toolBar->addAction(flipVerticalAction_);
  toolBar->addSeparator();

  toolBar->addAction(grayscaleAction_);
  toolBar->addAction(resetImageAction_);
  toolBar->addSeparator();

  toolBar->addAction(previousSliceAction_);
  toolBar->addAction(nextSliceAction_);
}

void MainWindow::createVolumeControlsDock()
{
  auto* dock = new QDockWidget("Volume Controls", this);
  dock->setFeatures(QDockWidget::NoDockWidgetFeatures);

  auto* panel = new QWidget(dock);
  panel->setMinimumWidth(220);
  auto* layout = new QFormLayout(panel);

  sliceOrientationComboBox_ = new QComboBox(panel);
  sliceOrientationComboBox_->addItem("Axial");
  sliceOrientationComboBox_->addItem("Coronal");
  sliceOrientationComboBox_->addItem("Sagittal");
  sliceOrientationComboBox_->setCurrentIndex(0);
  sliceOrientationComboBox_->setEnabled(false);
  sliceOrientationComboBox_->setToolTip("Slice Orientation");
  sliceOrientationComboBox_->setStatusTip("Slice Orientation");
  connect(sliceOrientationComboBox_,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &MainWindow::setSliceOrientation);

  sliceSlider_ = new QSlider(Qt::Horizontal, panel);
  sliceSlider_->setEnabled(false);
  sliceSlider_->setToolTip("Slice");
  sliceSlider_->setStatusTip("Slice");
  connect(sliceSlider_, &QSlider::valueChanged, this, &MainWindow::setSliceFromSlider);

  sliceSpinBox_ = new QSpinBox(panel);
  sliceSpinBox_->setRange(1, 1);
  sliceSpinBox_->setEnabled(false);
  sliceSpinBox_->setToolTip("Slice Index");
  sliceSpinBox_->setStatusTip("Slice Index");
  connect(sliceSpinBox_, &QSpinBox::valueChanged, this, &MainWindow::setSliceFromSpinBox);

  modeValueLabel_ = new QLabel("-", panel);
  sizeValueLabel_ = new QLabel("-", panel);
  spacingValueLabel_ = new QLabel("-", panel);
  currentSliceValueLabel_ = new QLabel("-", panel);
  cursorValueLabel_ = new QLabel("-", panel);
  cursorValueLabel_->setMinimumWidth(140);
  cursorValueLabel_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

  windowSpinBox_ = new QSpinBox(panel);
  windowSpinBox_->setRange(1, 4096);
  windowSpinBox_->setValue(255);
  windowSpinBox_->setEnabled(false);
  windowSpinBox_->setToolTip("Window");
  windowSpinBox_->setStatusTip("Window");
  connect(windowSpinBox_, &QSpinBox::valueChanged, this, &MainWindow::updateWindowLevel);

  levelSpinBox_ = new QSpinBox(panel);
  levelSpinBox_->setRange(-2048, 4096);
  levelSpinBox_->setValue(127);
  levelSpinBox_->setEnabled(false);
  levelSpinBox_->setToolTip("Level");
  levelSpinBox_->setStatusTip("Level");
  connect(levelSpinBox_, &QSpinBox::valueChanged, this, &MainWindow::updateWindowLevel);

  resetWindowLevelButton_ = new QPushButton("Reset W/L", panel);
  resetWindowLevelButton_->setEnabled(false);
  connect(resetWindowLevelButton_, &QPushButton::clicked, this, &MainWindow::resetWindowLevel);

  invertGrayscaleCheckBox_ = new QCheckBox("Invert grayscale", panel);
  invertGrayscaleCheckBox_->setEnabled(false);
  connect(invertGrayscaleCheckBox_, &QCheckBox::toggled, this,
          &MainWindow::updateInvertGrayscale);

  auto* navigationHeader = new QLabel("Navigation", panel);
  QFont navigationHeaderFont = navigationHeader->font();
  navigationHeaderFont.setBold(true);
  navigationHeader->setFont(navigationHeaderFont);

  auto* infoHeader = new QLabel("Info", panel);
  QFont infoHeaderFont = infoHeader->font();
  infoHeaderFont.setBold(true);
  infoHeader->setFont(infoHeaderFont);

  auto* displayHeader = new QLabel("Display", panel);
  QFont displayHeaderFont = displayHeader->font();
  displayHeaderFont.setBold(true);
  displayHeader->setFont(displayHeaderFont);

  layout->addRow(navigationHeader);
  layout->addRow("Orientation:", sliceOrientationComboBox_);
  layout->addRow("Slice:", sliceSlider_);
  layout->addRow("Slice Index:", sliceSpinBox_);
  layout->addRow(infoHeader);
  layout->addRow("Mode:", modeValueLabel_);
  layout->addRow("Size:", sizeValueLabel_);
  layout->addRow("Spacing:", spacingValueLabel_);
  layout->addRow("Current:", currentSliceValueLabel_);
  layout->addRow("Cursor:", cursorValueLabel_);
  layout->addRow(displayHeader);
  layout->addRow("Window:", windowSpinBox_);
  layout->addRow("Level:", levelSpinBox_);
  layout->addRow(resetWindowLevelButton_);
  layout->addRow(invertGrayscaleCheckBox_);

  dock->setWidget(panel);
  dock->setMinimumWidth(230);
  addDockWidget(Qt::RightDockWidgetArea, dock);
  updateVolumeInfoLabels();
}

void MainWindow::rotateLeft()
{
  viewer_->rotateLeft();
  updateStatusBar();
}

void MainWindow::rotateRight()
{
  viewer_->rotateRight();
  updateStatusBar();
}

void MainWindow::flipHorizontal()
{
  viewer_->flipHorizontal();
  updateStatusBar();
}

void MainWindow::flipVertical()
{
  viewer_->flipVertical();
  updateStatusBar();
}

void MainWindow::convertToGrayscale()
{
  ImageProcessor processor;

  const QImage grayscaleImage = processor.toGrayscale(viewer_->image());

  if (grayscaleImage.isNull())
    return;

  viewer_->setImage(grayscaleImage);
  updateStatusBar();
  updateActions();
}

void MainWindow::resetImage()
{
  if (originalImage_.isNull())
    return;

  viewer_->setImage(originalImage_);
  updateStatusBar();
  updateActions();
}

VolumeData MainWindow::createSyntheticVolume() const
{
  constexpr std::size_t width = 128;
  constexpr std::size_t height = 128;
  constexpr std::size_t depth = 32;
  constexpr float spacing = 1.0F;

  std::vector<float> voxels;
  voxels.reserve(width * height * depth);

  const float centerX = static_cast<float>(width - 1) / 2.0F;
  const float centerY = static_cast<float>(height - 1) / 2.0F;
  const float centerZ = static_cast<float>(depth - 1) / 2.0F;
  constexpr float sphereRadius = 1.02F;
  constexpr float brightValue = 240.0F;
  constexpr float backgroundValue = 35.0F;

  for (std::size_t z = 0; z < depth; ++z)
  {
    const float dz = (static_cast<float>(z) - centerZ) / centerZ;

    for (std::size_t y = 0; y < height; ++y)
    {
      const float dy = (static_cast<float>(y) - centerY) / centerY;

      for (std::size_t x = 0; x < width; ++x)
      {
        const float dx = (static_cast<float>(x) - centerX) / centerX;
        const float distance = std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
        voxels.push_back(distance <= sphereRadius ? brightValue : backgroundValue);
      }
    }
  }

  return VolumeData(width, height, depth, spacing, spacing, spacing, std::move(voxels));
}

void MainWindow::openSyntheticVolumeSlice()
{
  activeVolume_ = createSyntheticVolume();
  activeSliceIndex_ = activeSliceCount() / 2;
  volumeActive_ = true;
  rawVolumeActive_ = false;
  if (cursorValueLabel_)
  {
    cursorValueLabel_->setText("-");
  }

  updateVolumeInfoLabels();
  displayCurrentSlice();
}

void MainWindow::openRawVolume()
{
  const QString metadataPath =
      QFileDialog::getOpenFileName(this, "Open RAW Volume Metadata", QString(),
                                   "JSON Metadata (*.json);;All Files (*)");
  if (metadataPath.isEmpty())
  {
    return;
  }

  const QString rawPath = QFileDialog::getOpenFileName(this, "Open RAW Volume Data", QString(),
                                                       "RAW Volume Data (*.raw);;All Files (*)");
  if (rawPath.isEmpty())
  {
    return;
  }

  try
  {
    activeVolume_ = RawVolumeLoader::load(metadataPath, rawPath);
    activeSliceIndex_ = activeSliceCount() / 2;
    volumeActive_ = true;
    rawVolumeActive_ = true;
    if (cursorValueLabel_)
    {
      cursorValueLabel_->setText("-");
    }

    updateVolumeInfoLabels();
    displayCurrentSlice();
  }
  catch (const std::exception& error)
  {
    QMessageBox::critical(this, "RAW Volume Load Error", error.what());
  }
}

void MainWindow::previousSlice()
{
  if (!volumeActive_ || activeSliceIndex_ == 0)
  {
    return;
  }

  --activeSliceIndex_;
  displayCurrentSlice();
}

void MainWindow::nextSlice()
{
  if (!volumeActive_ || activeSliceIndex_ + 1 >= activeSliceCount())
  {
    return;
  }

  ++activeSliceIndex_;
  displayCurrentSlice();
}

void MainWindow::setSliceFromSlider(int sliceIndex)
{
  if (!volumeActive_ || sliceIndex < 0 ||
      static_cast<std::size_t>(sliceIndex) >= activeSliceCount())
  {
    return;
  }

  activeSliceIndex_ = static_cast<std::size_t>(sliceIndex);
  displayCurrentSlice();
}

void MainWindow::setSliceFromSpinBox(int sliceNumber)
{
  if (!volumeActive_ || sliceNumber <= 0 ||
      static_cast<std::size_t>(sliceNumber) > activeSliceCount())
  {
    return;
  }

  activeSliceIndex_ = static_cast<std::size_t>(sliceNumber - 1);
  displayCurrentSlice();
}

void MainWindow::setSliceOrientation(int orientationIndex)
{
  switch (orientationIndex)
  {
  case 1:
    activeSliceOrientation_ = SliceOrientation::Coronal;
    break;
  case 2:
    activeSliceOrientation_ = SliceOrientation::Sagittal;
    break;
  default:
    activeSliceOrientation_ = SliceOrientation::Axial;
    break;
  }

  if (!volumeActive_)
  {
    updateVolumeInfoLabels();
    return;
  }

  activeSliceIndex_ = activeSliceCount() / 2;
  updateVolumeInfoLabels();
  displayCurrentSlice();
}

void MainWindow::updateWindowLevel()
{
  if (!volumeActive_)
  {
    return;
  }

  displayCurrentSlice();
}

void MainWindow::resetWindowLevel()
{
  if (windowSpinBox_)
  {
    const QSignalBlocker blocker(windowSpinBox_);
    windowSpinBox_->setValue(255);
  }
  if (levelSpinBox_)
  {
    const QSignalBlocker blocker(levelSpinBox_);
    levelSpinBox_->setValue(127);
  }

  if (volumeActive_)
  {
    displayCurrentSlice();
  }
}

void MainWindow::updateInvertGrayscale()
{
  if (!volumeActive_)
  {
    return;
  }

  displayCurrentSlice();
}

void MainWindow::updateMouseImagePosition(int x, int y)
{
  int voxelX = 0;
  int voxelY = 0;
  int voxelZ = 0;
  if (!voxelCoordinatesFromImagePosition(x, y, voxelX, voxelY, voxelZ))
  {
    if (cursorValueLabel_)
    {
      cursorValueLabel_->setText(volumeActive_ ? "-" : QString("x=%1 y=%2").arg(x).arg(y));
    }
    statusBar()->showMessage(QString("Mouse x=%1 y=%2").arg(x).arg(y));
    return;
  }

  if (cursorValueLabel_)
  {
    cursorValueLabel_->setText(QString("x=%1 y=%2 v=%3")
                                   .arg(x)
                                   .arg(y)
                                   .arg(voxelValueAt(voxelX, voxelY, voxelZ)));
  }
  statusBar()->showMessage(QString("%1 %2 slice %3/%4 | Mouse x=%5 y=%6 | Voxel value=%7")
                               .arg(currentModeText())
                               .arg(sliceOrientationName(activeSliceOrientation_))
                               .arg(activeSliceIndex_ + 1)
                               .arg(activeSliceCount())
                               .arg(x)
                               .arg(y)
                               .arg(voxelValueAt(voxelX, voxelY, voxelZ)));
}

bool MainWindow::voxelCoordinatesFromImagePosition(int imageX,
                                                   int imageY,
                                                   int& voxelX,
                                                   int& voxelY,
                                                   int& voxelZ) const
{
  if (!volumeActive_ || imageX < 0 || imageY < 0)
  {
    return false;
  }

  switch (activeSliceOrientation_)
  {
  case SliceOrientation::Coronal:
    voxelX = imageX;
    voxelY = static_cast<int>(activeSliceIndex_);
    voxelZ = imageY;
    break;
  case SliceOrientation::Sagittal:
    voxelX = static_cast<int>(activeSliceIndex_);
    voxelY = imageX;
    voxelZ = imageY;
    break;
  case SliceOrientation::Axial:
    voxelX = imageX;
    voxelY = imageY;
    voxelZ = static_cast<int>(activeSliceIndex_);
    break;
  }

  return voxelX >= 0 && voxelY >= 0 && voxelZ >= 0 &&
         static_cast<std::size_t>(voxelX) < activeVolume_.width() &&
         static_cast<std::size_t>(voxelY) < activeVolume_.height() &&
         static_cast<std::size_t>(voxelZ) < activeVolume_.depth();
}

float MainWindow::voxelValueAt(int voxelX, int voxelY, int voxelZ) const
{
  const std::size_t x = static_cast<std::size_t>(voxelX);
  const std::size_t y = static_cast<std::size_t>(voxelY);
  const std::size_t z = static_cast<std::size_t>(voxelZ);
  const std::size_t index =
      (z * activeVolume_.height() * activeVolume_.width()) + (y * activeVolume_.width()) + x;

  return activeVolume_.voxels().at(index);
}

void MainWindow::displayCurrentSlice()
{
  const auto slice =
      SliceExtractor::extract(activeVolume_, activeSliceOrientation_, activeSliceIndex_);
  const float window = windowSpinBox_ ? static_cast<float>(windowSpinBox_->value()) : 255.0F;
  const float level = levelSpinBox_ ? static_cast<float>(levelSpinBox_->value()) : 127.0F;
  QImage image = SliceImageConverter::toGrayscaleImage(slice, window, level);
  if (invertGrayscaleCheckBox_ && invertGrayscaleCheckBox_->isChecked())
  {
    image.invertPixels();
  }

  originalImage_ = image;
  viewer_->setImage(image);
  if (sliceSlider_)
  {
    const QSignalBlocker blocker(sliceSlider_);
    sliceSlider_->setRange(0, static_cast<int>(activeSliceCount() - 1));
    sliceSlider_->setValue(static_cast<int>(activeSliceIndex_));
  }
  if (sliceSpinBox_)
  {
    const QSignalBlocker blocker(sliceSpinBox_);
    sliceSpinBox_->setRange(1, static_cast<int>(activeSliceCount()));
    sliceSpinBox_->setValue(static_cast<int>(activeSliceIndex_ + 1));
  }
  updateVolumeInfoLabels();
  updateActions();
  const QString sliceLabel = rawVolumeActive_ ? "RAW volume" : "Synthetic";
  statusBar()->showMessage(QString("%1 %2 slice %3/%4")
                               .arg(sliceLabel)
                               .arg(sliceOrientationName(activeSliceOrientation_))
                               .arg(activeSliceIndex_ + 1)
                               .arg(activeSliceCount()));
}

std::size_t MainWindow::activeSliceCount() const
{
  switch (activeSliceOrientation_)
  {
  case SliceOrientation::Coronal:
    return activeVolume_.height();
  case SliceOrientation::Sagittal:
    return activeVolume_.width();
  case SliceOrientation::Axial:
    return activeVolume_.depth();
  }

  return activeVolume_.depth();
}

void MainWindow::updateSliceActions()
{
  if (!previousSliceAction_ || !nextSliceAction_)
  {
    return;
  }

  const bool hasVolume = volumeActive_ && activeSliceCount() > 0;

  previousSliceAction_->setEnabled(hasVolume && activeSliceIndex_ > 0);
  nextSliceAction_->setEnabled(hasVolume && activeSliceIndex_ + 1 < activeSliceCount());
  if (sliceOrientationComboBox_)
  {
    sliceOrientationComboBox_->setEnabled(hasVolume);
  }
  if (sliceSlider_)
  {
    sliceSlider_->setEnabled(hasVolume);
  }
  if (sliceSpinBox_)
  {
    sliceSpinBox_->setEnabled(hasVolume);
  }
  if (windowSpinBox_)
  {
    windowSpinBox_->setEnabled(hasVolume);
  }
  if (levelSpinBox_)
  {
    levelSpinBox_->setEnabled(hasVolume);
  }
  if (resetWindowLevelButton_)
  {
    resetWindowLevelButton_->setEnabled(hasVolume);
  }
  if (invertGrayscaleCheckBox_)
  {
    invertGrayscaleCheckBox_->setEnabled(hasVolume);
  }
}

void MainWindow::updateVolumeInfoLabels()
{
  if (!modeValueLabel_ || !sizeValueLabel_ || !spacingValueLabel_ || !currentSliceValueLabel_)
  {
    return;
  }

  modeValueLabel_->setText(currentModeText());
  sizeValueLabel_->setText(currentSizeText());
  spacingValueLabel_->setText(currentSpacingText());
  currentSliceValueLabel_->setText(currentSliceText());
}

QString MainWindow::currentModeText() const
{
  if (!volumeActive_)
  {
    return originalImage_.isNull() ? "None" : "Image";
  }

  return rawVolumeActive_ ? "RAW" : "Synthetic";
}

QString MainWindow::currentSizeText() const
{
  if (volumeActive_)
  {
    return QString("%1 × %2 × %3")
        .arg(activeVolume_.width())
        .arg(activeVolume_.height())
        .arg(activeVolume_.depth());
  }

  if (!originalImage_.isNull())
  {
    return QString("%1 × %2").arg(originalImage_.width()).arg(originalImage_.height());
  }

  return "-";
}

QString MainWindow::currentSpacingText() const
{
  if (!volumeActive_)
  {
    return "-";
  }

  return QString("%1 × %2 × %3")
      .arg(activeVolume_.spacingX())
      .arg(activeVolume_.spacingY())
      .arg(activeVolume_.spacingZ());
}

QString MainWindow::currentSliceText() const
{
  if (!volumeActive_)
  {
    return "-";
  }

  return QString("%1 slice %2/%3")
      .arg(sliceOrientationName(activeSliceOrientation_))
      .arg(activeSliceIndex_ + 1)
      .arg(activeSliceCount());
}

void MainWindow::showAboutDialog()
{
  QMessageBox::about(
      this, "About Qt Viewer",
      "Qt Viewer\n\n"
      "A lightweight desktop image viewer built with C++, Qt Widgets, and OpenCV.\n\n"
      "Features:\n"
      "- Open images\n"
      "- Drag and drop\n"
      "- Zoom, fit, and actual size\n"
      "- Rotate and flip\n"
      "- Grayscale processing\n"
      "- Reset image\n"
      "- Save processed image\n\n"
      "Version: 0.1");
}

} // namespace qvp
