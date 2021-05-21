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
CFitDataDialog::CFitDataDialog(QList<struct CTrackData::fitdata_t> &fitdatas, QWidget *parent) :
    QDialog(parent)
    , fitdatas(fitdatas)
{
    setupUi(this);

    widgetHeaderCb->hide();

    buttonBox->button(QDialogButtonBox::Reset)->setText(tr("Remove"));
    buttonBox->button(QDialogButtonBox::RestoreDefaults)->setText(tr("Hide/show columns"));
    buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save to csv"));

    buttonBox->button(QDialogButtonBox::Reset)->setToolTip(tr("Remove the FIT data from the track and close the dialog."));

    connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &CFitDataDialog::slotReset);
    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &CFitDataDialog::slotRestoreDefaults);
    connect(buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &CFitDataDialog::slotSave2Csv);
    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &CFitDataDialog::accept);
    connect(pushHelp, &QPushButton::clicked, this, &CFitDataDialog::slotShowHelp);

    labels << tr("Type");
    labels << "#";
    labels << tr("Elaps. Time");
    labels << tr("Timer Time");
    labels << tr("Pause");
    labels << tr("Distance");
    labels << tr("Avg. Speed");
    labels << tr("Max. Speed");
    labels << tr("Ascent");
    labels << tr("Desent");
    labels << tr("Avg. HR");
    labels << tr("Max. HR");
    labels << tr("Avg. Cad.");
    labels << tr("Max. Cad.");
    labels << tr("Avg. Power");
    labels << tr("Max. Power");
    labels << tr("Norm. Power");
    labels << tr("Left Balance");
    labels << tr("Right Balance");
    labels << tr("Left Pedal Smooth.");
    labels << tr("Right Pedal Smooth.");
    labels << tr("Left Torque Eff.");
    labels << tr("Right Torque Eff.");
    labels << tr("Training Stress");
    labels << tr("Intensity");
    labels << tr("Work");
    labels << tr("Energy Use");
    treeTable->setHeaderLabels(labels);

    QList<QTreeWidgetItem*> items;
    for(struct CTrackData::fitdata_t fitdata : fitdatas)
    {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        QFont font = QFont();
        if (fitdata.type == CTrackData::fitdata_t::eLap)
        {
            item->setText(eColType, tr("Lap"));
        }
        else if (fitdata.type == CTrackData::fitdata_t::eSession)
        {
            item->setText(eColType, tr("Session"));
            font.setBold(true);
        }
        item->setFont(eColType, font);
        item->setTextAlignment(eColType, Qt::AlignLeft);

        item->setText(eColIndex, QString("%1").arg(fitdata.index));
        item->setTextAlignment(eColIndex, Qt::AlignRight);

        QString val, unit;
        IUnit::self().seconds2time(fitdata.elapsedTime, val, unit);
        item->setText(eColElapsedTime, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColElapsedTime, Qt::AlignRight);

        IUnit::self().seconds2time(fitdata.timerTime, val, unit);
        item->setText(eColTimerTime, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColTimerTime, Qt::AlignRight);

        IUnit::self().seconds2time(fitdata.elapsedTime - fitdata.timerTime, val, unit);
        item->setText(eColPause, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColPause, Qt::AlignRight);

        IUnit::self().meter2distance(fitdata.distance, val, unit);
        item->setText(eColDistance, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColDistance, Qt::AlignRight);

        IUnit::self().meter2speed(fitdata.avgSpeed / 1000., val, unit);
        item->setText(eColAvgSpeed, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColAvgSpeed, Qt::AlignRight);

        IUnit::self().meter2speed(fitdata.maxSpeed / 1000., val, unit);
        item->setText(eColMaxSpeed, QString("%L1%2").arg(val).arg(unit));
        item->setTextAlignment(eColMaxSpeed, Qt::AlignRight);

        IUnit::self().meter2elevation(fitdata.ascent, val, unit);
        item->setText(eColAscent, QString("%L1%2").arg(val).arg(unit));
        item->setTextAlignment(eColAscent, Qt::AlignRight);

        IUnit::self().meter2elevation(fitdata.descent, val, unit);
        item->setText(eColDescent, QString("%L1%2").arg(val).arg(unit));
        item->setTextAlignment(eColDescent, Qt::AlignRight);

        item->setText(eColAvgHr, QString("%L1%2").arg(fitdata.avgHr).arg(tr("bpm")));
        item->setTextAlignment(eColAvgHr, Qt::AlignRight);
        item->setText(eColMaxHr, QString("%L1%2").arg(fitdata.maxHr).arg(tr("bpm")));
        item->setTextAlignment(eColMaxHr, Qt::AlignRight);

        item->setText(eColAvgCad, QString("%L1%2").arg(fitdata.avgCad).arg(tr("rpm")));
        item->setTextAlignment(eColAvgCad, Qt::AlignRight);
        item->setText(eColMaxCad, QString("%L1%2").arg(fitdata.maxCad).arg(tr("rpm")));
        item->setTextAlignment(eColMaxCad, Qt::AlignRight);

        item->setText(eColAvgPower, QString("%L1%2").arg(fitdata.avgPower).arg("Watt"));
        item->setTextAlignment(eColAvgPower, Qt::AlignRight);
        item->setText(eColMaxPower, QString("%L1%2").arg(fitdata.maxPower).arg("Watt"));
        item->setTextAlignment(eColMaxPower, Qt::AlignRight);
        item->setText(eColNormPower, QString("%L1%2").arg(fitdata.normPower).arg("Watt"));
        item->setTextAlignment(eColNormPower, Qt::AlignRight);

        item->setText(eColLeftBalance, QString("%L1%").arg(fitdata.leftBalance, 0, 'f', 2));
        item->setTextAlignment(eColLeftBalance, Qt::AlignRight);
        item->setText(eColRightBalance, QString("%L1%").arg(fitdata.rightBalance, 0, 'f', 2));
        item->setTextAlignment(eColRightBalance, Qt::AlignRight);

        item->setText(eColLeftPedalSmooth, QString("%L1%").arg(fitdata.leftPedalSmooth));
        item->setTextAlignment(eColLeftPedalSmooth, Qt::AlignRight);
        item->setText(eColRightPedalSmooth, QString("%L1%").arg(fitdata.rightPedalSmooth));
        item->setTextAlignment(eColRightPedalSmooth, Qt::AlignRight);

        item->setText(eColLeftTorqueEff, QString("%L1%").arg(fitdata.leftTorqueEff));
        item->setTextAlignment(eColLeftTorqueEff, Qt::AlignRight);
        item->setText(eColRightTorqueEff, QString("%L1%").arg(fitdata.rightTorqueEff));
        item->setTextAlignment(eColRightTorqueEff, Qt::AlignRight);

        item->setText(eColTrainStress, fitdata.trainStress ? QString("%L1").arg(fitdata.trainStress, 0, 'f', 2) : "-");
        item->setTextAlignment(eColTrainStress, Qt::AlignRight);
        item->setText(eColIntensity, fitdata.intensity ? QString("%L1").arg(fitdata.intensity, 0, 'f', 2) : "-");
        item->setTextAlignment(eColIntensity, Qt::AlignRight);

        item->setText(eColWork, QString("%L1%2").arg(fitdata.work / 1000).arg("kJ"));
        item->setTextAlignment(eColWork, Qt::AlignRight);
        item->setText(eColEnergy, QString("%L1%2").arg(fitdata.energy).arg("kcal"));
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
    for (quint8 index = 0; index < eColMax; ++index)
    {
        quint8 row = index / noOfGridCols;
        quint8 col = index - row * noOfGridCols;

        QCheckBox *checkbox = new QCheckBox(labels.at(index), this);
        checkbox->setProperty("index", index);

        qDebug() << "checkstates=" << checkstates;
        bool checked = (checkstates >> index) & 0x1;
        checkbox->setChecked(checked);

        treeTable->setColumnHidden(index, !checked);

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
        fitdatas.clear();
        reject();
    }
}

