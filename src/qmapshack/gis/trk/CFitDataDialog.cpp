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

#include <QCheckBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QPainter>

/** @brief Constructor - Initiate the dialog GUI

   @param xxx yyy
   @param xxx yyy
 */
//CFitDataDialog::CFitDataDialog(CFitData& fitdata, CGisItemTrk& trk, QWidget* parent) :
CFitDataDialog::CFitDataDialog(QWidget* parent, CGisItemTrk& trk) :
    QDialog(parent)
    , trk(trk)
{
    setupUi(this);
    widgetHeaderCb->hide();

    // Show product name in GUI label
    quint16 product = trk.getFitData().getProduct();
    QString prefix(tr("FIT data from device:"));
    QString labelTxt = productName.contains(product) ?
                QString("%1 (%2) %3").arg(prefix).arg(product).arg(productName[product]) :
                QString("%1 (%2) %3").arg(prefix).arg(product).arg(tr("Unknown device"));
    labelProductName->setText(labelTxt);

    checkShowTrkptInfo->setChecked(trk.getFitData().getIsTrkptInfo());

    buttonBox->button(QDialogButtonBox::Reset)->setText(tr("Remove"));
    buttonBox->button(QDialogButtonBox::RestoreDefaults)->setText(tr("Hide/Show Columns"));
    buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save Data to .csv File"));
    QPushButton* buttonAddSessionToDb = buttonBox->addButton(tr("Add Session to DB"), QDialogButtonBox::ActionRole); //Add a button to add current session to DB
    QPushButton* buttonToogleView = buttonBox->addButton(tr("Show Sessions DB"), QDialogButtonBox::ActionRole); //Add a button to toggle beetwen lap and sessions view
    QPushButton* buttonSettingsDialog = buttonBox->addButton(tr("Settings ..."), QDialogButtonBox::ActionRole); //Add a button to toggle beetwen lap and sessions view

    buttonBox->button(QDialogButtonBox::Reset)->setToolTip(tr("Remove the FIT data from the track and close the dialog."));

    connect(checkShowTrkptInfo, &QCheckBox::clicked, this, &CFitDataDialog::slotShowTrkptInfo);
    connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &CFitDataDialog::slotReset);
    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &CFitDataDialog::slotButtonColumns);
    connect(buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &CFitDataDialog::slotSave2Csv);
    connect(buttonToogleView, &QPushButton::clicked, this, &CFitDataDialog::slotToogleView);
    connect(buttonSettingsDialog, &QPushButton::clicked, this, &CFitDataDialog::slotSettingsDialog);
    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &CFitDataDialog::slotOk);
    connect(treeTable, &QTreeWidget::itemDoubleClicked, this, &CFitDataDialog::slotItemDoubleClicked);
    connect(pushHelp, &QPushButton::clicked, this, &CFitDataDialog::slotShowHelp);

    //Set the overall session data, this values are in session only, not in laps
    labelFto->setText(tr("Functional Threshold Power:"));
    labelFtoValue->setText(QString("%L1%L2").arg(trk.getFitData().getFunctionalThresholdPower()).arg("Watt"));
    labelIf->setText(tr("Intensity Factor:"));
    labelIfValue->setText(QString("%L1").arg(trk.getFitData().getIntensityFactor(), 0, 'f', 3));
    labelTss->setText(tr("Training Stress Score:"));
    labelTssValue->setText(QString("%L1").arg(trk.getFitData().getTrainingStressScore(), 0, 'f', 1));

    //Read checkstates from setting file
    SETTINGS;
    cfg.beginGroup("FitData");
    checkstates = cfg.value("checkstates", 0xFFFFFFFF).toUInt(); // for max 32 columns, default all "on"
    shownTableCols = cfg.value("shownTableCols").toList();
    cfg.endGroup();
    for(QVariant& colNo : shownTableCols) { //Convert from string to int
        colNo = colNo.toInt();
    }

    //Add Header labels to treeTable for laps and session values
    QTreeWidgetItem* item = new QTreeWidgetItem();
    qint32 treeCol = 0;
    for (QVariant shownTableCol : shownTableCols) {
      struct columnLabel_t col = columns1.at(shownTableCol.toInt());
      item->setText(treeCol++, col.label);
    }
    treeTable->setHeaderItem(item);

    //Add values to treeTable
    QString val, unit;
    QList<QTreeWidgetItem*> items;
    for(CFitData::lap_t& lap : trk.getFitData().getLaps()) { //For all laps/session
      treeCol = 0;
      QTreeWidgetItem *item = new QTreeWidgetItem();
      for (QVariant shownTableCol : shownTableCols) { //For all shown tree columns
        //struct columnLabel_t col = columns1.at(shownTableCol.toInt());
        //item->setText(treeCol++, col.label);
        //struct columnLabel_t col = columns1.at(shownTableCol.toInt());
        QString cellStr;
        getCellString(lap, shownTableCol.toInt(), cellStr);
        item->setText(treeCol, cellStr);
        /*
        switch (shownTableCol.toInt()) {
          case 0: //#
            item->setText(treeCol, QString("%1").arg(lap.no + 1));
            break;
          case 1: //type
            if (lap.type == CFitData::eTypeLap) {
              item->setText(treeCol, tr("Lap"));
            } else if (lap.type == CFitData::eTypeSession) {
              item->setText(treeCol, tr("Session"));
            }
            break;
          case 2: //comment
            item->setText(treeCol, lap.comment);
            item->setToolTip(treeCol, tr("Double click to edit comment"));
            break;
          case 3: //startTime
            val = IUnit::self().datetime2string(lap.startTime, IUnit::eTimeFormatShortWithSecs);
            item->setText(treeCol, QString("%L1").arg(val));
            break;
          case 4: //elapsedTime
            IUnit::self().seconds2time(lap.elapsedTime, val, unit);
            item->setText(treeCol, QString("%1%2").arg(val).arg(unit));
            break;
          case 5: //timerTime
            IUnit::self().seconds2time(lap.timerTime, val, unit);
            item->setText(treeCol, QString("%1%2").arg(val).arg(unit));
            break;
          case 6: //pause
            IUnit::self().seconds2time(lap.elapsedTime - lap.timerTime, val, unit);
            item->setText(treeCol, QString("%1%2").arg(val).arg(unit));
            break;
          case 7: //distance
            IUnit::self().meter2distance(lap.distance, val, unit);
            item->setText(treeCol, QString("%1%2").arg(val).arg(unit));
            break;
          case 8: //avgSpeed
            IUnit::self().meter2speed(lap.avgSpeed / 1000., val, unit);
            item->setText(treeCol, QString("%1%2").arg(val).arg(unit));
            break;
        }
*/
        item->setTextAlignment(treeCol, columns1.at(treeCol).alignment);
        treeCol++;
      }
    items << item;
    }
    treeTable->clear();
    treeTable->addTopLevelItems(items);

    /*
    QMapIterator<columns_t, struct columnLabel_t> col(columns);
    while (col.hasNext())
    {
        col.next();
        item->setText(col.key(), col.value().label);
        item->setTextAlignment(col.key(), col.value().alignment);
    }
*/
    //treeTable->setHeaderItem(item);
