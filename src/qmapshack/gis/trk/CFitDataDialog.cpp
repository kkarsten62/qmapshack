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
#include "gis/trk/CFitData.h"
#include "gis/trk/CFitDataDialog.h"
#include "helpers/CSettings.h"
#include <QCheckBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

/** @brief Constructor - Initiate the dialog GUI

   @param xxx yyy
   @param xxx yyy
 */
CFitDataDialog::CFitDataDialog(CFitData& fitdata, QWidget* parent) :
    QDialog(parent)
    , fitdata(fitdata)
{
    setupUi(this);

    widgetHeaderCb->hide();

    // Show product name in GUI label
    quint16 product = fitdata.getProduct();
    QString prefix(tr("FIT data from device:"));
    QString labelTxt = productName.contains(product) ?
                QString("%1 (%2) %3").arg(prefix).arg(product).arg(productName[product]) :
                QString("%1 (%2) %3").arg(prefix).arg(product).arg(tr("Unknown device"));
    label->setText(labelTxt);

    checkShowTrkptInfo->setChecked(fitdata.getIsTrkptInfo());

    buttonBox->button(QDialogButtonBox::Reset)->setText(tr("Remove"));
    buttonBox->button(QDialogButtonBox::RestoreDefaults)->setText(tr("Hide/show columns"));
    buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save to csv"));

    buttonBox->button(QDialogButtonBox::Reset)->setToolTip(tr("Remove the FIT data from the track and close the dialog."));

    connect(checkShowTrkptInfo, &QCheckBox::clicked, this, &CFitDataDialog::slotShowTrkptInfo);
    connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &CFitDataDialog::slotReset);
    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &CFitDataDialog::slotButtonColumns);
    connect(buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &CFitDataDialog::slotSave2Csv);
    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &CFitDataDialog::slotOk);
    connect(treeTable, &QTreeWidget::itemDoubleClicked, this, &CFitDataDialog::slotItemDoubleClicked);
    connect(pushHelp, &QPushButton::clicked, this, &CFitDataDialog::slotShowHelp);


    // Add Header labels to treeTable
    QTreeWidgetItem* item = new QTreeWidgetItem();
    QMapIterator<columns_t, struct columnLabel_t> col(columns);
    while (col.hasNext())
    {
        col.next();
        item->setText(col.key(), col.value().label);
        item->setTextAlignment(col.key(), col.value().alignment);
    }
    treeTable->setHeaderItem(item);

    // Add values to treeTable
    qint32 index = 0;
    QList<QTreeWidgetItem*> items;
    for(CFitData::lap_t& lap : fitdata.getLaps())
    {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        if (lap.type == CFitData::eTypeLap)
        {
            item->setText(eColType, tr("Lap"));
            item->setText(eColIndex, QString("%1").arg(lap.no));
        }
        else if (lap.type == CFitData::eTypeSession)
        {
            item->setText(eColType, tr("Session"));
            item->setText(eColIndex, QString("%1").arg(lap.no));
        }
        item->setTextAlignment(eColType, columns[eColType].alignment);
        item->setTextAlignment(eColIndex, columns[eColIndex].alignment);

        item->setText(eColComment, lap.comment);
        item->setToolTip(eColComment, tr("Double click to edit comment"));

        item->setData(eColComment, Qt::UserRole, QVariant(index++));
        item->setTextAlignment(eColComment, columns[eColComment].alignment);

        QString val, unit;
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

        item->setText(eColLeftBalance, QString("%L1%").arg(lap.leftBalance, 0, 'f', 2));
        item->setTextAlignment(eColLeftBalance, columns[eColLeftBalance].alignment);

        item->setText(eColRightBalance, QString("%L1%").arg(lap.rightBalance, 0, 'f', 2));
        item->setTextAlignment(eColRightBalance, columns[eColRightBalance].alignment);

        item->setText(eColLeftPedalSmooth, QString("%L1%").arg(lap.leftPedalSmooth));
        item->setTextAlignment(eColLeftPedalSmooth, columns[eColLeftPedalSmooth].alignment);

        item->setText(eColRightPedalSmooth, QString("%L1%").arg(lap.rightPedalSmooth));
        item->setTextAlignment(eColRightPedalSmooth, columns[eColRightPedalSmooth].alignment);

        item->setText(eColLeftTorqueEff, QString("%L1%").arg(lap.leftTorqueEff));
        item->setTextAlignment(eColLeftTorqueEff, columns[eColLeftTorqueEff].alignment);

        item->setText(eColRightTorqueEff, QString("%L1%").arg(lap.rightTorqueEff));
        item->setTextAlignment(eColRightTorqueEff, columns[eColRightTorqueEff].alignment);

        item->setText(eColTrainStress, lap.trainStress ? QString("%L1").arg(lap.trainStress, 0, 'f', 2) : "-");
        item->setTextAlignment(eColTrainStress, columns[eColTrainStress].alignment);

        item->setText(eColIntensity, lap.intensity ? QString("%L1").arg(lap.intensity, 0, 'f', 2) : "-");
        item->setTextAlignment(eColIntensity, columns[eColIntensity].alignment);

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
    treeTable->header()->resizeSections(QHeaderView::ResizeToContents);

    // Read checkstates from setting file
    SETTINGS;
    cfg.beginGroup("FitData");
    checkstates = cfg.value("checkstates", 0xFFFFFFFF).toUInt(); // for max 32 columns
    cfg.endGroup();

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
}

