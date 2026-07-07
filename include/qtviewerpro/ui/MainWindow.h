#pragma once

#include "qtviewerpro/core/VolumeData.h"

#include <QImage>
#include <QMainWindow>
#include <QFutureWatcher>
#include <QString>
#include <QStringList>

#include <cstddef>
#include <chrono>
#include <memory>

class QMenu;
class QCloseEvent;
class QAction;
class QStackedWidget;

namespace qvp
{

class ImageViewer2D;
class MprViewerWidget;
class OpenGLVolumeViewerWidget;
class Volume3DViewerWidget;
enum class VolumeRenderPreset;

/**
 * @brief Main application window for Qt Viewer Pro.
 *
 * MainWindow owns the top-level UI composition, menus, toolbar actions,
 * recent-file state, and coordination between image loading, viewing, and
 * processing components.
 */
class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  /**
   * @brief Constructs the main window and initializes its child widgets.
   * @param parent Optional Qt parent widget.
   */
  explicit MainWindow(QWidget* parent = nullptr);

  /**
   * @brief Destroys the main window.
   */
  ~MainWindow() override = default;

protected:
  void closeEvent(QCloseEvent* event) override;

private slots:
  void openImage();
  void openImage(const QString& fileName);
  void saveImageAs();
  void fitToWindow();
  void actualSize();
  void zoomIn();
  void zoomOut();
  void rotateLeft();
  void rotateRight();
  void flipHorizontal();
  void flipVertical();
  void convertToGrayscale();
  void resetImage();
  void reset3DView();
  void setVolumeRenderPreset(VolumeRenderPreset preset);
  void handleVolumeResampleFinished();
  void resampleVolumeToIsotropicSpacing();
  void openMedicalVolume();
  void openDicomSeriesFolder();
  void openMaskOverlay();
  void openSyntheticVolumeSlice();
  void openRawVolume();
  void showAboutDialog();
  void openRecentFile();
  void clearRecentFiles();

private:
  struct VolumeResampleResult
  {
    std::shared_ptr<const VolumeData> sourceVolume;
    VolumeData volume;
    QString errorMessage;
    bool success = false;
    std::chrono::steady_clock::time_point backgroundStart;
  };

  // UI
  void createViewer();
  void createMenus();
  void createFileMenu();
  void createViewMenu();
  void createProcessingMenu();
  void createImageMenu();
  void createDemoMenu();
  void createHelpMenu();
  void createStatusBar();
  void updateStatusBar();
  void updateActions();
  void createToolBar();
  void showImagePage();
  void showMedicalVolumePage();
  void showMprViewerPage();
  void showVolume3DPage();
  void displayLoadedVolume(VolumeData volume);
  VolumeData createSyntheticVolume() const;

  QAction* rotateLeftAction_ = nullptr;
  QAction* rotateRightAction_ = nullptr;

  // Recent files
  void addRecentFile(const QString& fileName);
  void updateRecentFilesMenu();

  // Settings
  void loadSettings();
  void saveSettings();

private:
  QStackedWidget* pageStack_ = nullptr;
  ImageViewer2D* viewer_ = nullptr;
  MprViewerWidget* mprViewerWidget_ = nullptr;
  OpenGLVolumeViewerWidget* medicalVolumeViewerWidget_ = nullptr;
  Volume3DViewerWidget* volume3DViewerWidget_ = nullptr;
  QImage originalImage_;
  std::shared_ptr<const VolumeData> currentMedicalVolume_;
  QFutureWatcher<VolumeResampleResult> resampleWatcher_;
  bool resamplingInProgress_ = false;
  QMenu* recentMenu_ = nullptr;
  QStringList recentFiles_{};

  // QAction section
  QAction* openAction_ = nullptr;
  QAction* saveAsAction_ = nullptr;
  QAction* zoomInAction_ = nullptr;
  QAction* zoomOutAction_ = nullptr;
  QAction* fitAction_ = nullptr;
  QAction* actualSizeAction_ = nullptr;

  QAction* flipHorizontalAction_ = nullptr;
  QAction* flipVerticalAction_ = nullptr;

  QAction* grayscaleAction_ = nullptr;
  QAction* resetImageAction_ = nullptr;
  QAction* openMedicalVolumeAction_ = nullptr;
  QAction* resampleVolumeAction_ = nullptr;
  QAction* openMaskOverlayAction_ = nullptr;
  QAction* openSyntheticVolumeSliceAction_ = nullptr;
  QAction* openRawVolumeAction_ = nullptr;
  QAction* aboutAction_ = nullptr;
};

} // namespace qvp
