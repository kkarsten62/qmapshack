/**********************************************************************************************

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

 **********************************************************************************************/

#include "CMainWindow.h"
#include "gis/trk/CGisItemTrk.h"
#include "gis/trk/CFitData.h"
#include "gis/trk/CFitDataDialog.h"
#include "gis/trk/CFitDataSettingsDialog.h"
#include "helpers/CSettings.h"
#include "helpers/CDraw.h"
#include "gis/db/macros.h"

#include <QSqlQuery>
#include <QSqlError>

CFitDataDialog::CFitDataDialog(QWidget* parent, CGisItemTrk& trk) :
                                                                    QDialog(parent)
                                                                    , trk(trk)
                                                                    , laps(trk.getFitData().getLaps()) {
  setupUi(this);

  checkShowTrkptInfo->setChecked(trk.getFitData().getIsTrkptInfo());

  buttonBox->button(QDialogButtonBox::Reset)->setText(tr("Remove"));
  buttonSettingsDialog = buttonBox->addButton(tr("Settings..."), QDialogButtonBox::ActionRole); //Show Settings Dialog

  buttonBox->button(QDialogButtonBox::Reset)->setToolTip(tr("Remove the Fit data from the track and close the dialog."));
  pushRemoveSessionFromDb->setEnabled(false);
  connect(checkShowSessionsDb, &QCheckBox::clicked, this, &CFitDataDialog::slotShowSessionsDb);
  connect(pushRemoveSessionFromDb, &QPushButton::clicked, this, &CFitDataDialog::slotDeleteSessionFromDb);
  connect(pushAddSessionToDb, &QPushButton::clicked, this, &CFitDataDialog::slotAddSessionToDb);
  connect(checkShowTrkptInfo, &QCheckBox::clicked, this, &CFitDataDialog::slotShowTrkptInfo);
  connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &CFitDataDialog::slotDeleteFitDataFromTrack);
  connect(buttonSettingsDialog, &QPushButton::clicked, this, &CFitDataDialog::slotSettingsDialog);
  connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &CFitDataDialog::slotOk);
  connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &CFitDataDialog::slotCancel);
  connect(treeTable, &QTreeWidget::itemDoubleClicked, this, &CFitDataDialog::slotItemDoubleClicked);
  connect(treeTable, &QTreeWidget::currentItemChanged, this, &CFitDataDialog::slotCurrentItemChanged);
  connect(pushHelp, &QPushButton::clicked, this, &CFitDataDialog::slotShowHelp);

          //Read settings
  SETTINGS;
  cfg.beginGroup("FitData");
  shownTableCols = cfg.value("shownTableCols").value<QList<qint32>>();
  shownMivs = cfg.value("shownMostImportantValues").value<QList<qint32>>();
  connectionDbName = cfg.value("curDbName", "").toString();
  cfg.endGroup();
  checkShowSessionsDb->setEnabled(connectionDbName.size() ? true : false);
  pushAddSessionToDb->setEnabled(connectionDbName.size() ? true : false);
  //checkCurSessionExistInDb();
          //Create the labels for the most important values
  qint32 row = 0;
  for (qint32 i = 0; i < maxMivs; ++i) {
    QLabel* labelMivName = new QLabel();
    QLabel* labelMivValue = new QLabel();
    mivLabels << labelMivName << labelMivValue;
    gridLayoutMiv->addWidget(labelMivName, row, 0);
    gridLayoutMiv->addWidget(labelMivValue, row, 1);
    ++row;
  }
  gridLayoutMiv->addItem(new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding), row, 0);

  updateData(trk.getFitData().getLaps());
  checkDbAccess();
  enableButtons();
}

CFitDataDialog::~CFitDataDialog() {
}

bool CFitDataDialog::checkDbAccess() {
  bool dBAccess = true;

  if (!QSqlDatabase::contains(connectionDbName)) {
    qWarning() << tr("The database '%1' has no connection!").arg(connectionDbName);
    dBAccess = false;
  }
  QSqlDatabase db = QSqlDatabase::database(connectionDbName);
  if (!db.isValid()) {
    qWarning() << tr("The database '%1' is not valid!").arg(connectionDbName);
    dBAccess = false;
  }
  QSqlQuery query(db);
  query.prepare("SELECT TABLE_SCHEMA, TABLE_NAME, TABLE_TYPE "
      "FROM information_schema.TABLES "
      "WHERE TABLE_SCHEMA LIKE :connectionDbName AND "
      "TABLE_TYPE LIKE 'BASE TABLE' AND TABLE_NAME = 'fitdata'");
  query.bindValue(":connectionDbName", connectionDbName);
  QUERY_EXEC();
  if (!query.next()) {
    qWarning() << tr("The database '%1' has no table fitdata!").arg(connectionDbName);
    dBAccess = false;
  }
  checkShowSessionsDb->setEnabled(dBAccess);
  return dBAccess;
}