void CFitDataDialog::slotRestoreDefaults(bool)
{
    qDebug() << "slotRestoreDefaults";

    qDebug() << "sizeof quint32=" << sizeof(quint32);
    qDebug() << "sizeof qulonglong=" << sizeof(qulonglong);

    bool isEnabled = buttonBox->button(QDialogButtonBox::Reset)->isEnabled();

    widgetHeaderCb->setVisible(isEnabled);
    buttonBox->button(QDialogButtonBox::Reset)->setEnabled(!isEnabled);
    buttonBox->button(QDialogButtonBox::Save)->setEnabled(!isEnabled);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!isEnabled);
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

    qDebug() << "index=" << index;
    qDebug() << "checked=" << checked;
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

        stream << labels.join(";") + "\n";

        QStringList strList;
        for (const struct CTrackData::fitdata_t &f : fitdatas)
        {
            strList.clear();
            strList << QString("%L1").arg(f.type) << QString("%L1").arg(f.index)
                    << QString("%L1").arg(f.elapsedTime) << QString("%L1").arg(f.timerTime)
                    << QString("%L1").arg(f.elapsedTime - f.timerTime)
                    << QString("%L1").arg(f.distance) << QString("%L1").arg(f.avgSpeed / 1000., 0, 'f', 3)
                    << QString("%L1").arg(f.maxSpeed / 1000., 0, 'f', 3) << QString("%L1").arg(f.avgHr)
                    << QString("%L1").arg(f.maxHr) << QString("%L1").arg(f.avgCad)
                    << QString("%L1").arg(f.maxCad) << QString("%L1").arg(f.ascent)
                    << QString("%L1").arg(f.descent) << QString("%L1").arg(f.avgPower)
                    << QString("%L1").arg(f.maxPower) << QString("%L1").arg(f.normPower)
                    << QString("%L1").arg(f.rightBalance, 0, 'f', 2) << QString("%L1").arg(f.leftBalance, 0, 'f', 2)
                    << QString("%L1").arg(f.leftPedalSmooth) << QString("%L1").arg(f.rightPedalSmooth)
                    << QString("%L1").arg(f.leftTorqueEff) << QString("%L1").arg(f.rightTorqueEff)
                    << QString("%L1").arg(f.intensity, 0, 'f', 2) << QString("%L1").arg(f.trainStress, 0, 'f', 2)
                    << QString("%L1").arg(f.work / 1000) << QString("%L1").arg(f.energy);

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
