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

#endif  // CFITDATASETTINGSDIALOG_H