void CFitDataDialog::enableButtons() {
  if (checkShowSessionsDb->checkState()) { //Sessions are show
    buttonSettingsDialog->setEnabled(false);

    QSqlDatabase db = QSqlDatabase::database(connectionDbName);
    QSqlQuery query(db);
    query.prepare("SELECT starttime FROM fitdata WHERE starttime = :starttime");
    query.bindValue(":starttime", trk.getFitData().getLap(trk.getFitData().getNoOfLaps() - 1).startTime);
    QUERY_EXEC();
    if (query.next()) {
      pushAddSessionToDb->setEnabled(false);
      pushAddSessionToDb->setToolTip(tr("Current session already exists in DB or there is no access to DB"));
    } else {
      pushAddSessionToDb->setEnabled(true);
      pushAddSessionToDb->setToolTip("");
    }
    pushRemoveSessionFromDb->setEnabled(nullptr != treeTable->currentItem());
  } else { //Laps are shown
    buttonSettingsDialog->setEnabled(true);
    pushAddSessionToDb->setEnabled(false);
    pushRemoveSessionFromDb->setEnabled(false);
  }
}

void CFitDataDialog::updateData(const QList<CFitData::lap_t> &laps) {
  //Add header labels to treeTable for laps and session values
  QTreeWidgetItem* item = new QTreeWidgetItem();
  qint32 treeCol = 0;
  for (qint32 shownTableCol : shownTableCols) {
    struct column_t& column = columns[shownTableCol];
    item->setText(treeCol++, column.label);
  }
  treeTable->setHeaderItem(item);

          //Add values to treeTable
  QList<QTreeWidgetItem*> items;
  qint32 treeRow = 0;
  for(const CFitData::lap_t& lap : laps) { //For all laps/session
    treeCol = 0;
    QTreeWidgetItem *item = new QTreeWidgetItem();
    for (qint32 shownTableCol : shownTableCols) { //For all shown tree columns
      QString cellStr;
      getCellStr(lap, shownTableCol, cellStr);
      item->setText(treeCol, cellStr);
      item->setTextAlignment(treeCol, columns[shownTableCol].alignment);
      item->setData(treeCol, Qt::UserRole, shownTableCol);
      item->setData(treeCol, Qt::UserRole + 1, treeRow);
      item->setData(treeCol, Qt::UserRole + 2, lap.startTime); //To have a relation to the db

      if (shownTableCol == eColComment && checkShowSessionsDb->checkState() == Qt::Unchecked) {
        item->setToolTip(treeCol, tr("Double click to edit comment"));
      }
      ++treeCol;
    }
    if (lap.type == CFitData::eTypeSession
        && checkShowSessionsDb->checkState() == Qt::Unchecked) { //Set text to bold for session row for lap view
      QFont font = QFont();
      font.setBold(true);
      for (qint32 i = 0; i < item->columnCount();item->setFont(i, font), ++i);
    }
    items << item;
    ++treeRow;
  }
  treeTable->clear();
  treeTable->addTopLevelItems(items);
  QTreeWidgetItem * tl0 = treeTable->topLevelItem(0);
  if (nullptr != tl0) {
    treeTable->setCurrentItem(tl0);
  }
  treeTable->header()->resizeSections(QHeaderView::ResizeToContents);

  updateDataMivs();

          //Hide non-shown mivs
  for (qint32 i = shownMivs.count(); i < maxMivs; ++i) {
    QLabel* labelMivName = mivLabels[2 * i];
    QLabel* labelMivValue = mivLabels[2 * i + 1];
    labelMivName->hide();
    labelMivValue->hide();
  }
  treeTable->setFocus();
}