CFitDataDialog::~CFitDataDialog()
{
}

void CFitDataDialog::slotOk(bool)
{
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
        fitdata.clear();
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
    SETTINGS;
    cfg.beginGroup("FitData");
    cfg.setValue("checkstates", checkstates);
    cfg.endGroup();
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
        for (const CFitData::lap_t& lap : fitdata.getLaps())
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
                << QString("%L1").arg(lap.avgHr)
                << QString("%L1").arg(lap.maxHr)
                << QString("%L1").arg(lap.avgCad)
                << QString("%L1").arg(lap.maxCad)
                << QString("%L1").arg(lap.ascent)
                << QString("%L1").arg(lap.descent)
                << QString("%L1").arg(lap.avgPower)
                << QString("%L1").arg(lap.maxPower)
                << QString("%L1").arg(lap.normPower)
                << QString("%L1").arg(lap.rightBalance, 0, 'f', 2)
                << QString("%L1").arg(lap.leftBalance, 0, 'f', 2)
                << QString("%L1").arg(lap.leftPedalSmooth)
                << QString("%L1").arg(lap.rightPedalSmooth)
                << QString("%L1").arg(lap.leftTorqueEff)
                << QString("%L1").arg(lap.rightTorqueEff)
                << QString("%L1").arg(lap.intensity, 0, 'f', 2)
                << QString("%L1").arg(lap.trainStress, 0, 'f', 2)
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

    CFitData::lap_t &lap = fitdata.getLap(index);

    QString str = tr("Comment for") + " ";
    if (lap.type == CFitData::eTypeLap)
    {
        str += tr("lap");
    }
    else if (lap.type == CFitData::eTypeSession)
    {
        str += tr("session");
    }
    str += QString(" %1").arg(fitdata.getLapNo(index));

    QString comNew = QInputDialog::getText(this, tr("Edit comment"),
                        str, QLineEdit::Normal, comCur, &ok);

    if (ok && comNew != comCur)
    {
        item->setText(eColComment, comNew);
        fitdata.setLapComment(index, comNew);
        isChanged = true;
        treeTable->header()->resizeSections(QHeaderView::ResizeToContents);
    }
}

void CFitDataDialog::slotShowTrkptInfo(bool checked)
{
    fitdata.setIsTrkptInfo(checked);
    if(checked)
    {
        fitdata.setTrkPtDesc();
    }
    else
    {
        fitdata.delTrkPtDesc();
    }
}

void CFitDataDialog::slotShowHelp()
{
    QString msg = tr("<p><b>Header</b></p>"
                     "<p>Within this dialog your personal energy use (consumption) for a cycling tour can be computed.</p>"
                     "<p>The computed value of \"Energy Use Cycling\" can be see as an indicator for the exertion of a cycling tour.</p>"
                     "<p>The tour length, speed and slope values will be taken into account.</p>"
                     "<p>To individualize your personal energy use the following input data are more needed:"
                     "<ul>"
                     "<li>Driver and bicyle weight</li>"
                     "<li>Air density, wind speed and position to the wind to consider the wind drag resistance</li>"
                     "<li>Ground situation (tyre and ground) to consider the rolling resistance</li>"
                     "<li>Average pedal cadence for the computation of pedal force</li>"
                     "</ul></p>"
                     "<p>The individualize data will be defined in this dialog and more computed values will be shown here.</p>"
                     "<p>When loading older tracks or switching in history to tracks with a different parameter set compared to the previous saved parameter set"
                     ", the shown parameter set in this dialog can be replaced by the previous saved parameter set."
                     "<p>The energy use in unit \"kcal\" will be stored in the track (qms format only) and can be remove later on when no longer needed.</p>"
                     "<p>For more information see tooltips on input and output values.</p>");

    QMessageBox::information(CMainWindow::getBestWidgetForParent(), tr("Help"), msg);
}
