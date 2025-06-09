#include "CFitDataSettingsDialog.h"
#include "helpers/CSettings.h"
#include<QListWidgetItem>
#include <QSqlQuery>
#include <QSqlError>

CFitDataSettingsDialog::CFitDataSettingsDialog(
    QWidget *parent
    , const QMap<qint32, struct CFitDataDialog::column_t>& columns
    , QList<qint32>& shownTableCols
    , QList<qint32>& shownMivs
    , qint32 maxMivs) : QDialog(parent), shownTableCols(shownTableCols), shownMivs(shownMivs), maxMivs(maxMivs) {

  setupUi(this);

  tableColsWidget = new CFitDataSettingsSelectWidget(
      layoutTreeTableCols
      , columns
      , shownTableCols);

  mivsWidget = new CFitDataSettingsSelectWidget(
      layoutMivCols
      , columns
      , shownMivs
      , maxMivs);

  labelTreeTableCols->setText(tr("Select Table Columns:"));
  labelMiv->setText(QString(tr("Select Most Important Values")
                            + ((maxMivs == -1) ? (":") : QString(tr(" (max. %L1 Values):")).arg(maxMivs))));

  SETTINGS;
  cfg.beginGroup("FitData");
  QString curDbName = cfg.value("curDbName", "").toString();
  cfg.endGroup();

  qint32 curIndex;
  cfg.beginGroup("Database");
  const QStringList& names = cfg.value("names").toStringList();
  cfg.beginGroup("Entries");
  for (const QString& name : names) {
    cfg.beginGroup(name);
    QString type = cfg.value("type").toString();
    if (type == "MySQL") {
      comboBoxDb->addItem(name);
      if (name == curDbName) {
        curIndex = comboBoxDb->count() - 1;
      }
    }
    cfg.endGroup();  //name
  }
  cfg.endGroup();  //Entries
  cfg.endGroup();  //Database
  if (comboBoxDb->count()) {
    comboBoxDb->setCurrentIndex(curIndex);
  }

  connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &CFitDataSettingsDialog::slotOk);
}

void CFitDataSettingsDialog::slotOk() {
  shownTableCols.clear();
  shownMivs.clear();
  tableColsWidget->getSelectedCols(shownTableCols);
  mivsWidget->getSelectedCols(shownMivs);

  SETTINGS;
  cfg.beginGroup("FitData");
  cfg.setValue("curDbName", comboBoxDb->currentText());
  cfg.endGroup();
}

CFitDataSettingsDialog::~CFitDataSettingsDialog() {
}


