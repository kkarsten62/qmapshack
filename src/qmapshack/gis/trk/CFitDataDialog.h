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

/** @brief GUI Dialog class to modify the CEnergyCycling parameter set
 */
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

  enum columnTypes_e
  {
    eColNo
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
    , eColWork
    , eColEnergy
    , eColCount //The number of the enum items
  };

 private slots:
  void slotOk(bool);
  void slotReset(bool);
  void slotButtonColumns(bool);
  void slotCheckColumns(bool checked);
  void slotSave2Csv(bool);
  void slotToogleView(bool);
  void slotSettingsDialog(bool);
  void slotItemDoubleClicked(QTreeWidgetItem* item, qint32 column);
  void slotShowTrkptInfo(bool checked);
  void slotShowHelp();
  void paintGraphics();

 private:
  QMap<quint16, QString> productName = {
      {0, "Unknown"}
      , {1836, "GARMIN Edge 1000"}
      , {3011, "GARMIN Edge Explore"}
      , {4440, "GARMIN Edge 1050"}
  };

  QList<qint32> shownTableCols;
  QList<qint32> shownMivs;

//  struct column_t
//  {
//    QString label;
//    Qt::AlignmentFlag alignment;
//  };
  /*
   * Information:
   * Structure is:
   * enum column, text in header column cell, alignment in header colum cell, shown in lap view, shown in session db view
   */

  QMap<qint32, struct column_t> columns = {
      {eColNo, {"#", Qt::AlignRight}}
      , {eColType, {tr("Type"), Qt::AlignLeft}}
      , {eColComment, {tr("Comment"), Qt::AlignLeft}}
      , {eColStartTime, {tr("Start Time"), Qt::AlignLeft}}
      , {eColElapsedTime, {tr("Elaps. Time"), Qt::AlignRight}}
      , {eColTimerTime, {tr("Timer Time"), Qt::AlignRight}}
      , {eColPause, {tr("Pause"), Qt::AlignRight}}
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
      , {eColWork, {tr("Work"), Qt::AlignRight}}
      , {eColEnergy, {tr("Energy Use"), Qt::AlignRight}}
  };

  /*
  QList<struct column_t> columns1 = {
      {"#", Qt::AlignRight} //0
      , {tr("Type"), Qt::AlignLeft} //1
      , {tr("Comment"), Qt::AlignLeft} //2
      , {tr("Start Time"), Qt::AlignLeft} //3
      , {tr("Elaps. Time"), Qt::AlignRight} //4
      , {tr("Timer Time"), Qt::AlignRight} //5
      , {tr("Pause"), Qt::AlignRight} //6
      , {tr("Distance"), Qt::AlignRight} //7
      , {tr("Avg. Speed"), Qt::AlignRight} //8
      , {tr("Max. Speed"), Qt::AlignRight}
      , {tr("Ascent"), Qt::AlignRight}
      , {tr("Descent"), Qt::AlignRight}
      , {tr("Avg. HR"), Qt::AlignRight}
      , {tr("Max. HR"), Qt::AlignRight}
      , {tr("Avg. Cad."), Qt::AlignRight}
      , {tr("Max. Cad."), Qt::AlignRight}
      , {tr("Avg. Power"), Qt::AlignRight}
      , {tr("Max. Power"), Qt::AlignRight}
      , {tr("Norm. Power"), Qt::AlignRight}
      , {tr("Left Balance"), Qt::AlignRight}
      , {tr("Right Balance"), Qt::AlignRight}
      , {tr("Left Pedal Smooth."), Qt::AlignRight}
      , {tr("Right Pedal Smooth."), Qt::AlignRight}
      , {tr("Left Torque Eff."), Qt::AlignRight}
      , {tr("Right Torque Eff."), Qt::AlignRight}
      , {tr("Work"), Qt::AlignRight}
      , {tr("Energy Use"), Qt::AlignRight}
  };
*/
  void getCellString(const CFitData::lap_t& lap, qint32 shownTableCol, QString& cellStr);
  CGisItemTrk& trk;
  //QList<qint32> shownTableCols;
  const qint32 maxMivs = 8;
  QList<QLabel *> mivLabels;
  quint32 checkstates; // Bitmask to store checkbox states, 32 columns max
  bool isChanged = false;
};

#endif // CFITDATADIALOG_H
