#pragma once

#include <QImage>
#include <QMainWindow>
#include <QStringList>

class QMenu;
class QCloseEvent;
class QAction;

namespace qvp
{

class ImageViewer2D;
class VolumeData;

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
  void openSyntheticVolumeSlice();
  void showAboutDialog();
  void openRecentFile();
  void clearRecentFiles();

private:
  // UI
  void createViewer();
  void createMenus();
  void createFileMenu();
  void createViewMenu();
  void createImageMenu();
  void createDemoMenu();
  void createHelpMenu();
  void createStatusBar();
  void updateStatusBar();
  void updateActions();
  void createToolBar();
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
  ImageViewer2D* viewer_ = nullptr;
  QImage originalImage_;
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
  QAction* openSyntheticVolumeSliceAction_ = nullptr;
  QAction* aboutAction_ = nullptr;
};

} // namespace qvp
