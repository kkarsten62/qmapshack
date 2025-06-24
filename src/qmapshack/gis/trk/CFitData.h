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

#ifndef CFITDATA_H
#define CFITDATA_H

#include "units/IUnit.h"

#include <QCoreApplication>
#include <QDataStream>

class CGisItemTrk;

class CFitData
{
  Q_DECLARE_TR_FUNCTIONS(CFitData)

 public:
  enum lapType_e
  {
    eTypeUnknown   = 0
    , eTypeLap     = 1
    , eTypeSession = 2
  };

  struct lap_t
  {
    quint16 manufacturer = 0;
    quint16 product = 0;
    quint16 no = NOIDX;
    qint32 type = lapType_e::eTypeUnknown;
    QString comment = "-";
    QDateTime startTime;
    qreal elapsedTime = 0;
    qreal timerTime = 0;
    qreal distance = 0;
    qreal avgSpeed = 0; //Enhanced
    qreal maxSpeed = 0; //Enhanced
    quint16 ascent = 0;
    quint16 descent = 0;
    quint8 avgHr = 0;
    quint8 maxHr = 0;
    quint8 avgCad = 0;
    quint8 maxCad = 0;
    quint16 avgPower = 0;
    quint16 maxPower = 0;
    quint16 normPower = 0;
    quint16 leftRightBalance = 0;
    qreal leftPedalSmooth = 0;
    qreal rightPedalSmooth = 0;
    qreal leftTorqueEff = 0;
    qreal rightTorqueEff = 0;
    qint8 leftPco = 0;
    qint8 rightPco = 0;
    /*
     *  16 values in QList for powerPhases
     *  0-3  leftPowerPhase
     *  4-7  leftPowerPhasePeak
     *  8-11 rightPowerPhase
     * 12-15 rightPowerPhasePeak
     */
    QList<qreal> powerPhases;
    quint16 functionalThresholdPower = 0;
    qreal intensityFactor = 0;
    qreal trainingStressScore = 0;
    quint32 work = 0;
    quint16 energy = 0;
  };

  CFitData() {}
  virtual ~CFitData() = default;

  bool getIsValid() const;
  void setIsValid(bool isValid);
  QList<lap_t>& getLaps();
  quint16 getNoOfLaps();
  void setLap(quint32 index, const struct lap_t& lap);
  lap_t& getLap(quint32 index);
  lap_t& getSession();
  void clear(CGisItemTrk &trk);
  void setLapComment(qint32 index, const QString& comment);
  void setSessionComment(const QString& comment);
  qint32 getLapNo(qint32 index) const;
  void assignTimeToIdx(CGisItemTrk &trk);
  void setTrkPtDesc(CGisItemTrk &trk);
  void delTrkPtDesc(CGisItemTrk &trk);
  bool getIsTrkptInfo() const;
  void setIsTrkptInfo(bool isTrkptInfo);

 private:
  friend QDataStream& operator<<(QDataStream& stream, const CFitData& f);
  friend QDataStream& operator>>(QDataStream& stream, CFitData& f);
  friend QDataStream& operator<<(QDataStream& stream, const CFitData::lap_t& l);
  friend QDataStream& operator>>(QDataStream& stream, CFitData::lap_t& l);

  bool isValid = false;
  QList<struct lap_t> laps;
  QMap<qint32, QString> idxDescs;
  bool isTrkptInfo = false;
};

/*
 * Old data structure for FIT version 1
 * Will not be used
 * Read in qms serialization only
 */
class CFitDataV1
{
 public:
  struct lap_t
  {
    qint32 type = 0;
    QDateTime endTime;
    qint32 no = NOIDX;
    QString comment = "-";
    quint32 elapsedTime = 0;
    quint32 timerTime = 0;
    quint32 distance = 0;
    quint16 avgSpeed = 0;
    quint16 maxSpeed = 0;
    quint8 avgHr = 0;
    quint8 maxHr = 0;
    quint8 avgCad = 0;
    quint8 maxCad = 0;
    quint16 ascent = 0;
    quint16 descent = 0;
    quint16 avgPower = 0;
    quint16 maxPower = 0;
    quint16 normPower = 0;
    qreal rightBalance = 0;
    qreal leftBalance = 0;
    quint8 leftPedalSmooth = 0;
    quint8 rightPedalSmooth = 0;
    quint8 leftTorqueEff = 0;
    quint8 rightTorqueEff = 0;
    qreal intensity = 0;
    qreal trainStress = 0;
    quint32 work = 0;
    quint16 energy = 0;
  };
 private:
  friend QDataStream& operator>>(QDataStream& stream, CFitDataV1& f);
  friend QDataStream& operator>>(QDataStream& stream, CFitDataV1::lap_t& l);

  bool isValid = false;
  quint16 product = 0;
  QList<struct lap_t> laps;
  //QMap<qint32, QString> idxDescs;
  bool isTrkptInfo = false;
};

#endif // CFITDATA_H
