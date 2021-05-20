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
    explicit CFitDataDialog(QList<struct CTrackData::fitdata_t> &fitdatas, QWidget *parent);
    ~CFitDataDialog();

private slots:
    void slotReset(bool);
    void slotRestoreDefaults(bool);
    void slotCheckHeader(bool checked);

private:
    enum columns_t
    {
        eColType
        , eColIndex
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
        , eColCalories
        , eColMax
    };
    QList<struct CTrackData::fitdata_t> &fitdatas;
};

#endif // CFITDATADIALOG_H