void CFitDataDialog::updateDataMivs() {
  QTreeWidgetItem* curItem = treeTable->currentItem();
  if (nullptr == curItem) {
    return;
  }
  qint32 treeRow = curItem->data(0, Qt::UserRole + 1).toInt();

  const CFitData::lap_t& lap = laps[treeRow];
  //Show deviceName in GUI label
  QString deviceStr;
  getDeviceName(lap, deviceStr);
  labelDeviceName->setText(QString(tr("Fit Data from Device %1")).arg(deviceStr));

  qint32 mivRow = 0;
  for (qint32 miv : shownMivs) { //For all shown miv
    struct column_t column = columns[miv];
    QLabel* labelMivName = mivLabels[2 * mivRow];
    QLabel* labelMivValue = mivLabels[2 * mivRow + 1];
    labelMivName->setText(column.label + ":");
    QString mivValueStr;
    getCellStr(lap, miv, mivValueStr);
    labelMivValue->setText(mivValueStr);
    if (miv == eColComment) {
      labelMivValue->setToolTip(mivValueStr);
    }
    labelMivName->show();
    labelMivValue->show();
    ++mivRow;
  }
  paintGraphics(lap);
}

QString CFitDataDialog::getPowerPhaseStr(const QList<qreal>& powerPhases, qint32 phase) {
  QString cellStr = "0°,0°,0°,0°";
  if (powerPhases.count() == 16) {
    QStringList strList;
    for (qint32 i = phase * 4; i < phase * 4 + 4; ++i) {
      strList << QString("%L1°").arg(powerPhases[i], 0, 'f', 1);
    }
    cellStr = strList.join(',');
  }
  return cellStr;
}

