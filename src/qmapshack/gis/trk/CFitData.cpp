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

CFitData::CFitData(CGisItemTrk& trk) :
    trk(trk)
{
}

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

void CFitData::setLap(const lap_t& lap)
{
    laps << lap;
}

CFitData::lap_t& CFitData::getLap(quint32 index)
{
    return laps[index];
}

void CFitData::clear()
{
    laps.clear();
    isValid = false;
    delTrkPtDesc();
    idxDescs.clear();
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

void CFitData::assignTimeToIdx()
{
    if (!idxDescs.isEmpty())
    {
        return;
    }
    for (const struct lap_t lap : laps)
    {
        if (lap.type != eTypeLap || !lap.endTime.isValid())
        {
            continue;
        }
        for(const CTrackData::trkpt_t& pt : trk.getTrackData())
        {
            if (pt.time == lap.endTime)
            {
                idxDescs.insert(pt.idxTotal,
                               QString(tr("FIT LAP")) + QString("-%1 (%2)").arg(lap.no).arg(pt.idxTotal));
            }
        }
    }
}

void CFitData::setTrkPtDesc()
{
    if (!isValid || laps.isEmpty())
    {
        return;
    }

    assignTimeToIdx();
    trk.setTrkPtDesc(idxDescs);
}

void CFitData::delTrkPtDesc()
{
    if (!isValid || laps.isEmpty())
    {
        return;
    }

    assignTimeToIdx();
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
