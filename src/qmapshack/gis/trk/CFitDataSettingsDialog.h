#ifndef CFITDATASETTINGSDIALOG_H
#define CFITDATASETTINGSDIALOG_H

#include "CFitData.h"
#include "CFitDataDialog.h"

#include <QListWidget>
#include <QtWidgets/QToolButton>

#include "ui_IFitDataSettingsDialog.h"

class CFitDataSettingsSelectWidget;

class CFitDataSettingsDialog : public QDialog, private Ui::IFitDataSettingsDialog {
  Q_OBJECT

 public:
  explicit CFitDataSettingsDialog(
      QWidget *parent
      , const QMap<qint32, CFitDataDialog::column_t>& columns
      , QList<qint32> &shownTableCols
      , QList<qint32> &shownMivs
      , qint32 maxMivs = -1);

  ~CFitDataSettingsDialog();

 private slots:
  void slotOk();

 private:
  QList<qint32>& shownTableCols;
  QList<qint32>& shownMivs;
  qint32 maxMivs;
  CFitDataSettingsSelectWidget* tableColsWidget;
  CFitDataSettingsSelectWidget* mivsWidget;
};


//Widget class CFitDataSettingsSelectWidget

class CFitDataSettingsSelectWidget : public QHBoxLayout {
  Q_OBJECT
 public:
  explicit CFitDataSettingsSelectWidget(
      QHBoxLayout* hBoxParent
      , const QMap<qint32, CFitDataDialog::column_t>& columns
      , const QList<qint32>& shownCols
      , qint32 maxMivs = -1);

  void getSelectedCols(QList<qint32>& selectedCols);

  ~CFitDataSettingsSelectWidget();

 private slots:
  void slotSelectionChanged();
  void slotSelect();
  void slotRemove();
  void slotUp();
  void slotDown();

 private:
  QListWidget* listAvailable;
  QListWidget* listSelected;
  QToolButton* toolSelect;
  QToolButton* toolRemove;
  QToolButton* toolUp;
  QToolButton* toolDown;
  qint32 maxMivs;
};

#endif  // CFITDATASETTINGSDIALOG_H