/*
    // Add values to treeTable
    qint32 index = 0;
    QList<QTreeWidgetItem*> items;
    for(CFitData::lap_t& lap : trk.getFitData().getLaps())
    {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        if (lap.type == CFitData::eTypeLap)
        {
            item->setText(eColType, tr("Lap"));
            item->setIcon(eColIndex, QIcon("://icons/32x32/DeleteOne.png"));
            //item->setText(eColIndex, QString("%1").arg(lap.no + 1));
        }
        else if (lap.type == CFitData::eTypeSession)
        {
            item->setText(eColType, tr("Session"));
            item->setText(eColIndex, QString("%1").arg(lap.no + 1));
        }
        item->setTextAlignment(eColType, columns[eColType].alignment);
        item->setTextAlignment(eColIndex, columns[eColIndex].alignment);

        item->setText(eColComment, lap.comment);
        item->setToolTip(eColComment, tr("Double click to edit comment"));

        item->setData(eColComment, Qt::UserRole, QVariant(index++));
        item->setTextAlignment(eColComment, columns[eColComment].alignment);

        QString val, unit;
        val = IUnit::self().datetime2string(lap.startTime, IUnit::eTimeFormatShortWithSecs);
        item->setText(eColStartTime, QString("%L1").arg(val));
        item->setTextAlignment(eColStartTime, columns[eColStartTime].alignment);

        IUnit::self().seconds2time(lap.elapsedTime, val, unit);
        item->setText(eColElapsedTime, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColElapsedTime, columns[eColElapsedTime].alignment);

        IUnit::self().seconds2time(lap.timerTime, val, unit);
        item->setText(eColTimerTime, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColTimerTime, columns[eColTimerTime].alignment);

        IUnit::self().seconds2time(lap.elapsedTime - lap.timerTime, val, unit);
        item->setText(eColPause, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColPause, columns[eColPause].alignment);

        IUnit::self().meter2distance(lap.distance, val, unit);
        item->setText(eColDistance, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColDistance, columns[eColDistance].alignment);

        IUnit::self().meter2speed(lap.avgSpeed / 1000., val, unit);
        item->setText(eColAvgSpeed, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColAvgSpeed, columns[eColAvgSpeed].alignment);

        IUnit::self().meter2speed(lap.maxSpeed / 1000., val, unit);
        item->setText(eColMaxSpeed, QString("%L1%2").arg(val).arg(unit));
        item->setTextAlignment(eColMaxSpeed, columns[eColMaxSpeed].alignment);

        IUnit::self().meter2elevation(lap.ascent, val, unit);
        item->setText(eColAscent, QString("%L1%2").arg(val).arg(unit));
        item->setTextAlignment(eColAscent, columns[eColAscent].alignment);

        IUnit::self().meter2elevation(lap.descent, val, unit);
        item->setText(eColDescent, QString("%L1%2").arg(val).arg(unit));
        item->setTextAlignment(eColDescent, columns[eColDescent].alignment);

        item->setText(eColAvgHr, QString("%L1%2").arg(lap.avgHr).arg(tr("bpm")));
        item->setTextAlignment(eColAvgHr, columns[eColAvgHr].alignment);

        item->setText(eColMaxHr, QString("%L1%2").arg(lap.maxHr).arg(tr("bpm")));
        item->setTextAlignment(eColMaxHr, columns[eColMaxHr].alignment);

        item->setText(eColAvgCad, QString("%L1%2").arg(lap.avgCad).arg(tr("rpm")));
        item->setTextAlignment(eColAvgCad, columns[eColAvgCad].alignment);

        item->setText(eColMaxCad, QString("%L1%2").arg(lap.maxCad).arg(tr("rpm")));
        item->setTextAlignment(eColMaxCad, columns[eColMaxCad].alignment);

        item->setText(eColAvgPower, QString("%L1%2").arg(lap.avgPower).arg("Watt"));
        item->setTextAlignment(eColAvgPower, columns[eColAvgPower].alignment);

        item->setText(eColMaxPower, QString("%L1%2").arg(lap.maxPower).arg("Watt"));
        item->setTextAlignment(eColMaxPower, columns[eColMaxPower].alignment);

        item->setText(eColNormPower, QString("%L1%2").arg(lap.normPower).arg("Watt"));
        item->setTextAlignment(eColNormPower, columns[eColNormPower].alignment);

        qreal rightBalance = 0;
        qreal leftBalance = 0;
        if (lap.leftRightBalance & 0x8000) { //According to FIT type "left_right_balance_100"
          rightBalance = (lap.leftRightBalance & 0x3FFF) / 100.;
          leftBalance = 100. - rightBalance;
        }
        item->setText(eColLeftBalance, QString("%L1%").arg(leftBalance, 0, 'f', 2));
        item->setTextAlignment(eColLeftBalance, columns[eColLeftBalance].alignment);

        item->setText(eColRightBalance, QString("%L1%").arg(rightBalance, 0, 'f', 2));
        item->setTextAlignment(eColRightBalance, columns[eColRightBalance].alignment);

        item->setText(eColLeftPedalSmooth, QString("%L1%").arg(lap.leftPedalSmooth));
        item->setTextAlignment(eColLeftPedalSmooth, columns[eColLeftPedalSmooth].alignment);

        item->setText(eColRightPedalSmooth, QString("%L1%").arg(lap.rightPedalSmooth));
        item->setTextAlignment(eColRightPedalSmooth, columns[eColRightPedalSmooth].alignment);

        item->setText(eColLeftTorqueEff, QString("%L1%").arg(lap.leftTorqueEff));
        item->setTextAlignment(eColLeftTorqueEff, columns[eColLeftTorqueEff].alignment);

        item->setText(eColRightTorqueEff, QString("%L1%").arg(lap.rightTorqueEff));
        item->setTextAlignment(eColRightTorqueEff, columns[eColRightTorqueEff].alignment);

        item->setText(eColWork, QString("%L1%2").arg(lap.work / 1000).arg("kJ"));
        item->setTextAlignment(eColWork, columns[eColWork].alignment);

        item->setText(eColEnergy, QString("%L1%2").arg(lap.energy).arg("kcal"));
        item->setTextAlignment(eColEnergy, columns[eColEnergy].alignment);

        // Set bold to session row
        if (lap.type == CFitData::eTypeSession)
        {
            QFont font = QFont();
            font.setBold(true);
            for (qint32 i = 0; i < item->columnCount();item->setFont(i, font), ++i);
        }

        items << item;
    }
    treeTable->clear();
    treeTable->addTopLevelItems(items);
    */
    treeTable->header()->resizeSections(QHeaderView::ResizeToContents);

    /*
    // Put checkboxes on checkbox widget
    const quint8 noOfGridCols = 5; // Five checkboxes in one row
    for (qint32 i = 0; i < eColMax; ++i)
    {
        quint8 row = i / noOfGridCols;
        quint8 col = i - row * noOfGridCols;

        QCheckBox *checkbox = new QCheckBox(columns[(columns_t)i].label, this);
        checkbox->setProperty("index", i);

        bool checked = (checkstates >> i) & 0x1;
        checkbox->setChecked(checked);

        treeTable->setColumnHidden(i, !checked);

        connect(checkbox, &QCheckBox::clicked, this, &CFitDataDialog::slotCheckColumns);

        gridHeaderCb->addWidget(checkbox, row, col);
    }
*/
  paintGraphics();
}

