#pragma once

#include <QMainWindow>
#include <QStringList>
#include <QImage>

class QMenu;
class ImageViewer;
class QCloseEvent;
class QAction;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override = default;

protected:
  void closeEvent(QCloseEvent *event) override;

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
  void createHelpMenu();
  void createStatusBar();
  void updateStatusBar();
  void updateActions();
  void createToolBar();

  QAction *rotateLeftAction_ = nullptr;
  QAction *rotateRightAction_ = nullptr;

  // Recent files
  void addRecentFile(const QString& fileName);
  void updateRecentFilesMenu();

  // Settings
  void loadSettings();
  void saveSettings();

private:
  ImageViewer *viewer_ = nullptr;
  QImage originalImage_;
  QMenu *recentMenu_ = nullptr;
  QStringList recentFiles_ {};

  // QAction section
  QAction *openAction_ = nullptr;
  QAction *saveAsAction_ = nullptr;
  QAction *zoomInAction_ = nullptr;
  QAction *zoomOutAction_ = nullptr;
  QAction *fitAction_ = nullptr;
  QAction *actualSizeAction_ = nullptr;

  QAction *flipHorizontalAction_ = nullptr;
  QAction *flipVerticalAction_ = nullptr;

  QAction *grayscaleAction_ = nullptr;
  QAction *resetImageAction_ = nullptr;
  QAction *aboutAction_ = nullptr;
};