void CFitDataDialog::getCellStr(const CFitData::lap_t& lap, qint32 column, QString& cellStr) {
  QString val, unit;
  cellStr = "";
  switch (column) {
    case eColManufacturer:
      cellStr = QString(tr("Unknown"));
      if (manufacturers.contains(lap.manufacturer)) {
        cellStr = manufacturers[lap.manufacturer];
      }
      break;
    case eColProduct:
      cellStr = QString(tr("Unknown"));
      for (struct product_t product : products) {
        if (product.manufacturer == lap.manufacturer && product.product == lap.product) {
          cellStr = product.productStr;
        }
      }
      break;
    case eColNo:
      cellStr = QString("%1").arg((lap.type == CFitData::eTypeLap) ? lap.no + 1 : lap.no);
      break;
    case eColType:
      if (lap.type == CFitData::eTypeLap) {
        cellStr = tr("Lap");
      } else if (lap.type == CFitData::eTypeSession) {
        cellStr = tr("Session");
      }
      break;
    case eColComment:
      cellStr = lap.comment;
      break;
    case eColStartTime:
      val = IUnit::self().datetime2string(lap.startTime, IUnit::eTimeFormatShortPlusSecs);
      cellStr = QString("%L1").arg(val);
      break;
    case eColElapsedTime:
      IUnit::self().seconds2time(lap.elapsedTime, val, unit);
      cellStr = QString("%L1%2").arg(val).arg(unit);
      break;
    case eColTimerTime:
      IUnit::self().seconds2time(lap.timerTime, val, unit);
      cellStr = QString("%L1%2").arg(val).arg(unit);
      break;
    case eColPause:
      IUnit::self().seconds2time(lap.elapsedTime - lap.timerTime, val, unit);
      cellStr = QString("%L1%2").arg(val).arg(unit);
      break;
    case eColDistance:
      IUnit::self().meter2distance(lap.distance, val, unit);
      cellStr = QString("%L1%2").arg(val).arg(unit);
      break;
    case eColAvgSpeed:
      IUnit::self().meter2speed(lap.avgSpeed / 1000., val, unit);
      cellStr = QString("%L1%2").arg(val).arg(unit);
      break;
    case eColMaxSpeed:
      IUnit::self().meter2speed(lap.maxSpeed / 1000., val, unit);
      cellStr = QString("%L1%2").arg(val).arg(unit);
      break;
    case eColAscent:
      IUnit::self().meter2elevation(lap.ascent, val, unit);
      cellStr = QString("%L1%2").arg(val).arg(unit);
      break;
    case eColDescent:
      IUnit::self().meter2elevation(lap.descent, val, unit);
      cellStr = QString("%L1%2").arg(val).arg(unit);
      break;
    case eColAvgHr:
      cellStr = QString("%L1%2").arg(lap.avgHr).arg(tr("bpm"));
      break;
    case eColMaxHr:
      cellStr = QString("%L1%2").arg(lap.maxHr).arg(tr("bpm"));
      break;
    case eColAvgCad:
      cellStr = QString("%L1%2").arg(lap.avgCad).arg(tr("rpm"));
      break;
    case eColMaxCad:
      cellStr = QString("%L1%2").arg(lap.maxCad).arg(tr("rpm"));
      break;
    case eColAvgPower:
      cellStr = QString("%L1%2").arg(lap.avgPower).arg(tr("Watt"));
      break;
    case eColMaxPower:
      cellStr = QString("%L1%2").arg(lap.maxPower).arg(tr("Watt"));
      break;
    case eColNormPower:
      cellStr = QString("%L1%2").arg(lap.normPower).arg(tr("Watt"));
      break;
    case eColLeftBalance:
    {
      qreal leftBalance = 0;
      if (lap.leftRightBalance & 0x8000) { //According to FIT type "left_right_balance_100"
        qreal rightBalance = (lap.leftRightBalance & 0x3FFF) / 100.;
        leftBalance = 100. - rightBalance;
      }
      cellStr = QString("%L1%").arg(leftBalance, 0, 'f', 1);
    }
    break;
    case eColRightBalance:
    {
      qreal rightBalance = 0;
      if (lap.leftRightBalance & 0x8000) { //According to FIT type "left_right_balance_100"
        rightBalance = (lap.leftRightBalance & 0x3FFF) / 100.;
      }
      cellStr = QString("%L1%").arg(rightBalance, 0, 'f', 1);
    }
    break;
    case eColLeftPedalSmooth:
      cellStr = QString("%L1%").arg(lap.leftPedalSmooth, 0, 'f', 1);
      break;
    case eColRightPedalSmooth:
      cellStr = QString("%L1%").arg(lap.rightPedalSmooth, 0, 'f', 1);
      break;
    case eColLeftTorqueEff:
      cellStr = QString("%L1%").arg(lap.leftTorqueEff, 0, 'f', 1);
      break;
    case eColRightTorqueEff:
      cellStr = QString("%L1%").arg(lap.rightTorqueEff, 0, 'f', 1);
      break;
    case eColLeftPco:
      cellStr = QString("%1mm").arg(lap.leftPco);
      break;
    case eColRightPco:
      cellStr = QString("%1mm").arg(lap.rightPco);
      break;
    case eColLeftPp:
      cellStr = getPowerPhaseStr(lap.powerPhases, 0);
      break;
    case eColLeftPpPeak:
      cellStr = getPowerPhaseStr(lap.powerPhases, 1);
      break;
    case eColRightPp:
      cellStr = getPowerPhaseStr(lap.powerPhases, 2);
      break;
    case eColRightPpPeak:
      cellStr = getPowerPhaseStr(lap.powerPhases, 3);
      break;
    case eColFtp:
      cellStr = "-";
      if (lap.type == CFitData::eTypeSession) {
        cellStr = QString("%L1%2").arg(lap.functionalThresholdPower).arg(tr("Watt"));
      }
      break;
    case eColIf:
      cellStr = "-";
      if (lap.type == CFitData::eTypeSession) {
        cellStr = QString("%L1").arg(lap.intensityFactor, 0, 'f', 2);
      }
      break;
    case eColTss:
      cellStr = "-";
      if (lap.type == CFitData::eTypeSession) {
        cellStr = QString("%L1").arg(lap.trainingStressScore, 0, 'f', 1);
      }
      break;
    case eColWork:
      cellStr = QString("%L1%2").arg(lap.work / 1000).arg(tr("kJ"));
      break;
    case eColEnergy:
      cellStr = QString("%L1%2").arg(lap.energy).arg(tr("kcal"));
      break;
  }
}