CFitDataDialog::~CFitDataDialog()
{
}

void CFitDataDialog::getCellString(const CFitData::lap_t& lap, qint32 shownTableCol, QString& cellStr) {
  QString val, unit;
  switch (shownTableCol) {
    case 0: //no
      cellStr = QString("%1").arg(lap.no + 1);
      //item->setText(treeCol, QString("%1").arg(lap.no + 1));
      break;
    case 1: //type
      if (lap.type == CFitData::eTypeLap) {
        cellStr = tr("Lap");
        //item->setText(treeCol, tr("Lap"));
      } else if (lap.type == CFitData::eTypeSession) {
        cellStr = tr("Session");
        //item->setText(treeCol, tr("Session"));
      }
      break;
    case 2: //comment
      //item->setText(treeCol, lap.comment);
      cellStr = lap.comment;
      //item->setToolTip(treeCol, tr("Double click to edit comment"));
      break;
    case 3: //startTime
      val = IUnit::self().datetime2string(lap.startTime, IUnit::eTimeFormatShortWithSecs);
      cellStr = QString("%L1").arg(val);
      //item->setText(treeCol, QString("%L1").arg(val));
      break;
    case 4: //elapsedTime
      IUnit::self().seconds2time(lap.elapsedTime, val, unit);
      cellStr = QString("%1%2").arg(val).arg(unit);
      //item->setText(treeCol, QString("%1%2").arg(val).arg(unit));
      break;
    case 5: //timerTime
      IUnit::self().seconds2time(lap.timerTime, val, unit);
      cellStr = QString("%1%2").arg(val).arg(unit);
      //item->setText(treeCol, QString("%1%2").arg(val).arg(unit));
      break;
    case 6: //pause
      IUnit::self().seconds2time(lap.elapsedTime - lap.timerTime, val, unit);
      cellStr = QString("%1%2").arg(val).arg(unit);
      //item->setText(treeCol, QString("%1%2").arg(val).arg(unit));
      break;
    case 7: //distance
      IUnit::self().meter2distance(lap.distance, val, unit);
      cellStr = QString("%1%2").arg(val).arg(unit);
      //item->setText(treeCol, QString("%1%2").arg(val).arg(unit));
      break;
    case 8: //avgSpeed
      IUnit::self().meter2speed(lap.avgSpeed / 1000., val, unit);
      cellStr = QString("%1%2").arg(val).arg(unit);
      //item->setText(treeCol, QString("%1%2").arg(val).arg(unit));
      break;
  }
}

