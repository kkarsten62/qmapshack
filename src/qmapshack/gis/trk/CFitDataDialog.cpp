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
                                                                    , trk(trk) {
  setupUi(this);

          //Show product name in GUI label
  quint16 product = trk.getFitData().getProduct();
  QString prefix(tr("FIT Data from Device:"));
  QString labelTxt = productName.contains(product) ?
                         QString("%1 (%2) %3").arg(prefix).arg(product).arg(productName[product]) :
                         QString("%1 (%2) %3").arg(prefix).arg(product).arg(tr("Unknown device"));
  labelProductName->setText(labelTxt);

  checkShowTrkptInfo->setChecked(trk.getFitData().getIsTrkptInfo());

  buttonBox->button(QDialogButtonBox::Reset)->setText(tr("Remove"));
  buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save Data to .csv File"));
  QPushButton* buttonAddSessionToDb = buttonBox->addButton(tr("Add Session to DB"), QDialogButtonBox::ActionRole); //Add a button to add current session to DB
  QPushButton* buttonToogleView = buttonBox->addButton(tr("Show Sessions DB"), QDialogButtonBox::ActionRole); //Add a button to toggle beetwen lap and sessions view
  QPushButton* buttonSettingsDialog = buttonBox->addButton(tr("Settings..."), QDialogButtonBox::ActionRole); //Show Settings Dialog

  buttonBox->button(QDialogButtonBox::Reset)->setToolTip(tr("Remove the Fit data from the track and close the dialog."));

  connect(checkShowTrkptInfo, &QCheckBox::clicked, this, &CFitDataDialog::slotShowTrkptInfo);
  connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &CFitDataDialog::slotReset);
  connect(buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &CFitDataDialog::slotSave2Csv);
  connect(buttonToogleView, &QPushButton::clicked, this, &CFitDataDialog::slotToogleView);
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
  cfg.endGroup();

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

  updateData();
}

CFitDataDialog::~CFitDataDialog() {
}