void CFitDataDialog::paintGraphics(const CFitData::lap_t& lap) {
  QSize size = labelGraphics->size();
  QImage image(size.width(), size.height(), QImage::Format_ARGB32);
  image.fill(Qt::lightGray);
  QPainter p;
  p.begin(&image);
  USE_ANTI_ALIASING(p, true);

  p.save(); //Save to initial state=0
  p.translate(10, 10); //Move to left border
  p.save(); //Save to state=1
  p.translate(0, 80); //Move down to center left of left pedal
  p.save(); //Save to state=2

  qint8 leftPcoVal = lap.leftPco;
  qint8 rightPcoVal = lap.rightPco;
  qint8 leftPco; //Maybe cutted
  qint8 rightPco; //Maybe cutted
  qint8 pco;
  for (qint32 i = 0; i < 2; ++i) {
    pco = leftPcoVal < 0 ? qMax(leftPcoVal, -30) : qMin(leftPcoVal, 30);
    leftPco = pco;
    if (i == 1) { //Print right pedal moved and mirrored
      p.translate(260, 0); //Move to right edge of right pedal
      p.scale(-1, 1); //Mirrored by y-axis
      pco = rightPcoVal < 0 ? qMax(rightPcoVal, -30) : qMin(rightPcoVal, 30);
      rightPco = pco;
    }
    p.setPen(QPen(QColor(Qt::black), 1, Qt::SolidLine,
                  Qt::FlatCap, Qt::MiterJoin));
    p.setBrush(QColor(Qt::darkGray));
    p.drawRect(5, -6, 75, 12); //Center axis of pedal
    QPainterPath path;
    path.addRoundedRect(0, -30, 60, 60, 3, 3); //Outer pedal rects
    path.addRect(5, -18, 50, 36);
    p.drawPath(path);
    p.drawRect(60, -15, 7, 30); //Pedal flange
    p.setPen(QPen(Qt::DashDotDotLine));
    p.drawLine(30, -40, 30, 40); //Center line of pedal
    p.setPen(QPen(QColor(Qt::red), 1, Qt::SolidLine));
    p.drawLine(30 - pco, -40, 30 - pco, 40); //PCO line, plus values to outer of the bike
  }
  p.restore(); //Back to state=2 center left of left pedal
  p.setPen(QPen(QColor(Qt::black)));
  //qint8 leftPco = lap.leftPco;
  QString valStr;
  getCellStr(lap, eColLeftPco, valStr);
  p.drawText(30 - leftPco, -45, QString("PCO: ") + valStr);
  getCellStr(lap, eColRightPco, valStr);
  p.drawText(230 + rightPco, -45, QString("PCO: ") + valStr);

  p.translate(0, 60); //Move down to text
  getCellStr(lap, eColLeftBalance, valStr);
  p.drawText(0, 0, QString(tr("Balance: ") + valStr));
  getCellStr(lap, eColLeftPedalSmooth, valStr);
  p.drawText(0, 20, QString(tr("Pedal Smoothness: ") + valStr));
  getCellStr(lap, eColLeftTorqueEff, valStr);
  p.drawText(0, 40, QString(tr("Torque Efficiency: ") + valStr));
  p.translate(180, 0); //Move to left edge of right pedal
  getCellStr(lap, eColRightBalance, valStr);
  p.drawText(0, 0, QString(tr("Balance: ") + valStr));
  getCellStr(lap, eColRightPedalSmooth, valStr);
  p.drawText(0, 20, QString(tr("Pedal Smoothness: ") + valStr));
  getCellStr(lap, eColRightTorqueEff, valStr);
  p.drawText(0, 40, QString(tr("Torque Efficiency: ") + valStr));
  p.restore(); //Back to state=1
  QFont font = QFont();
  font.setBold(true);
  font.setUnderline(true);
  p.setFont(font);
  p.drawText(0, 10, tr("Left Pedal"));
  p.drawText(180, 10, tr("Right Pedal"));
  font.setBold(false);
  font.setUnderline(false);
  p.setFont(font);
  p.restore(); //Back to state=0

          //Power Phases
  if (lap.powerPhases.count()) {
  //if (false) {
    p.translate(475, 0); //Center of left PP
    p.save(); //Save to state=1
    p.translate(0, 110); //Center of left PP

    QList<struct marker_t> markers;
    qint32 endAngleInner;
    qint32 spanAngleInner;
    qint32 endAngleOuter;
    qint32 spanAngleOuter;
    for (qint32 i = 0; i < 2; ++i) { //0=Left pedal, 1=right pedal
      p.save(); //Save to state=2
      if (i == 0) { //Print left pedal
        markers = {
          {lap.powerPhases[0], 65} //Start angle
            , {lap.powerPhases[1], 65} //End angle
            , {lap.powerPhases[4], 75} //Start peak angle
            , {lap.powerPhases[5], 75} //End peak angle
            , {lap.powerPhases[7], 75} //Center peak angle
        };
        endAngleOuter = lap.powerPhases[5];
        spanAngleOuter = lap.powerPhases[6];
        endAngleInner = lap.powerPhases[1];
        spanAngleInner = lap.powerPhases[2];
      } else if (i == 1) { //Print right pedal, moved
        markers = {
            {lap.powerPhases[8], 65} //Start angle
            , {lap.powerPhases[9], 65} //End angle
            , {lap.powerPhases[12], 75} //Start peak angle
            , {lap.powerPhases[13], 75} //End peak angle
            , {lap.powerPhases[15], 75} //Center peak angle
        };
        endAngleOuter = lap.powerPhases[13];
        spanAngleOuter = lap.powerPhases[14];
        endAngleInner = lap.powerPhases[9];
        spanAngleInner = lap.powerPhases[10];
        p.translate(200, 0);
      }

      p.setPen(QPen(Qt::SolidLine));
      p.setBrush(QColor(Qt::darkBlue));
      p.drawPie(-70, -70, 140, 140, -(endAngleOuter - 90) * 16, spanAngleOuter * 16); //PP peak, pie is on couterclock at 3 o'clock
      p.setBrush(QColor(Qt::darkGreen));
      p.drawPie(-60, -60, 120, 120, -(endAngleInner - 90) * 16, spanAngleInner * 16); //PP
      p.setBrush(QColor(Qt::darkGray));
      p.drawEllipse(-50, -50, 100, 100);

      p.rotate(-90); //PP angles are on 12 o'clock

      p.setPen(QPen(Qt::DashDotLine));
      for (struct marker_t marker: markers) {
        p.save(); //Save to state=3
        p.rotate(marker.angle); //Rotate angle ccw to 12 o'clock
        p.drawLine(0, 0, marker.length, 0); //Draw the dotted line, we have rotate, so x=y!
        p.translate(marker.length, 0); //Move to the end of the line
        p.rotate(90 - marker.angle); //Rotate cw to draw the text

        QRect rect;
        qint32 alignment;
        for (struct position_t position : positions) { //Find the right position at end of line
          if (marker.angle >= position.gt && marker.angle <= position.lt) {
            rect = position.rect;
            alignment = position.alignment;
          }
        }
        QString text = QString("%1°").arg(marker.angle, 0, 'f', 1);
        QRect textRect = p.boundingRect(rect, alignment, text); //Get the right rect with given position rect and alignment
        p.drawText(textRect, Qt::AlignCenter, text);

        p.restore(); //Back to state=3
      }
      p.restore(); //Back to state=2
    }
    p.restore(); //Back to state=1
    QFont font = QFont();
    font.setBold(true);
    font.setUnderline(true);
    p.setFont(font);
    p.drawText(-50, 20, tr("Left Power Phases"));
    p.drawText(150, 20, tr("Right Power Phases"));
    //p.restore(); //Back to state=0
  }
  labelGraphics->setPixmap(QPixmap::fromImage(image)); //Assign the img to the GUI
}

