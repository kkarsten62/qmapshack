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

#ifndef CFITDATADIALOG_H
#define CFITDATADIALOG_H

#include "gis/trk/CGisItemTrk.h"
#include "ui_IFitDataDialog.h"

// class CGisItemTrk;

/** @brief GUI Dialog class to modify the CEnergyCycling parameter set
 */
class CFitDataDialog : public QDialog, private Ui::IFitDataDialog
{
    Q_OBJECT

public:
    explicit CFitDataDialog(CTrackData::fitdata_t &fitdata, QWidget *parent);
    ~CFitDataDialog();

private slots:
    void slotOk(bool);
    void slotReset(bool);
    void slotButtonColumns(bool);
    void slotCheckColumns(bool checked);
    void slotSave2Csv(bool);
    void slotItemDoubleClicked(QTreeWidgetItem* item, qint32 column);
    void slotShowHelp();

private:
    CTrackData::fitdata_t &fitdata;

    QMap<quint16, QString> productName = {
        {1836, "Garmin Edge 1000"}
    };

    enum columns_t
    {
        eColType
        , eColIndex
        , eColComment
        , eColElapsedTime
        , eColTimerTime
        , eColPause
        , eColDistance
        , eColAvgSpeed
        , eColMaxSpeed
        , eColAscent
        , eColDescent
        , eColAvgHr
        , eColMaxHr
        , eColAvgCad
        , eColMaxCad
        , eColAvgPower
        , eColMaxPower
        , eColNormPower
        , eColLeftBalance
        , eColRightBalance
        , eColLeftPedalSmooth
        , eColRightPedalSmooth
        , eColLeftTorqueEff
        , eColRightTorqueEff
        , eColTrainStress
        , eColIntensity
        , eColWork
        , eColEnergy
        , eColMax
    };
    struct columnLabel_t
    {
        QString label;
        Qt::AlignmentFlag alignment;
    };
    QMap<columns_t, struct columnLabel_t> columns = {
        {eColType, {tr("Type"), Qt::AlignLeft}}
        , {eColIndex, {"#", Qt::AlignRight}}
        , {eColComment, {tr("Comment"), Qt::AlignLeft}}
        , {eColElapsedTime, {tr("Elaps. Time"), Qt::AlignRight}}
        , {eColTimerTime, {tr("Timer Time"), Qt::AlignRight}}
        , {eColPause, {tr("Pause"), Qt::AlignRight}}
        , {eColDistance, {tr("Distance"), Qt::AlignRight}}
        , {eColAvgSpeed, {tr("Avg. Speed"), Qt::AlignRight}}
        , {eColMaxSpeed, {tr("Max. Speed"), Qt::AlignRight}}
        , {eColAscent, {tr("Ascent"), Qt::AlignRight}}
        , {eColDescent, {tr("Desent"), Qt::AlignRight}}
        , {eColAvgHr, {tr("Avg. HR"), Qt::AlignRight}}
        , {eColMaxHr, {tr("Max. HR"), Qt::AlignRight}}
        , {eColAvgCad, {tr("Avg. Cad."), Qt::AlignRight}}
        , {eColMaxCad, {tr("Max. Cad."), Qt::AlignRight}}
        , {eColAvgPower, {tr("Avg. Power"), Qt::AlignRight}}
        , {eColMaxPower, {tr("Max. Power"), Qt::AlignRight}}
        , {eColNormPower, {tr("Norm. Power"), Qt::AlignRight}}
        , {eColLeftBalance, {tr("Left Balance"), Qt::AlignRight}}
        , {eColRightBalance, {tr("Right Balance"), Qt::AlignRight}}
        , {eColLeftPedalSmooth, {tr("Left Pedal Smooth."), Qt::AlignRight}}
        , {eColRightPedalSmooth, {tr("Right Pedal Smooth."), Qt::AlignRight}}
        , {eColLeftTorqueEff, {tr("Left Torque Eff."), Qt::AlignRight}}
        , {eColRightTorqueEff, {tr("Right Torque Eff."), Qt::AlignRight}}
        , {eColTrainStress, {tr("Training Stress"), Qt::AlignRight}}
        , {eColIntensity, {tr("Intensity"), Qt::AlignRight}}
        , {eColWork, {tr("Work"), Qt::AlignRight}}
        , {eColEnergy, {tr("Energy Use"), Qt::AlignRight}}
    };

    quint32 checkstates; //Bitmask to store checkbox states, 31 columns max
    bool isChanged = false;
};

#endif // CFITDATADIALOG_H
