#ifndef CFITDATASETTINGSDIALOG_H
#define CFITDATASETTINGSDIALOG_H

#include <QDialog>

#include "ui_IFitDataSettingsDialog.h"

class CFitDataSettingsDialog : public QDialog, private Ui::IFitDataSettingsDialog {
  Q_OBJECT

 public:
  explicit CFitDataSettingsDialog(QWidget *parent = nullptr);
  ~CFitDataSettingsDialog();

 private:
};

class CFitDataSettingsSelectCols : public QWidget {
  Q_OBJECT
 public:
  explicit CFitDataSettingsSelectCols(QWidget *parent = nullptr);
  ~CFitDataSettingsSelectCols();

private:
  QList<qint32> availableCols;
  QList<qint32> selectedCols;


};

#endif  // CFITDATASETTINGSDIALOG_H
