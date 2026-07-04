#include "qtviewerpro/ui/MainWindow.h"
#include "qtviewerpro/core/VolumeData.h"
#include "qtviewerpro/io/DicomVolumeLoader.h"
#include "qtviewerpro/io/ImageLoader.h"
#include "qtviewerpro/io/MedicalVolumeLoaderRegistry.h"
#include "qtviewerpro/io/RawVolumeLoader.h"
#include "qtviewerpro/processing/ImageProcessor.h"
#include "qtviewerpro/ui/ImageViewer2D.h"
#include "qtviewerpro/ui/OpenGLVolumeViewerWidget.h"
#include "qtviewerpro/ui/Volume3DViewerWidget.h"

#include <QAction>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QStackedWidget>
#include <QToolBar>
#include <QWidget>

#include <QSize>
#include <QStyle>

#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QPainter>
#include <QPixmap>

#include <cmath>
#include <exception>
#include <limits>
#include <memory>
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

QString validateRawMetadataFile(const QString& metadataPath)
{
  QFile metadataFile(metadataPath);
  if (!metadataFile.open(QIODevice::ReadOnly))
  {
    return QStringLiteral("Unable to open metadata file");
  }

  QJsonParseError parseError;
  const QJsonDocument metadataDocument =
      QJsonDocument::fromJson(metadataFile.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError)
  {
    return QStringLiteral("Metadata JSON could not be parsed: %1").arg(parseError.errorString());
  }

  if (!metadataDocument.isObject())
  {
    return QStringLiteral("Metadata JSON must be a valid object");
  }

  const QJsonObject metadata = metadataDocument.object();

  auto requiredValueError = [](const QJsonObject& object, const QString& fieldName) -> QString {
    if (!object.contains(fieldName))
    {
      return QStringLiteral("Missing metadata field: %1").arg(fieldName);
    }

    return QString();
  };

  auto positiveDimensionError = [&](const QString& fieldName) -> QString {
    const QString requiredError = requiredValueError(metadata, fieldName);
    if (!requiredError.isEmpty())
    {
      return requiredError;
    }

    const QJsonValue value = metadata.value(fieldName);
    if (!value.isDouble())
    {
      return QStringLiteral("Metadata field must be numeric: %1").arg(fieldName);
    }

    const double number = value.toDouble();
    if (!std::isfinite(number) || number <= 0.0 || std::floor(number) != number)
    {
      return QStringLiteral("Metadata dimension must be a positive integer: %1").arg(fieldName);
    }

    if (number > static_cast<double>(std::numeric_limits<std::size_t>::max()))
    {
      return QStringLiteral("Metadata dimension is too large: %1").arg(fieldName);
    }

    return QString();
  };

  auto positiveSpacingError = [&](const QString& fieldName) -> QString {
    const QString requiredError = requiredValueError(metadata, fieldName);
    if (!requiredError.isEmpty())
    {
      return requiredError;
    }

    const QJsonValue value = metadata.value(fieldName);
    if (!value.isDouble())
    {
      return QStringLiteral("Metadata field must be numeric: %1").arg(fieldName);
    }

    const double number = value.toDouble();
    if (!std::isfinite(number) || number <= 0.0 ||
        number > static_cast<double>(std::numeric_limits<float>::max()))
    {
      return QStringLiteral("Metadata spacing must be positive: %1").arg(fieldName);
    }

    return QString();
  };

  for (const QString& fieldName :
       {QStringLiteral("width"), QStringLiteral("height"), QStringLiteral("depth")})
  {
    const QString error = positiveDimensionError(fieldName);
    if (!error.isEmpty())
    {
      return error;
    }
  }

  for (const QString& fieldName :
       {QStringLiteral("spacingX"), QStringLiteral("spacingY"), QStringLiteral("spacingZ")})
  {
    const QString error = positiveSpacingError(fieldName);
    if (!error.isEmpty())
    {
      return error;
    }
  }

  return QString();
}

