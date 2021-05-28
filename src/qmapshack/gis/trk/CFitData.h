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

//#include "gis/trk/CGisItemTrk.h"

#include "units/IUnit.h"

#include <QDataStream>

class CGisItemTrk;

class CFitData
{
public:
    enum lapType_e
    {
        eTypeUnknown   = 0
        , eTypeLap     = 1
        , eTypeSession = 2
    };

    struct lap_t
    {
        qint32 type = lapType_e::eTypeUnknown;
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

    CFitData(CGisItemTrk& trk);
    virtual ~CFitData() = default;

    bool getIsValid() const;
    void setIsValid(bool isValid);
    QList<lap_t>& getLaps();
    void setLap(const struct lap_t& lap);
    lap_t &getLap(quint32 index);
    void clear();
    quint16 getProduct() const;
    void setProduct(quint16 product);
    void setLapComment(qint32 index, const QString& comment);
    qint32 getLapNo(qint32 index) const;

private:
    friend QDataStream& operator<<(QDataStream& stream, const CFitData& f);
    friend QDataStream& operator>>(QDataStream& stream, CFitData& f);

    CGisItemTrk& trk;

    bool isValid = false;
    quint16 product = 0;
    QList<struct lap_t> laps;
};

#endif // CFITDATA_H
