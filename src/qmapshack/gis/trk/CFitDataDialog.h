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

#include "ui_IFitDataDialog.h"

class CGisItemTrk;
class CFitData;

class CFitDataDialog : public QDialog, private Ui::IFitDataDialog
{
  Q_OBJECT

 public:
  explicit CFitDataDialog(QWidget *parent, CGisItemTrk& trk);
  ~CFitDataDialog();

  struct column_t
  {
    QString label;
    Qt::AlignmentFlag alignment;
  };

  enum columnTypes_e {
    eColProduct
    , eColNo
    , eColType
    , eColComment
    , eColStartTime
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
    , eColLeftPco
    , eColRightPco
    , eColLeftPp
    , eColLeftPpPeak
    , eColRightPp
    , eColRightPpPeak
    , eColFtp
    , eColIf
    , eColTss
    , eColWork
    , eColEnergy
    , eColCount //The number of the enum items
  };

 private slots:
  void slotOk(bool);
  void slotCancel(bool);
  void slotDeleteFitDataFromTrack(bool);
  void slotSettingsDialog(bool);
  void slotItemDoubleClicked(QTreeWidgetItem* item, qint32 column);
  void slotCurrentItemChanged(QTreeWidgetItem* currentItem, QTreeWidgetItem*);
  void slotShowSessionsDb(bool checked);
  void slotAddSessionToDb();
  void slotDeleteSessionFromDb();
  void slotShowTrkptInfo(bool checked);
  void slotShowHelp();

 private:
  QMap<quint16, QString> products = {
      {0, "Unknown"}
      , {1836, "GARMIN Edge 1000"}
      , {3011, "GARMIN Edge Explore"}
      , {4440, "GARMIN Edge 1050"}
  };

  QMap<qint32, struct column_t> columns = {
      {eColProduct, {"Product", Qt::AlignLeft}}
      ,{eColNo, {"#", Qt::AlignRight}}
      , {eColType, {tr("Type"), Qt::AlignLeft}}
      , {eColComment, {tr("Comment"), Qt::AlignLeft}}
      , {eColStartTime, {tr("Start Time"), Qt::AlignLeft}}
      , {eColElapsedTime, {tr("Elaps. Time"), Qt::AlignLeft}}
      , {eColTimerTime, {tr("Timer Time"), Qt::AlignLeft}}
      , {eColPause, {tr("Pause"), Qt::AlignLeft}}
      , {eColDistance, {tr("Distance"), Qt::AlignRight}}
      , {eColAvgSpeed, {tr("Avg. Speed"), Qt::AlignRight}}
      , {eColMaxSpeed, {tr("Max. Speed"), Qt::AlignRight}}
      , {eColAscent, {tr("Ascent"), Qt::AlignRight}}
      , {eColDescent, {tr("Descent"), Qt::AlignRight}}
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
      , {eColLeftPco, {tr("Left PCO"), Qt::AlignRight}}
      , {eColRightPco, {tr("Right PCO"), Qt::AlignRight}}
      , {eColLeftPp, {tr("Left Power Phase"), Qt::AlignLeft}}
      , {eColLeftPpPeak, {tr("Left Power Phase Peak"), Qt::AlignLeft}}
      , {eColRightPp, {tr("Right Power Phase"), Qt::AlignLeft}}
      , {eColRightPpPeak, {tr("Right Power Phase Peak"), Qt::AlignLeft}}
      , {eColFtp, {tr("Func. Thresh. Power"), Qt::AlignRight}}
      , {eColIf, {tr("Intensity Factor"), Qt::AlignRight}}
      , {eColTss, {tr("Training Stress Score"), Qt::AlignRight}}
      , {eColWork, {tr("Work"), Qt::AlignRight}}
      , {eColEnergy, {tr("Energy Use"), Qt::AlignRight}}
  };
   struct direction_t {
     qint32 gt;
     qint32 lt;
     QRect rect;
     qint32 alignment;
   };
   QList<struct direction_t> const directions = {
      {0, 0, QRect(-0.5, -1, 1, 1), Qt::AlignHCenter | Qt::AlignBottom}
      , {1, 89, QRect(0, 0, 1, 1), Qt::AlignLeft | Qt::AlignBottom}
      , {90, 90, QRect(0, -0.5, 1, 1), Qt::AlignLeft | Qt::AlignVCenter}
      , {91, 179, QRect(0, -0.5, 1, 1), Qt::AlignLeft | Qt::AlignTop}
      , {180, 180, QRect(-0.5, -1, 1, 1), Qt::AlignHCenter | Qt::AlignTop}
      , {181, 269, QRect(-1, 0, 1, 1), Qt::AlignRight | Qt::AlignTop}
      , {270, 270, QRect(-1, -0.5, 1, 1), Qt::AlignRight | Qt::AlignVCenter}
      , {271, 359, QRect(-1, -1, 1, 1), Qt::AlignRight | Qt::AlignBottom}
  };

  struct marker_t {
    qint32 angle;
    qint32 length;
  };

  bool checkDbAccess();
  void enableButtons();
  void updateData(const QList<struct CFitData::lap_t>& laps);
  void updateDataMivs();
  QString getPowerPhaseStr(const QList<qreal> &powerPhases, qint32 phase);
  void getCellStr(const CFitData::lap_t& lap, qint32 column, QString& cellStr);
  void paintGraphics(const CFitData::lap_t &lap);

  CGisItemTrk& trk;
  QList<qint32> shownTableCols;
  QList<qint32> shownMivs;
  const qint32 maxMivs = 8;
  QList<QLabel*> mivLabels;
  QString connectionDbName;
  QList<struct CFitData::lap_t> laps;
  QPushButton* buttonSettingsDialog;
};

#endif // CFITDATADIALOG_H
