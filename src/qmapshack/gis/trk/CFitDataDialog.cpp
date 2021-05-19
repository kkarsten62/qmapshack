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
//#include "gis/trk/CGisItemTrk.h"


/** @brief Constructor - Initiate the dialog GUI

   @param energyCycling Reference to the track's CEnergyCycling object
   @param parent Pointer to the parent widget
 */
CFitDataDialog::CFitDataDialog(QList<struct CTrackData::fitdata_t> &fitdatas, QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);

//    treeTable->setColumnCount(5);

    QStringList labels;
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
    labels << tr("Train. Stress");
    labels << tr("Intensity");
    labels << tr("Work");
    labels << tr("KCalories");
    treeTable->setHeaderLabels(labels);

    QList<QTreeWidgetItem*> items;

    for(struct CTrackData::fitdata_t fitdata : fitdatas)
    {
        QTreeWidgetItem *item = new QTreeWidgetItem();

        if (fitdata.type == CTrackData::fitdata_t::eLap)
        {
            item->setText(eColType, tr("Lap"));
        }
        else if (fitdata.type == CTrackData::fitdata_t::eSession)
        {
            item->setText(eColType, tr("Session"));
        }
        item->setTextAlignment(eColType, Qt::AlignLeft);

        item->setText(eColIndex, QString("%1").arg(fitdata.index));
        item->setTextAlignment(eColIndex, Qt::AlignRight);

        QString val, unit;
        IUnit::self().seconds2time(fitdata.totalElapsedTime, val, unit);
        item->setText(eColElapsedTime, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColElapsedTime, Qt::AlignRight);

        IUnit::self().seconds2time(fitdata.totalTimerTime, val, unit);
        item->setText(eColTimerTime, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColTimerTime, Qt::AlignRight);

        IUnit::self().seconds2time(fitdata.totalElapsedTime - fitdata.totalTimerTime, val, unit);
        item->setText(eColPause, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColPause, Qt::AlignRight);

        IUnit::self().meter2distance(fitdata.totalDistance, val, unit);
        item->setText(eColDistance, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColDistance, Qt::AlignRight);

        IUnit::self().meter2speed(fitdata.avgSpeed / 1000., val, unit);
        item->setText(eColAvgSpeed, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColAvgSpeed, Qt::AlignRight);

        IUnit::self().meter2speed(fitdata.maxSpeed / 1000., val, unit);
        item->setText(eColMaxSpeed, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColMaxSpeed, Qt::AlignRight);

        IUnit::self().meter2elevation(fitdata.totalAscent, val, unit);
        item->setText(eColAscent, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColAscent, Qt::AlignRight);

        IUnit::self().meter2elevation(fitdata.totalDescent, val, unit);
        item->setText(eColDescent, QString("%1%2").arg(val).arg(unit));
        item->setTextAlignment(eColDescent, Qt::AlignRight);

        item->setText(eColAvgHr, QString("%1%2").arg(fitdata.avgHr).arg(tr("bpm")));
        item->setTextAlignment(eColAvgHr, Qt::AlignRight);
        item->setText(eColMaxHr, QString("%1%2").arg(fitdata.maxHr).arg(tr("bpm")));
        item->setTextAlignment(eColMaxHr, Qt::AlignRight);

        item->setText(eColAvgCad, QString("%1%2").arg(fitdata.avgCad).arg(tr("rpm")));
        item->setTextAlignment(eColAvgCad, Qt::AlignRight);
        item->setText(eColMaxCad, QString("%1%2").arg(fitdata.maxCad).arg(tr("rpm")));
        item->setTextAlignment(eColMaxCad, Qt::AlignRight);

        item->setText(eColAvgPower, QString("%1%2").arg(fitdata.avgPower).arg("Watt"));
        item->setTextAlignment(eColAvgPower, Qt::AlignRight);
        item->setText(eColMaxPower, QString("%1%2").arg(fitdata.maxPower).arg("Watt"));
        item->setTextAlignment(eColMaxPower, Qt::AlignRight);
        item->setText(eColNormPower, QString("%1%2").arg(fitdata.normPower).arg("Watt"));
        item->setTextAlignment(eColNormPower, Qt::AlignRight);

        item->setText(eColLeftBalance, QString("%1%").arg(fitdata.leftBalance));
        item->setTextAlignment(eColLeftBalance, Qt::AlignRight);
        item->setText(eColRightBalance, QString("%1%").arg(fitdata.rightBalance));
        item->setTextAlignment(eColRightBalance, Qt::AlignRight);

        item->setText(eColLeftPedalSmooth, QString("%1%").arg(fitdata.leftPedalSmooth));
        item->setTextAlignment(eColLeftPedalSmooth, Qt::AlignRight);
        item->setText(eColRightPedalSmooth, QString("%1%").arg(fitdata.rightPedalSmooth));
        item->setTextAlignment(eColRightPedalSmooth, Qt::AlignRight);

        item->setText(eColLeftTorqueEff, QString("%1%").arg(fitdata.leftTorqueEff));
        item->setTextAlignment(eColLeftTorqueEff, Qt::AlignRight);
        item->setText(eColRightTorqueEff, QString("%1%").arg(fitdata.rightTorqueEff));
        item->setTextAlignment(eColRightTorqueEff, Qt::AlignRight);

        item->setText(eColTrainStress, QString("%1").arg(fitdata.trainStress));
        item->setTextAlignment(eColTrainStress, Qt::AlignRight);
        item->setText(eColIntensity, QString("%1").arg(fitdata.intensity));
        item->setTextAlignment(eColIntensity, Qt::AlignRight);

        item->setText(eColWork, QString("%1%2").arg(fitdata.work / 1000).arg("kJ"));
        item->setTextAlignment(eColWork, Qt::AlignRight);
        item->setText(eColCalories, QString("%1%2").arg(fitdata.totalCalories).arg("kcal"));
        item->setTextAlignment(eColCalories, Qt::AlignRight);

        items << item;
    }
    treeTable->clear();
    treeTable->addTopLevelItems(items);
    treeTable->header()->resizeSections(QHeaderView::ResizeToContents);

}

CFitDataDialog::~CFitDataDialog()
{
}

/** @brief Update all widgets when a input value has changed in dialog
 */

/** @brief When "Ok" button is clicked:
     Set the temporarily parameter set back to parameter set of the track
     Compute the "Energy Use Cycling" value in track parameter set
     Update history
     Update status panel
     Save parameter set to SETTINGS
 */
/*
void CFitDataDialog::slotOk(bool)
{
    accept();
}
*/
