#include "CFitDataSettingsDialog.h"

CFitDataSettingsDialog::CFitDataSettingsDialog(QWidget *parent) : QDialog(parent) {
  setupUi(this);
  QListWidgetItem* item;
  item = new QListWidgetItem();
  item->setText("Custom Item_1");
  item->setData(Qt::UserRole, 1);
  listWidget_2->addItem(item);
  item = new QListWidgetItem();
  item->setText("Custom Item_2");
  item->setData(Qt::UserRole, 2);
  listWidget_2->addItem(item);
}

CFitDataSettingsDialog::~CFitDataSettingsDialog() { }

/*
 *
 */

CFitDataSettingsSelectCols::CFitDataSettingsSelectCols(QWidget *parent) : QWidget(parent) {

}

CFitDataSettingsSelectCols::~CFitDataSettingsSelectCols() { }
