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