//Methods for class CFitDataSettingsSelectWidget
CFitDataSettingsSelectWidget::CFitDataSettingsSelectWidget(
    QHBoxLayout *hBoxParent
    , const QMap<qint32, struct CFitDataDialog::column_t>& columns
    , const QList<qint32>& shownCols
    , qint32 maxMivs) : maxMivs(maxMivs) {

  //Left listWidget
  QVBoxLayout* vBoxAvailList = new QVBoxLayout();
  vBoxAvailList->addWidget(new QLabel(tr("Available Columns")));
  listAvailable = new QListWidget();
  listAvailable->setSortingEnabled(true);

  QMapIterator<qint32, struct CFitDataDialog::column_t> column(columns);
  while (column.hasNext())
  {
    column.next();
    if (!shownCols.contains(column.key())) {
      QListWidgetItem* item = new QListWidgetItem();
      QString text = QString("%L1 ").arg(column.key(), 2) + column.value().label;
      item->setText(text);
      item->setData(Qt::UserRole, column.key());
      listAvailable->addItem(item);
    }
  }
  vBoxAvailList->addWidget(listAvailable);
  hBoxParent->addLayout(vBoxAvailList);

  //Left Tools
  QVBoxLayout* vBoxAvailTools = new QVBoxLayout();
  vBoxAvailTools->addItem(new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding));
  toolSelect = new QToolButton();
  toolSelect->setObjectName("toolSelect");
  toolSelect->setEnabled(false);
  QIcon iconSelect;
  iconSelect.addFile(QString::fromUtf8(":/icons/32x32/Right.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
  toolSelect->setIcon(iconSelect);
  vBoxAvailTools->addWidget(toolSelect);
  vBoxAvailTools->addItem(new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding));
  toolRemove = new QToolButton();
  toolRemove->setObjectName("toolSelect");
  toolRemove->setEnabled(false);
  QIcon iconRemove;
  iconRemove.addFile(QString::fromUtf8(":/icons/32x32/Left.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
  toolRemove->setIcon(iconRemove);
  vBoxAvailTools->addWidget(toolRemove);
  vBoxAvailTools->addItem(new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding));
  hBoxParent->addLayout(vBoxAvailTools);

  //Right listWidget
  QVBoxLayout* vBoxSelectedList = new QVBoxLayout();
  vBoxSelectedList->addWidget(new QLabel(tr("Selected Columns")));
  listSelected = new QListWidget();
  listSelected->resize(250, 250);
  for (qint32 shownCol : shownCols) {
    QListWidgetItem* item = new QListWidgetItem();
    QString text = QString("%L1 ").arg(shownCol, 2) + columns[shownCol].label;
    item->setText(text);
    item->setData(Qt::UserRole, shownCol);
    listSelected->addItem(item);
  }
  vBoxSelectedList->addWidget(listSelected);
  hBoxParent->addLayout(vBoxSelectedList);

  //Right Tools
  QVBoxLayout* vBoxSelectedTools = new QVBoxLayout();
  vBoxSelectedTools->addItem(new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding));
  toolUp = new QToolButton();
  toolUp->setObjectName("toolUp");
  toolUp->setEnabled(false);
  QIcon iconUp;
  iconUp.addFile(QString::fromUtf8(":/icons/32x32/Up.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
  toolUp->setIcon(iconUp);
  vBoxSelectedTools->addWidget(toolUp);
  vBoxSelectedTools->addItem(new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding));
  toolDown = new QToolButton();
  toolDown->setObjectName("toolDown");
  toolDown->setEnabled(false);
  QIcon iconDown;
  iconDown.addFile(QString::fromUtf8(":/icons/32x32/Down.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
  toolDown->setIcon(iconDown);
  vBoxSelectedTools->addWidget(toolDown);
  vBoxSelectedTools->addItem(new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding));
  hBoxParent->addLayout(vBoxSelectedTools);

  connect(listAvailable, &QListWidget::itemSelectionChanged, this, &CFitDataSettingsSelectWidget::slotSelectionChanged);
  connect(listSelected, &QListWidget::itemSelectionChanged, this, &CFitDataSettingsSelectWidget::slotSelectionChanged);
  connect(toolSelect, &QToolButton::clicked, this, &CFitDataSettingsSelectWidget::slotSelect);
  connect(toolRemove, &QToolButton::clicked, this, &CFitDataSettingsSelectWidget::slotRemove);
  connect(toolUp, &QToolButton::clicked, this, &CFitDataSettingsSelectWidget::slotUp);
  connect(toolDown, &QToolButton::clicked, this, &CFitDataSettingsSelectWidget::slotDown);

  slotSelectionChanged();
}

void CFitDataSettingsSelectWidget::getSelectedCols(QList<qint32>& selectedCols) {
  selectedCols.clear();
  for (qint32 i = 0; i < listSelected->count(); ++i) {
    QListWidgetItem* item = listSelected->item(i);
    selectedCols << item->data(Qt::UserRole).toInt();
  }
}

void CFitDataSettingsSelectWidget::slotSelectionChanged() {
  if (maxMivs == -1 || listSelected->count() < maxMivs) {
      QListWidgetItem* item = listAvailable->currentItem();
      toolSelect->setEnabled(item != nullptr); //Enable when a item is selected
    } else {
      toolSelect->setEnabled(false); //Disable when maximum reached
  }

  QListWidgetItem* item = listSelected->currentItem();
  toolRemove->setEnabled(item != nullptr);
  toolUp->setEnabled(item != nullptr);
  toolDown->setEnabled(item != nullptr);

  if (item) {
    if (listSelected->row(item) == 0) {
      toolUp->setEnabled(false);
    }
    if (listSelected->row(item) == (listSelected->count() - 1)) {
      toolDown->setEnabled(false);
    }
  }
}

void CFitDataSettingsSelectWidget::slotSelect() {
  QListWidgetItem* item = listAvailable->currentItem();

  if (nullptr == item) {
    return;
  }

  listAvailable->takeItem(listAvailable->row(item));
  listSelected->addItem(item);

  slotSelectionChanged();
}

void CFitDataSettingsSelectWidget::slotRemove() {
  QListWidgetItem* item = listSelected->currentItem();

  if (nullptr == item) {
    return;
  }

  listSelected->takeItem(listSelected->row(item));
  listAvailable->addItem(item);

  slotSelectionChanged();
}

void CFitDataSettingsSelectWidget::slotUp() {
  QListWidgetItem* item = listSelected->currentItem();
  if (item) {
    int row = listSelected->row(item);
    if (row == 0) {
      return;
    }
    listSelected->takeItem(row);
    row = row - 1;
    listSelected->insertItem(row, item);
    listSelected->setCurrentItem(item);
  }
}

void CFitDataSettingsSelectWidget::slotDown() {
  QListWidgetItem* item = listSelected->currentItem();
  if (item) {
    int row = listSelected->row(item);
    if (row == (listSelected->count() - 1)) {
      return;
    }
    listSelected->takeItem(row);
    row = row + 1;
    listSelected->insertItem(row, item);
    listSelected->setCurrentItem(item);
  }
}

CFitDataSettingsSelectWidget::~CFitDataSettingsSelectWidget() {
}