void CFitDataDialog::getDeviceName(const CFitData::lap_t &lap, QString& deviceStr) {

  if (manufacturers.contains(lap.manufacturer)) {
    QString manufacturerStr = manufacturers[lap.manufacturer];

    QString productStr = QString(tr("unknown"));
    for (struct product_t product : products) {
      if (product.manufacturer == lap.manufacturer && product.product == lap.product) {
        productStr = product.productStr;
      }
      deviceStr = QString("%1 %2").arg(manufacturerStr, productStr);
    }
  } else {
      deviceStr = QString(tr("Manfacturer and product unknown"));
    return;
  }
}

void CFitDataDialog::slotOk(bool) {
  SETTINGS;
  cfg.beginGroup("FitData");
  cfg.setValue("shownTableCols", QVariant::fromValue(shownTableCols));
  cfg.setValue("shownMostImportantValues", QVariant::fromValue(shownMivs));
  cfg.endGroup();
  accept();
}

void CFitDataDialog::slotCancel(bool) {
  reject();
}

void CFitDataDialog::slotDeleteFitDataFromTrack(bool) {
  qint32 ret = QMessageBox::question(CMainWindow::getBestWidgetForParent()
                                     , tr("Deletes FIT Data from Track")
                                     , "<h4>" + tr("Do you really want to delete all FIT data from the track and close this dialog?") + "</h4>"
                                     , QMessageBox::No | QMessageBox::Yes
                                     , QMessageBox::No);

  if (ret == QMessageBox::Yes)
  {
    trk.getFitData().clear(trk);
    reject();
  }
}

void CFitDataDialog::slotSettingsDialog(bool) {
  CFitDataSettingsDialog dialog = CFitDataSettingsDialog(this, columns, shownTableCols, shownMivs, maxMivs);
  qint32 ret = dialog.exec();
  if (ret == QDialog::Accepted) {
    SETTINGS;
    cfg.beginGroup("FitData");
    connectionDbName = cfg.value("curDbName", "").toString();
    cfg.endGroup();
    checkDbAccess();
    updateData(trk.getFitData().getLaps());
  }
}