void CFitDataDialog::updateData() {
  //Add Header labels to treeTable for laps and session values
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
  for(CFitData::lap_t& lap : trk.getFitData().getLaps()) { //For all laps/session
    treeCol = 0;
    QTreeWidgetItem *item = new QTreeWidgetItem();
    for (qint32 shownTableCol : shownTableCols) { //For all shown tree columns
      QString cellStr;
      getCellStr(lap, shownTableCol, cellStr);
      item->setText(treeCol, cellStr);
      item->setTextAlignment(treeCol, columns[shownTableCol].alignment);
      item->setData(treeCol, Qt::UserRole, shownTableCol);
      item->setData(treeCol, Qt::UserRole + 1, treeRow);
      if (shownTableCol == eColComment) {
        item->setToolTip(treeCol, tr("Double click to edit comment"));
      }
      ++treeCol;
    }
    if (lap.type == CFitData::eTypeSession) { //Set text to bold for session row
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
  qint32 treeRow = curItem->data(0, Qt::UserRole +1).toInt();

  CFitData::lap_t& lap = trk.getFitData().getLap(treeRow);

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
  paintGraphics();
}

QString CFitDataDialog::getPowerPhaseStr(const QList<qreal>& powerPhases, qint32 phase) {
  QString cellStr = "0,0,0,0";
  if (powerPhases.count() == 16) {
    QStringList strList;
    for (qint32 i = phase * 4; i < phase * 4 + 4; ++i) {
      strList << QString("%L1").arg(powerPhases[i]);
    }
    cellStr = strList.join(',');
  }
  return cellStr;
}

void CFitDataDialog::getCellStr(const CFitData::lap_t& lap, qint32 column, QString& cellStr) {
  QString val, unit;
  cellStr = "";
  switch (column) {
    case eColNo:
      cellStr = QString("%1").arg(lap.no + 1);
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
      val = IUnit::self().datetime2string(lap.startTime, IUnit::eTimeFormatShortWithSecs);
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
      cellStr = QString("%L1mm").arg(lap.leftPco);
      break;
    case eColRightPco:
      cellStr = QString("%L1mm").arg(lap.rightPco);
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
        cellStr = QString("%L1").arg(lap.functionalThresholdPower).arg(tr("Watt"));
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

void CFitDataDialog::paintGraphics() {
  QSize size = labelGraphics->size();
  QImage image(size.width(), size.height(), QImage::Format_ARGB32);
  image.fill(Qt::lightGray);
  QPainter p;
  p.begin(&image);
  USE_ANTI_ALIASING(p, true);

  p.save(); //Save to initial state=0
  p.translate(20, 0); //Move to left border
  p.save(); //Save to state=1
  p.translate(0, 90); //Move down to center of pedal

  for (qint32 i = 0; i < 2; ++i) {
    if (i == 1) { //Print right pedal moved and mirrored
      p.save(); //Save to state=2
      p.translate(270, 0); //Move to right edge of right pedal
      p.scale(-1, 1); //Mirrored by y-axis
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
    p.drawLine(30 + 2 * 10, -40, 30 +2 * 10, 40); //PCO line
  }
  p.restore(); //Back to state=2
  p.setPen(QPen(QColor(Qt::black)));
  p.drawText(0, 60, "Balance: 50%");
  p.drawText(0, 80, "Pedal Smoothness: 50%");
  p.drawText(0, 100, "Torque Efficiency: 50%");
  p.save(); //Save to a next state=3
  p.translate(190, 0); //Move to left edge of right pedal
  p.drawText(0, 60, "Balance: 50%");
  p.drawText(0, 80, "Pedal Smoothness: 50%");
  p.drawText(0, 100, "Torque Efficiency: 50%");
  p.restore(); //Back to state=2
  p.drawText(30 + 2 * 10, -45, "PCO: 10mm");
  p.drawText(240 - 2 * 10, -45, "PCO: 10mm");

  p.restore(); //Back to state=1
  QFont font = QFont();
  font.setBold(true);
  font.setUnderline(true);
  p.setFont(font);
  p.drawText(0, 20, tr("Left Pedal"));
  p.drawText(190, 20, tr("Right Pedal"));
  p.restore(); //Back to initial state=0

          //Power Phases
  p.translate(450, 100);
  p.rotate(-90);
  for (qint32 i = 0; i < 2; ++i) {
    if (i == 1) { //Print right pedal moved and mirrored
      p.translate(0, 200); //Due to rotation x and y are swapped
    }
    p.setBrush(QColor(Qt::darkBlue));
    p.drawPie(-70, -70, 140, 140, -45 * 16, -90 * 16);
    p.setBrush(QColor(Qt::darkGreen));
    p.drawPie(-60, -60, 120, 120, -350 * 16, -200 * 16);
    p.setBrush(QColor(Qt::darkGray));
    p.drawEllipse(-50, -50, 100, 100);

    p.save(); //Save to state=1
    p.rotate(45);
    p.setPen(QPen(Qt::DashDotDotLine));
    p.drawLine(0, 0, 75, 0);
    p.drawText(75, 0, "45°");
    p.restore(); //Back to state=1
    p.save(); //Save to state=1
    p.rotate(135);
    p.setPen(QPen(Qt::DashDotDotLine));
    p.drawLine(0, 0, 75, 0);
    p.drawText(75, 0, "135°");
    p.restore(); //Back to state=1
    p.save(); //Save to state=1
    p.rotate(350);
    p.setPen(QPen(Qt::DashDotDotLine));
    p.drawLine(0, 0, 65, 0);
    p.drawText(65, 0, "350°");
    p.restore(); //Back to state=1
    p.save(); //Save to state=1
    p.rotate(190);
    p.setPen(QPen(Qt::DashDotDotLine));
    p.drawLine(0, 0, 65, 0);
    p.drawText(65, 0, "190°");
    p.restore(); //Back to state=1
    //p.translate(75, 0);
    //p.save(); //Save to state=2
    //QRectF textRect(0,0,50,50);
    //p.rotate(-190 + 90);
    //p.setOpacity(0.7); // Some opacity to see a bit the underlaying map
    //p.fillRect(p.boundingRect(textRect, Qt::AlignCenter, "190°"), Qt::white); // Fill text box with a white rect
    //p.setOpacity(1);
    //p.drawText(textRect, Qt::AlignCenter, "190°");
    //p.restore(); //Back to state=2
  }
  labelGraphics->setPixmap(QPixmap::fromImage(image)); // Assign the img to the GUI
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

void CFitDataDialog::slotReset(bool) {
  qint32 ret = QMessageBox::question(CMainWindow::getBestWidgetForParent()
                                     , tr("Remove the FIT data from the track and close the dialog.")
                                     , "<h3>" + tr("Do you really want to remove all FIT data from the track and close this dialog?") + "</h3>"
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
    updateData();
  }
}

void CFitDataDialog::slotToogleView(bool) {
  qDebug() << "Hallo Session DB";
  QStringList list = QSqlDatabase::connectionNames();
  //if (QSqlDatabase::contains("karlkarsten_qms")) {
  //qDebug() << "connectionName found!";
  QSqlDatabase db = QSqlDatabase::database("karlkarsten_qms_local");
  if (db.isValid()) {
    qDebug() << "connection is valid!";
  } else {
    qDebug() << "connection is NOT valid!";
  }
  //}
  QSqlQuery query(db);
  query.prepare("SELECT * FROM fitDataSessions ");
  //query.bindValue(":id", id);
  QUERY_EXEC(return);
  while (query.next()) {
    qint32 id = query.value(0).toInt();
    QDateTime startTime = query.value(1).toDateTime();
    qreal distance = query.value(2).toReal();
  }
}

void CFitDataDialog::slotSave2Csv(bool) {
  /*
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
*/
}

void CFitDataDialog::slotItemDoubleClicked(QTreeWidgetItem* item, qint32 column) {
  qint32 treeCol = item->data(column, Qt::UserRole).toInt();
  if (treeCol != eColComment) {
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
  updateDataMivs();
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
