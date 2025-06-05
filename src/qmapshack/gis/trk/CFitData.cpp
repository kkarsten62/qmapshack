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

#include "gis/trk/CFitData.h"
#include "gis/trk/CGisItemTrk.h"

bool CFitData::getIsValid() const
{
    return isValid;
}

void CFitData::setIsValid(bool isValid)
{
    this->isValid = isValid;
}

QList<CFitData::lap_t>& CFitData::getLaps()
{
    return laps;
}

quint32 CFitData::getNoOfLaps() {
  return laps.size();
}

void CFitData::setLap(quint32 index, const lap_t& lap)
{
    //laps << lap;
  laps.insert(index, lap);
}

CFitData::lap_t& CFitData::getLap(quint32 index)
{
  if (laps.size()) {
    return laps[index];
  } else {
    //return nullptr;
  }
}

CFitData::lap_t& CFitData::getSession()
{
  if (laps.size() >= 2) { //Minimum one lap and the session
    return laps[laps.size() - 1]; //The latest lap is the session
  } else {
    //return nullptr;
  }
}
/*
qreal CFitData::getFunctionalThresholdPower() const {
  return functionalThresholdPower;
}
void CFitData::setFunctionalThresholdPower(qreal functionalThresholdPower) {
  this->functionalThresholdPower = functionalThresholdPower;
}
qreal CFitData::getIntensityFactor() const {
  return intensityFactor;
}
void CFitData::setIntensityFactor(qreal intensityFactor) {
  this->intensityFactor = intensityFactor;
}
qreal CFitData::getTrainingStressScore() const {
  return trainStressScore;
}
void CFitData::setTrainingStressScore(qreal trainStressScore) {
  this->trainStressScore = trainStressScore;
}
*/
void CFitData::clear(CGisItemTrk& trk)
{
    delTrkPtDesc(trk); // Must be done first
    laps.clear();
    idxDescs.clear();
    isValid = false;
    isTrkptInfo = false;
}

quint16 CFitData::getProduct() const
{
    return product;
}

void CFitData::setProduct(quint16 product)
{
    this->product = product;
}

void CFitData::setLapComment(qint32 index, const QString& comment)
{
    laps[index].comment = comment;
}

qint32 CFitData::getLapNo(qint32 index) const
{
    return laps[index].no;
}

void CFitData::assignTimeToIdx(CGisItemTrk& trk)
{
    if (!idxDescs.isEmpty())
    {
        return;
    }
    for (const struct lap_t &lap : laps)
    {
        if (lap.type != eTypeLap || !lap.startTime.isValid())
        {
            continue;
        }
        //Naive approach to find closest startTime next to a track point
        //See https://www.geeksforgeeks.org/find-closest-number-array/
        qint32 idx = 0;
        CTrackData::trkpt_t ptClosedBy;
        for(const CTrackData::trkpt_t& pt : trk.getTrackData())
        {
          if (idx++ == 0) {
            ptClosedBy = pt;
            continue;
          }
          if (qAbs(pt.time.toSecsSinceEpoch() - lap.startTime.toSecsSinceEpoch())
              <= qAbs(ptClosedBy.time.toSecsSinceEpoch() - lap.startTime.toSecsSinceEpoch())) {
            ptClosedBy = pt;
          }
        }
        qDebug() << "ptClosedBy.idxTotal:" << ptClosedBy.idxTotal << "lap.startTime:" << lap.startTime.toString() << "ptClosedBy.time:" << ptClosedBy.time.toString();
        idxDescs.insert(ptClosedBy.idxTotal,
        QString(tr("FIT LAP")) + QString("-%1 (%2)").arg(lap.no + 1).arg(ptClosedBy.idxTotal));
    }
}

void CFitData::setTrkPtDesc(CGisItemTrk& trk)
{
    if (!isValid || laps.isEmpty())
    {
        return;
    }

    assignTimeToIdx(trk);
    trk.setTrkPtDesc(idxDescs);
}

void CFitData::delTrkPtDesc(CGisItemTrk& trk)
{
    if (!isValid || laps.isEmpty())
    {
        return;
    }

    assignTimeToIdx(trk);
    QList<qint32> idxTotals = idxDescs.keys();
    trk.delTrkPtDesc(idxTotals);
}

bool CFitData::getIsTrkptInfo() const
{
    return isTrkptInfo;
}

void CFitData::setIsTrkptInfo(bool isTrkptInfo)
{
    this->isTrkptInfo = isTrkptInfo;
}
