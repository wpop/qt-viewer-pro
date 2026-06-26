#include "qtviewerpro/ui/MainWindow.h"
#include "qtviewerpro/core/SliceExtractor.h"
#include "qtviewerpro/core/SliceOrientation.h"
#include "qtviewerpro/core/VolumeData.h"
#include "qtviewerpro/io/ImageLoader.h"
#include "qtviewerpro/io/RawVolumeLoader.h"
#include "qtviewerpro/processing/ImageProcessor.h"
#include "qtviewerpro/processing/SliceImageConverter.h"
#include "qtviewerpro/ui/ImageViewer2D.h"

#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QImage>
#include <QLabel>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QWidget>

#include <QSize>
#include <QStyle>

#include <QIcon>
#include <QPainter>
#include <QPixmap>

#include <cmath>
#include <exception>
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
  auto* layout = new QFormLayout(panel);

  sliceOrientationComboBox_ = new QComboBox(panel);
  sliceOrientationComboBox_->addItem("Axial");
  sliceOrientationComboBox_->addItem("Coronal");
  sliceOrientationComboBox_->addItem("Sagittal");
  sliceOrientationComboBox_->setCurrentIndex(0);
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

  modeValueLabel_ = new QLabel("-", panel);
  sizeValueLabel_ = new QLabel("-", panel);
  spacingValueLabel_ = new QLabel("-", panel);
  currentSliceValueLabel_ = new QLabel("-", panel);

  windowSpinBox_ = new QSpinBox(panel);
  windowSpinBox_->setRange(1, 4096);
  windowSpinBox_->setValue(255);
  windowSpinBox_->setToolTip("Window");
  windowSpinBox_->setStatusTip("Window");
  connect(windowSpinBox_, &QSpinBox::valueChanged, this, &MainWindow::updateWindowLevel);

  levelSpinBox_ = new QSpinBox(panel);
  levelSpinBox_->setRange(-2048, 4096);
  levelSpinBox_->setValue(127);
  levelSpinBox_->setToolTip("Level");
  levelSpinBox_->setStatusTip("Level");
  connect(levelSpinBox_, &QSpinBox::valueChanged, this, &MainWindow::updateWindowLevel);

  layout->addRow("Orientation:", sliceOrientationComboBox_);
  layout->addRow("Slice:", sliceSlider_);
  layout->addRow("Mode:", modeValueLabel_);
  layout->addRow("Size:", sizeValueLabel_);
  layout->addRow("Spacing:", spacingValueLabel_);
  layout->addRow("Current:", currentSliceValueLabel_);
  layout->addRow("Window:", windowSpinBox_);
  layout->addRow("Level:", levelSpinBox_);

  dock->setWidget(panel);
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

void MainWindow::displayCurrentSlice()
{
  const auto slice =
      SliceExtractor::extract(activeVolume_, activeSliceOrientation_, activeSliceIndex_);
  const float window = windowSpinBox_ ? static_cast<float>(windowSpinBox_->value()) : 255.0F;
  const float level = levelSpinBox_ ? static_cast<float>(levelSpinBox_->value()) : 127.0F;
  const QImage image = SliceImageConverter::toGrayscaleImage(slice, window, level);

  originalImage_ = image;
  viewer_->setImage(image);
  if (sliceSlider_)
  {
    const QSignalBlocker blocker(sliceSlider_);
    sliceSlider_->setRange(0, static_cast<int>(activeSliceCount() - 1));
    sliceSlider_->setValue(static_cast<int>(activeSliceIndex_));
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
  if (sliceSlider_)
  {
    sliceSlider_->setEnabled(hasVolume);
  }
}

void MainWindow::updateVolumeInfoLabels()
{
  if (!modeValueLabel_ || !sizeValueLabel_ || !spacingValueLabel_ || !currentSliceValueLabel_)
  {
    return;
  }

  if (!volumeActive_)
  {
    modeValueLabel_->setText("None");
    sizeValueLabel_->setText("-");
    spacingValueLabel_->setText("-");
    currentSliceValueLabel_->setText("-");
    return;
  }

  modeValueLabel_->setText(rawVolumeActive_ ? "RAW" : "Synthetic");
  sizeValueLabel_->setText(QString("%1 × %2 × %3")
                               .arg(activeVolume_.width())
                               .arg(activeVolume_.height())
                               .arg(activeVolume_.depth()));
  spacingValueLabel_->setText(QString("%1 × %2 × %3")
                                  .arg(activeVolume_.spacingX())
                                  .arg(activeVolume_.spacingY())
                                  .arg(activeVolume_.spacingZ()));
  currentSliceValueLabel_->setText(QString("%1 slice %2/%3")
                                       .arg(sliceOrientationName(activeSliceOrientation_))
                                       .arg(activeSliceIndex_ + 1)
                                       .arg(activeSliceCount()));
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