void showRawVolumeLoadError(QWidget* parent, const QString& details)
{
  QMessageBox messageBox(parent);
  messageBox.setIcon(QMessageBox::Warning);
  messageBox.setWindowTitle("RAW Volume Load Error");
  messageBox.setText("RAW test volumes require JSON metadata plus a separate float32 .raw file.");
  messageBox.setInformativeText(
      QStringLiteral("MetaImage/LUNA16 .raw files should be opened via the .mhd file using "
                     "File -> Open Medical Volume...\n\n"
                     "Details: %1")
          .arg(details));
  messageBox.setStyleSheet("QMessageBox { background-color: palette(window); }"
                           "QLabel { color: palette(text); }");
  messageBox.exec();
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
  showImagePage();
  viewer_->setImage(image);
  updateStatusBar();
  updateActions();

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
  pageStack_ = new QStackedWidget(this);
  viewer_ = new ImageViewer2D(pageStack_);
  medicalVolumeViewerWidget_ = new OpenGLVolumeViewerWidget(pageStack_);
  volume3DViewerWidget_ = new Volume3DViewerWidget(pageStack_);
  pageStack_->addWidget(viewer_);
  pageStack_->addWidget(medicalVolumeViewerWidget_);
  pageStack_->addWidget(volume3DViewerWidget_);
  setCentralWidget(pageStack_);
  showImagePage();

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

  fileMenu->addSeparator();

  openMedicalVolumeAction_ = fileMenu->addAction("Open Medical Volume...");
  openMedicalVolumeAction_->setStatusTip("Open a medical volume in the OpenGL slice viewer");
  connect(openMedicalVolumeAction_, &QAction::triggered, this, &MainWindow::openMedicalVolume);

  QAction* openDicomSeriesFolderAction = fileMenu->addAction("Open DICOM Series Folder...");
  openDicomSeriesFolderAction->setStatusTip("Open a DICOM series by selecting its folder");
  connect(openDicomSeriesFolderAction, &QAction::triggered, this, &MainWindow::openDicomSeriesFolder);

  openMaskOverlayAction_ = fileMenu->addAction("Open Mask Overlay...");
  openMaskOverlayAction_->setStatusTip("Open a mask overlay for the current medical volume");
  connect(openMaskOverlayAction_, &QAction::triggered, this, &MainWindow::openMaskOverlay);

  fileMenu->addSeparator();

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

  viewMenu->addSeparator();

  QAction* showImageViewerAction = viewMenu->addAction("Image Viewer");
  showImageViewerAction->setStatusTip("Switch to the main image viewer page");
  connect(showImageViewerAction, &QAction::triggered, this, &MainWindow::showImagePage);

  QAction* showMedicalVolumeViewerAction = viewMenu->addAction("Medical Volume Viewer");
  showMedicalVolumeViewerAction->setStatusTip("Switch to the medical volume viewer page");
  connect(showMedicalVolumeViewerAction, &QAction::triggered, this,
          &MainWindow::showMedicalVolumePage);

  QAction* showVolume3DViewerAction = viewMenu->addAction("3D Volume Viewer");
  showVolume3DViewerAction->setStatusTip("Switch to the 3D volume viewer page");
  connect(showVolume3DViewerAction, &QAction::triggered, this, &MainWindow::showVolume3DPage);
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
  QMenu* demoMenu = menuBar()->addMenu("&Tools");

  openSyntheticVolumeSliceAction_ = demoMenu->addAction("Load Synthetic Test Volume");
  openSyntheticVolumeSliceAction_->setStatusTip("Load a synthetic test volume into the medical viewer");
  connect(openSyntheticVolumeSliceAction_, &QAction::triggered, this,
          &MainWindow::openSyntheticVolumeSlice);

  openRawVolumeAction_ = demoMenu->addAction("Load Custom RAW Test Volume...");
  openRawVolumeAction_->setStatusTip("Load a custom float32 RAW test volume using JSON metadata");
  connect(openRawVolumeAction_, &QAction::triggered, this, &MainWindow::openRawVolume);
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

void MainWindow::openMedicalVolume()
{
  const QString fileName = QFileDialog::getOpenFileName(
      this, "Open Medical Volume", QString(),
      "Medical Volumes ("
      "*.nii *.nii.gz "
      "*.mhd *.mha "
      "*.dcm "
      "*.nrrd *.nhdr"
      ");;"
      "NIfTI (*.nii *.nii.gz);;"
      "MetaImage (*.mhd *.mha);;"
      "DICOM (*.dcm);;"
      "NRRD (*.nrrd *.nhdr);;"
      "All Files (*)");

  if (fileName.isEmpty())
  {
    return;
  }

  VolumeLoadResult result = loadMedicalVolume(fileName);
  if (!result.success)
  {
    QMessageBox::warning(this, "Medical Volume Load Error", result.errorMessage);
    return;
  }

  displayLoadedVolume(std::move(result.volume));
}

void MainWindow::openDicomSeriesFolder()
{
  const QString directoryPath =
      QFileDialog::getExistingDirectory(this, "Open DICOM Series Folder", QString());
  if (directoryPath.isEmpty())
  {
    return;
  }

  const DicomVolumeLoader loader;
  VolumeLoadResult result = loader.loadSeriesDirectory(directoryPath);
  if (!result.success)
  {
    QMessageBox::warning(this, "Medical Volume Load Error", result.errorMessage);
    return;
  }

  displayLoadedVolume(std::move(result.volume));
}

void MainWindow::openMaskOverlay()
{
  showMedicalVolumePage();
  medicalVolumeViewerWidget_->openMaskOverlay();
}

void MainWindow::displayLoadedVolume(VolumeData volume)
{
  auto sharedVolume = std::make_shared<const VolumeData>(std::move(volume));
  medicalVolumeViewerWidget_->setVolume(sharedVolume);
  volume3DViewerWidget_->setVolume(sharedVolume);
  showMedicalVolumePage();
}

void MainWindow::showImagePage()
{
  pageStack_->setCurrentWidget(viewer_);
}

void MainWindow::showMedicalVolumePage()
{
  pageStack_->setCurrentWidget(medicalVolumeViewerWidget_);
}

void MainWindow::showVolume3DPage()
{
  pageStack_->setCurrentWidget(volume3DViewerWidget_);
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
  displayLoadedVolume(createSyntheticVolume());
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

  const QString metadataError = validateRawMetadataFile(metadataPath);
  if (!metadataError.isEmpty())
  {
    showRawVolumeLoadError(this, metadataError);
    return;
  }

  const QString rawPath = QFileDialog::getOpenFileName(this, "Open RAW Volume Data", QString(),
                                                       "RAW Float32 Volume (*.raw);;All Files (*)");
  if (rawPath.isEmpty())
  {
    return;
  }

  try
  {
    VolumeData volume = RawVolumeLoader::load(metadataPath, rawPath);
    displayLoadedVolume(std::move(volume));
  }
  catch (const std::exception& error)
  {
    showRawVolumeLoadError(this, QString::fromUtf8(error.what()));
  }
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