void CFitDataDialog::slotOk(bool) {
    if (isChanged)
    {
        reject();
    }
    else
    {
        accept();
    }
}

void CFitDataDialog::slotReset(bool)
{
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

void CFitDataDialog::slotButtonColumns(bool)
{
    bool isEnabled = buttonBox->button(QDialogButtonBox::Reset)->isEnabled();

    buttonBox->button(QDialogButtonBox::Reset)->setEnabled(!isEnabled);
    buttonBox->button(QDialogButtonBox::Save)->setEnabled(!isEnabled);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!isEnabled);
    widgetHeaderCb->setVisible(isEnabled);

    treeTable->header()->resizeSections(QHeaderView::ResizeToContents);
}

void CFitDataDialog::slotCheckColumns(bool checked)
{
    QWidget *widget = qApp->focusWidget();
    quint8 index = widget->property("index").toInt();
    treeTable->setColumnHidden(index, !checked);

    checkstates ^= 1 << index; // Toogle bit
    QList<QVariant> list;
    list << 1 << 3 << 4;


    SETTINGS;
    cfg.beginGroup("FitData");
    cfg.setValue("checkstates", checkstates);
    cfg.setValue("list", list);
    cfg.endGroup();
}

void CFitDataDialog::slotSettingsDialog(bool)
{
  CFitDataSettingsDialog fitDataSettingsDialog(this);

  qint32 ret = fitDataSettingsDialog.exec();
}

