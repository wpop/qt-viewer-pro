#pragma once

#include <QMessageBox>
#include <QString>
#include <QWidget>

namespace qvp
{

inline QString darkDialogStyleSheet()
{
  return QStringLiteral(R"(
QMessageBox {
  background-color: #1E1E1E;
}

QLabel {
  color: #E6E6E6;
}

QPushButton {
  background-color: #333333;
  color: #E6E6E6;
  border: 1px solid #3A3A3A;
  border-radius: 4px;
  padding: 4px 8px;
  min-width: 72px;
}

QPushButton:hover {
  background-color: #3A3A3A;
}

QPushButton:pressed {
  background-color: #444444;
  color: #FFFFFF;
}

QPushButton:disabled {
  color: #777777;
  background-color: #252526;
}
)");
}

inline void styleMessageBox(QMessageBox& messageBox)
{
  messageBox.setStyleSheet(darkDialogStyleSheet());
}

inline int showStyledMessageBox(QWidget* parent,
                                QMessageBox::Icon icon,
                                const QString& title,
                                const QString& text,
                                const QString& informativeText = QString())
{
  QMessageBox messageBox(parent);
  messageBox.setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint |
                            Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                            Qt::WindowCloseButtonHint);
  messageBox.setWindowModality(Qt::WindowModal);
  messageBox.setAttribute(Qt::WA_NativeWindow);
  messageBox.setIcon(icon);
  messageBox.setWindowTitle(title);
  messageBox.setText(text);
  if (!informativeText.isEmpty())
  {
    messageBox.setInformativeText(informativeText);
  }
  styleMessageBox(messageBox);
  messageBox.winId();
  return messageBox.exec();
}

inline int showStyledWarning(QWidget* parent,
                             const QString& title,
                             const QString& text,
                             const QString& informativeText = QString())
{
  return showStyledMessageBox(parent, QMessageBox::Warning, title, text, informativeText);
}

inline int showStyledInformation(QWidget* parent,
                                 const QString& title,
                                 const QString& text,
                                 const QString& informativeText = QString())
{
  return showStyledMessageBox(parent, QMessageBox::Information, title, text, informativeText);
}

} // namespace qvp