void CFitDataDialog::slotItemDoubleClicked(QTreeWidgetItem* item, qint32 column) {
  qint32 treeCol = item->data(column, Qt::UserRole).toInt();
  if (treeCol != eColComment || checkShowSessionsDb->checkState() == Qt::Checked) { //Only on comment in lap view
    return;
  }

  bool ok;
  QString curComment = item->text(column);

  qint32 treeRow = item->data(eColComment, Qt::UserRole + 1).toInt();
  CFitData::lap_t &lap = trk.getFitData().getLap(treeRow);

  QString str = tr("Comment for") + " ";
  if (lap.type == CFitData::eTypeLap) {
    str += tr("lap");
  }
  else if (lap.type == CFitData::eTypeSession) {
    str += tr("session");
  }
  str += QString(" %1").arg(trk.getFitData().getLapNo(treeRow));

  QString newComment = QInputDialog::getText(this, tr("Edit comment"),
                                             str, QLineEdit::Normal, curComment, &ok);

  if (ok && newComment != curComment) {
    item->setText(column, newComment);
    trk.getFitData().setLapComment(treeRow, newComment);
    treeTable->header()->resizeSections(QHeaderView::ResizeToContents);
  }
}

void CFitDataDialog::slotCurrentItemChanged(QTreeWidgetItem* currentItem, QTreeWidgetItem* ) {
  //if (checkShowSessionsDb->checkState() == Qt::Checked //Session DB on
  //    && treeTable->topLevelItemCount() //There are items in view
  //    && nullptr != treeTable->currentItem()) { //And a item is selected
  //  pushRemoveFromDb->setEnabled(true); //Enable remove button
  //}
  updateDataMivs();
  enableButtons();
}

void CFitDataDialog::slotShowSessionsDb(bool checked) {
  if (checked) {
    QSqlDatabase db = QSqlDatabase::database(connectionDbName);
    QSqlQuery query(db);
    query.prepare("SELECT lap FROM fitdata ORDER BY starttime DESC");
    QUERY_EXEC(return);

    QByteArray qbaIn;
    laps.clear();
    while (query.next()) {
      qbaIn = query.value(0).toByteArray();
      qbaIn = qUncompress(qbaIn);
      QDataStream in(&qbaIn, QIODevice::ReadOnly);
      in.setByteOrder(QDataStream::LittleEndian);
      in.setVersion(QDataStream::Qt_5_2);
      CFitData::lap_t lap;
      in >> lap;
      laps << lap;
    }
    updateData(laps);
  } else {
    laps.clear();
    laps = trk.getFitData().getLaps();
    updateData(laps);
  }
  enableButtons();
}

void CFitDataDialog::slotAddSessionToDb() {
  QSqlDatabase db = QSqlDatabase::database(connectionDbName);
  QSqlQuery query(db);
  QByteArray qbaOut;
  QDataStream out(&qbaOut, QIODeviceBase::WriteOnly);
  out.setByteOrder(QDataStream::LittleEndian);
  out.setVersion(QDataStream::Qt_5_2);
  const CFitData::lap_t& lap = trk.getFitData().getLap(trk.getFitData().getNoOfLaps() - 1);
  out << lap;
  qbaOut = qCompress(qbaOut, 9);

  query.prepare("INSERT INTO fitdata (starttime, lap) VALUES (:starttime, :lap)");
  query.bindValue(":starttime", lap.startTime);
  query.bindValue(":lap", qbaOut);
  QUERY_EXEC(return);
  slotShowSessionsDb(true); //Load and show the sessions from db
}

void CFitDataDialog::slotDeleteSessionFromDb() {
  qint32 ret = QMessageBox::question(CMainWindow::getBestWidgetForParent()
                                     , tr("Delete Session")
                                     , "<h4>" + tr("Do you really want to delete the selected session from the database?") + "</h4>"
                                     , QMessageBox::No | QMessageBox::Yes
                                     , QMessageBox::No);

  if (ret == QMessageBox::Yes)
  {
    QTreeWidgetItem* item = treeTable->currentItem();
    QDateTime startTime = item->data(0, Qt::UserRole + 2).toDateTime();

    QSqlDatabase db = QSqlDatabase::database(connectionDbName);
    QSqlQuery query(db);
    query.prepare("DELETE FROM fitdata WHERE starttime=:starttime");
    query.bindValue(":starttime", startTime);
    QUERY_EXEC(return);
    slotShowSessionsDb(true); //Load and show the sessions from db
  }
}