void CFitDataDialog::slotToogleView(bool)
{
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

void CFitDataDialog::slotSave2Csv(bool)
{
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

void CFitDataDialog::slotItemDoubleClicked(QTreeWidgetItem* item, qint32 column)
{
    if (column != eColComment)
    {
      return;
    }

    bool ok;
    QString comCur = item->text(eColComment);

    qint32 index = item->data(eColComment, Qt::UserRole).toInt();
    CFitData::lap_t &lap = trk.getFitData().getLap(index);

    QString str = tr("Comment for") + " ";
    if (lap.type == CFitData::eTypeLap)
    {
        str += tr("lap");
    }
    else if (lap.type == CFitData::eTypeSession)
    {
        str += tr("session");
    }
    str += QString(" %1").arg(trk.getFitData().getLapNo(index));

    QString comNew = QInputDialog::getText(this, tr("Edit comment"),
                        str, QLineEdit::Normal, comCur, &ok);

    if (ok && comNew != comCur)
    {
        item->setText(eColComment, comNew);
        trk.getFitData().setLapComment(index, comNew);
        isChanged = true;
        treeTable->header()->resizeSections(QHeaderView::ResizeToContents);
    }
}

void CFitDataDialog::slotShowTrkptInfo(bool checked)
{
    trk.getFitData().setIsTrkptInfo(checked);
    if(checked)
    {
        trk.getFitData().setTrkPtDesc(trk);
    }
    else
    {
        trk.getFitData().delTrkPtDesc(trk);
    }
}

void CFitDataDialog::slotShowHelp()
{
    QString msg = tr("<p><b>Show FIT data</b></p>"
                     "<p>Links to specific values</p>"
                     "<p><a href=\"https://www.trainingpeaks.com/learn/articles/normalized-power-intensity-factor-training-stress/\">"
                     "https://www.trainingpeaks.com/learn/articles/normalized-power-intensity-factor-training-stress/</a></p>"
                     "<p><a href=\"https://support.garmin.com/de-DE/?faq=9EOIDzMcjx7kFqTJUGqHj5\">"
                     "https://support.garmin.com/de-DE/?faq=9EOIDzMcjx7kFqTJUGqHj5/ (German)</a></p>"
                     );

    QMessageBox::information(CMainWindow::getBestWidgetForParent(), tr("Help"), msg);
}

void CFitDataDialog::paintGraphics()
{
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
