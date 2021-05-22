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
#include "gis/trk/CFitDataDialog.h"
#include "helpers/CSettings.h"

/** @brief Constructor - Initiate the dialog GUI

   @param energyCycling Reference to the track's CEnergyCycling object
   @param parent Pointer to the parent widget
 */
CFitDataDialog::CFitDataDialog(CTrackData::fitdata_t &fitdata, QWidget *parent) :
    QDialog(parent)
    , fitdata(fitdata)
{
    setupUi(this);

    widgetHeaderCb->hide();

    quint16 product = fitdata.getProduct();
    QString prefix(tr("FIT data from device:"));
    QString labelTxt = productName.contains(product) ?
                QString("%1 (%2) %3").arg(prefix).arg(product).arg(productName[product]) :
                QString("%1 (%2) %3").arg(prefix).arg(product).arg(tr("Unknown device"));
    label->setText(labelTxt);

    buttonBox->button(QDialogButtonBox::Reset)->setText(tr("Remove"));
    buttonBox->button(QDialogButtonBox::RestoreDefaults)->setText(tr("Hide/show columns"));
    buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save to csv"));

    buttonBox->button(QDialogButtonBox::Reset)->setToolTip(tr("Remove the FIT data from the track and close the dialog."));

    connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &CFitDataDialog::slotReset);
    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &CFitDataDialog::slotButtonColumns);
    connect(buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &CFitDataDialog::slotSave2Csv);
    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &CFitDataDialog::accept);
    connect(pushHelp, &QPushButton::clicked, this, &CFitDataDialog::slotShowHelp);

    QTreeWidgetItem *item = new QTreeWidgetItem();
    QMapIterator<columns_t, struct columnLabel_t> col(columns);
    while (col.hasNext())
    {
        col.next();
        item->setText(col.key(), col.value().label);
        item->setTextAlignment(col.key(), col.value().alignment);
    }
    treeTable->setHeaderItem(item);

    QList<QTreeWidgetItem*> items;
    for(struct CTrackData::fitdata_t::lap_t lap : fitdata.getLaps())
    {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        QFont font = QFont();
        if (lap.type == CTrackData::fitdata_t::eTypeLap)
        {
            item->setText(eColType, tr("Lap"));
        }
        else if (lap.type == CTrackData::fitdata_t::eTypeSession)
        {
            item->setText(eColType, tr("Session"));
            font.setBold(true);
        }
        item->setFont(eColType, font);
        item->setTextAlignment(eColType, columns[eColType].alignment);

        item->setText(eColIndex, QString("%1").arg(lap.index));
        item->setTextAlignment(eColIndex, columns[eColIndex].alignment);

        QString val, unit;
        IUnit::self().seconds2time(lap.elapsedTime, val, unit);
        item->setText(eColElapsedTime, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColElapsedTime, Qt::AlignRight);

        IUnit::self().seconds2time(lap.timerTime, val, unit);
        item->setText(eColTimerTime, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColTimerTime, Qt::AlignRight);

        IUnit::self().seconds2time(lap.elapsedTime - lap.timerTime, val, unit);
        item->setText(eColPause, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColPause, Qt::AlignRight);

        IUnit::self().meter2distance(lap.distance, val, unit);
        item->setText(eColDistance, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColDistance, Qt::AlignRight);

        IUnit::self().meter2speed(lap.avgSpeed / 1000., val, unit);
        item->setText(eColAvgSpeed, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColAvgSpeed, Qt::AlignRight);

        IUnit::self().meter2speed(lap.maxSpeed / 1000., val, unit);
        item->setText(eColMaxSpeed, QString("%L1%2").arg(val).arg(unit));
        item->setTextAlignment(eColMaxSpeed, Qt::AlignRight);

        IUnit::self().meter2elevation(lap.ascent, val, unit);
        item->setText(eColAscent, QString("%L1%2").arg(val).arg(unit));
        item->setTextAlignment(eColAscent, Qt::AlignRight);

        IUnit::self().meter2elevation(lap.descent, val, unit);
        item->setText(eColDescent, QString("%L1%2").arg(val).arg(unit));
        item->setTextAlignment(eColDescent, Qt::AlignRight);

        item->setText(eColAvgHr, QString("%L1%2").arg(lap.avgHr).arg(tr("bpm")));
        item->setTextAlignment(eColAvgHr, Qt::AlignRight);
        item->setText(eColMaxHr, QString("%L1%2").arg(lap.maxHr).arg(tr("bpm")));
        item->setTextAlignment(eColMaxHr, Qt::AlignRight);

        item->setText(eColAvgCad, QString("%L1%2").arg(lap.avgCad).arg(tr("rpm")));
        item->setTextAlignment(eColAvgCad, Qt::AlignRight);
        item->setText(eColMaxCad, QString("%L1%2").arg(lap.maxCad).arg(tr("rpm")));
        item->setTextAlignment(eColMaxCad, Qt::AlignRight);

        item->setText(eColAvgPower, QString("%L1%2").arg(lap.avgPower).arg("Watt"));
        item->setTextAlignment(eColAvgPower, Qt::AlignRight);
        item->setText(eColMaxPower, QString("%L1%2").arg(lap.maxPower).arg("Watt"));
        item->setTextAlignment(eColMaxPower, Qt::AlignRight);
        item->setText(eColNormPower, QString("%L1%2").arg(lap.normPower).arg("Watt"));
        item->setTextAlignment(eColNormPower, Qt::AlignRight);

        item->setText(eColLeftBalance, QString("%L1%").arg(lap.leftBalance, 0, 'f', 2));
        item->setTextAlignment(eColLeftBalance, Qt::AlignRight);
        item->setText(eColRightBalance, QString("%L1%").arg(lap.rightBalance, 0, 'f', 2));
        item->setTextAlignment(eColRightBalance, Qt::AlignRight);

        item->setText(eColLeftPedalSmooth, QString("%L1%").arg(lap.leftPedalSmooth));
        item->setTextAlignment(eColLeftPedalSmooth, Qt::AlignRight);
        item->setText(eColRightPedalSmooth, QString("%L1%").arg(lap.rightPedalSmooth));
        item->setTextAlignment(eColRightPedalSmooth, Qt::AlignRight);

        item->setText(eColLeftTorqueEff, QString("%L1%").arg(lap.leftTorqueEff));
        item->setTextAlignment(eColLeftTorqueEff, Qt::AlignRight);
        item->setText(eColRightTorqueEff, QString("%L1%").arg(lap.rightTorqueEff));
        item->setTextAlignment(eColRightTorqueEff, Qt::AlignRight);

        item->setText(eColTrainStress, lap.trainStress ? QString("%L1").arg(lap.trainStress, 0, 'f', 2) : "-");
        item->setTextAlignment(eColTrainStress, Qt::AlignRight);
        item->setText(eColIntensity, lap.intensity ? QString("%L1").arg(lap.intensity, 0, 'f', 2) : "-");
        item->setTextAlignment(eColIntensity, Qt::AlignRight);

        item->setText(eColWork, QString("%L1%2").arg(lap.work / 1000).arg("kJ"));
        item->setTextAlignment(eColWork, Qt::AlignRight);
        item->setText(eColEnergy, QString("%L1%2").arg(lap.energy).arg("kcal"));
        item->setTextAlignment(eColEnergy, Qt::AlignRight);

        items << item;
    }
    treeTable->clear();
    treeTable->addTopLevelItems(items);
    treeTable->header()->resizeSections(QHeaderView::ResizeToContents);

    SETTINGS;
    cfg.beginGroup("FitData");
    checkstates = cfg.value("checkstates", 0xFFFFFFFF).toUInt(); // for max 32 columns
    cfg.endGroup();

    const quint8 noOfGridCols = 5;
    for (qint32 i = 0; i < eColMax; ++i)
    {
        quint8 row = i / noOfGridCols;
        quint8 col = i - row * noOfGridCols;

        QCheckBox *checkbox = new QCheckBox(columns[(columns_t)i].label, this);
        checkbox->setProperty("index", i);

        qDebug() << "checkstates=" << checkstates;
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

        QStringList labels;
        QMapIterator<columns_t, struct columnLabel_t> col(columns);
        while (col.hasNext())
        {
            col.next();
            labels << col.value().label;
        }
        stream << labels.join(";") + "\n";

        QStringList strList;
        for (const struct CTrackData::fitdata_t::lap_t &lap : fitdata.getLaps())
        {
            strList.clear();
            strList << QString("%L1").arg(lap.type) << QString("%L1").arg(lap.index)
                    << QString("%L1").arg(lap.elapsedTime) << QString("%L1").arg(lap.timerTime)
                    << QString("%L1").arg(lap.elapsedTime - lap.timerTime)
                    << QString("%L1").arg(lap.distance) << QString("%L1").arg(lap.avgSpeed / 1000., 0, 'f', 3)
                    << QString("%L1").arg(lap.maxSpeed / 1000., 0, 'f', 3) << QString("%L1").arg(lap.avgHr)
                    << QString("%L1").arg(lap.maxHr) << QString("%L1").arg(lap.avgCad)
                    << QString("%L1").arg(lap.maxCad) << QString("%L1").arg(lap.ascent)
                    << QString("%L1").arg(lap.descent) << QString("%L1").arg(lap.avgPower)
                    << QString("%L1").arg(lap.maxPower) << QString("%L1").arg(lap.normPower)
                    << QString("%L1").arg(lap.rightBalance, 0, 'f', 2) << QString("%L1").arg(lap.leftBalance, 0, 'f', 2)
                    << QString("%L1").arg(lap.leftPedalSmooth) << QString("%L1").arg(lap.rightPedalSmooth)
                    << QString("%L1").arg(lap.leftTorqueEff) << QString("%L1").arg(lap.rightTorqueEff)
                    << QString("%L1").arg(lap.intensity, 0, 'f', 2) << QString("%L1").arg(lap.trainStress, 0, 'f', 2)
                    << QString("%L1").arg(lap.work / 1000) << QString("%L1").arg(lap.energy);

            stream << strList.join(";") + "\n";
        }
        file.close();
    }
    path = QFileInfo(filename).absolutePath();
    cfg.setValue("csvPath", path);
    cfg.endGroup();

}

void CFitDataDialog::slotShowHelp()
{
    QString msg = tr("<p><b>Set Energy Use for Cycling</b></p>"
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