void CFitDataDialog::slotShowTrkptInfo(bool checked) {
  trk.getFitData().setIsTrkptInfo(checked);
  if(checked) {
    trk.getFitData().setTrkPtDesc(trk);
  } else {
    trk.getFitData().delTrkPtDesc(trk);
  }
}

void CFitDataDialog::slotShowHelp() {
  QString msg = tr("<p><b>FIT Data</b></p>"
      "<p>Links to specific values</p>"
      "<p><a href=\"https://www.trainingpeaks.com/learn/articles/normalized-power-intensity-factor-training-stress/\">"
      "https://www.trainingpeaks.com/learn/articles/normalized-power-intensity-factor-training-stress/</a></p>"
      "<p><a href=\"https://support.garmin.com/de-DE/?faq=9EOIDzMcjx7kFqTJUGqHj5\">"
      "https://support.garmin.com/de-DE/?faq=9EOIDzMcjx7kFqTJUGqHj5/ (German)</a></p>"
      );

  QMessageBox::information(CMainWindow::getBestWidgetForParent(), tr("Help"), msg);
}

/*
void CFitDataDialog::slotSave2Csv(bool) {
    SETTINGS;
    cfg.beginGroup("FitData");
    QString path = cfg.value("csvPath", QDir::homePath()).toString();
    QString filename = QFileDialog::getSaveFileName(CMainWindow::getBestWidgetForParent()
                            , tr("Select CSV output file"), path
                            , tr("csv output file (*.csv)"));

 QFile file(filename);
 if(file.open(QIODevice::WriteOnly))
 {
     QTextStream stream(&file);
     QStringList strList;

     // Put header labels into stream
     QMapIterator<columns_t, struct columnLabel_t> column(columns);
     while (column.hasNext())
     {
         column.next();
         strList << column.value().label;
     }
     stream << strList.join(";") + "\n"; // Separeted by semicolon!

     // Put values into stream
     for (const CFitData::lap_t& lap : trk.getFitData().getLaps())
     {
         strList.clear();
         strList << QString("%L1").arg(lap.type)
             << QString("%L1").arg(lap.no)
             << QString("\"%1\"").arg(lap.comment)
             << QString("%L1").arg(lap.elapsedTime)
             << QString("%L1").arg(lap.timerTime)
             << QString("%L1").arg(lap.elapsedTime - lap.timerTime)
             << QString("%L1").arg(lap.distance)
             << QString("%L1").arg(lap.avgSpeed / 1000., 0, 'f', 3)
             << QString("%L1").arg(lap.maxSpeed / 1000., 0, 'f', 3)
             << QString("%L1").arg(lap.ascent)
             << QString("%L1").arg(lap.descent)
             << QString("%L1").arg(lap.avgHr)
             << QString("%L1").arg(lap.maxHr)
             << QString("%L1").arg(lap.avgCad)
             << QString("%L1").arg(lap.maxCad)
             << QString("%L1").arg(lap.avgPower)
             << QString("%L1").arg(lap.maxPower)
             << QString("%L1").arg(lap.normPower)
             //<< QString("%L1").arg(lap.rightBalance, 0, 'f', 2)
             //<< QString("%L1").arg(lap.leftRightBalance, 0, 'f', 2)
             << QString("%L1").arg(lap.leftPedalSmooth)
             << QString("%L1").arg(lap.rightPedalSmooth)
             << QString("%L1").arg(lap.leftTorqueEff)
             << QString("%L1").arg(lap.rightTorqueEff)
             //<< QString("%L1").arg(lap.intensityFactor, 0, 'f', 2)
             //<< QString("%L1").arg(lap.intensityFactor)
             //<< QString("%L1").arg(lap.trainStressScore, 0, 'f', 2)
             //<< QString("%L1").arg(lap.trainStressScore)
             << QString("%L1").arg(lap.work / 1000)
             << QString("%L1").arg(lap.energy);
          stream << strList.join(";") + "\n"; // Separeted by semicolon!
     }
     file.close();
 }
 path = QFileInfo(filename).absolutePath();
 cfg.setValue("csvPath", path);
 cfg.endGroup();
}
*/
