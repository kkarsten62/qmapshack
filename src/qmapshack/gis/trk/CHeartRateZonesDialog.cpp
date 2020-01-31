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
#include "canvas/CCanvas.h"
#include "gis/trk/CGisItemTrk.h"
#include "gis/trk/CHeartRateZonesDialog.h"
#include "helpers/CSettings.h"

CHeartRateZonesDialog::CHeartRateZonesDialog(QWidget *parent, const CGisItemTrk &trk) :
    QDialog(parent)
    , trk(trk)
{
    setupUi(this);

    SETTINGS;
    maxHeartRate = cfg.value("TrackDetails/HeartRateZones/maxHeartRate", 180).toInt();

    spinMaxHeartRate->setValue(maxHeartRate);

//    QWidget *widget = gridLayout->itemAtPosition(3, 5)->widget();

    CPercentBar *percentBar1 = new CPercentBar(columnType_e::eIsPercent, colorList["moderate"], this);
    CPercentBar *percentBar2 = new CPercentBar(columnType_e::eIsPercent, colorList["fitness"], this);
    CPercentBar *percentBar3 = new CPercentBar(columnType_e::eIsPercent, colorList["aerobic"], this);
    CPercentBar *percentBar4 = new CPercentBar(columnType_e::eIsPercent, colorList["anaerobic"], this);
    CPercentBar *percentBar5 = new CPercentBar(columnType_e::eIsPercent, colorList["warning"], this);

//    gridLayout->itemAtPosition(2, 5)->setGeometry(QRect(0, 0, 200, 50));
    gridLayout->addWidget(percentBar1, 2, 5, 1, 1);
    gridLayout->addWidget(percentBar2, 4, 5, 1, 1);
    gridLayout->addWidget(percentBar3, 6, 5, 1, 1);
    gridLayout->addWidget(percentBar4, 8, 5, 1, 1);
    gridLayout->addWidget(percentBar5, 10, 5, 1, 1);

    connect(spinMaxHeartRate, SIGNAL(valueChanged(qint32)), this, SLOT(slotSetMaxHeartRate(qint32)));
    connect(toolButtonApply, SIGNAL(clicked(bool)), this, SLOT(slotApply(bool)));
}

CHeartRateZonesDialog::~CHeartRateZonesDialog()
{
    SETTINGS;
    cfg.setValue("TrackDetails/HeartRateZones/maxHeartRate", spinMaxHeartRate->value());
}

void CHeartRateZonesDialog::slotSetMaxHeartRate(qint32 maxHeartRate)
{
    this->maxHeartRate = maxHeartRate;
    update();
}

void CHeartRateZonesDialog::slotApply(bool)
{
    quint32 invalidMask = (trk.getAllValidFlags() & CTrackData::trkpt_t::eValidMask) << 16;

    qint32 lastEle       = NOINT;
    CTrackData::trkpt_t const *lastTrkpt  = nullptr;

    initZoneItems();

    for(const CTrackData::trkpt_t& trkpt : trk.getTrackData())
    {
        if(trkpt.isInvalid(CTrackData::trkpt_t::invalid_e(invalidMask)) || trkpt.isHidden())
        {
            continue;
        }

        qreal hr = trkpt.extensions["gpxtpx:TrackPointExtension|gpxtpx:hr"].toDouble();
        if (hr <= 0 || hr == NOFLOAT)
        {
            continue;
        }

        qint32 zone = 0;
        for (qint32 i = 3; i >=0; --i)
        {
            if (hr >= (zones[i] * maxHeartRate / 100))
            {
                zone = i;
                break;
            }
        }
        qDebug() << "hr: " << hr << "zone:" << zones[zone];

        if(lastTrkpt != nullptr)
        {
            // time moving
            qreal dt = (trkpt.time.toMSecsSinceEpoch() - lastTrkpt->time.toMSecsSinceEpoch()) / 1000.0;
            if(dt > 0 && ((trkpt.deltaDistance / dt) > 0.2))
            {
                zoneItems[zone][0].count++;
                zoneItems[zone][0].sum += dt;
            }

            // ascent descent
            if(lastEle != NOINT)
            {
                qint32 delta  = trkpt.ele - lastEle;

                if(qAbs(delta) >= ASCENT_THRESHOLD)
                {
                    const qint32 step = (delta/ASCENT_THRESHOLD)*ASCENT_THRESHOLD;

                    if(delta > 0)
                    {
                        zoneItems[zone][1].count++;
                        zoneItems[zone][1].sum += step;
                    }
                    else
                    {
                        zoneItems[zone][2].count++;
                        zoneItems[zone][2].sum += step;
                    }
                    lastEle += step;
                }
            }

            // slopes uphill, downhill
            if (trkpt.slope2 > 0)
            {
                zoneItems[zone][3].count++;
                zoneItems[zone][3].sum += trkpt.slope2;
            }
            if (trkpt.slope2 < 0)
            {
                zoneItems[zone][4].count++;
                zoneItems[zone][4].sum += trkpt.slope2;
            }
        }
        else
        {
            lastEle        = trkpt.ele;
        }
        lastTrkpt = &trkpt;
    }

    qreal totaltime = 0;
    for (qint32 i = 0; i < 5; i++)
    {
        qDebug() << "zone: " << "moving time: " << zoneItems[i][0].sum;
        totaltime += zoneItems[i][0].sum;
    }
    qDebug() << "totaltime: " << totaltime;
    qDebug() << "getTotalElapsedSecondsMoving: " << trk.getTotalElapsedSecondsMoving();

    qreal totalAscent = 0;
    qreal totalDescent = 0;
    for (qint32 i = 0; i < 5; i++)
    {
        qDebug() << "zone: " << i << "Ascent: " << zoneItems[i][1].sum;
        qDebug() << "zone: " << i << "Desent: " << zoneItems[i][2].sum;;
        totalAscent += zoneItems[i][1].sum;
        totalDescent += zoneItems[i][2].sum;
    }
    qDebug() << "totalAscent: " << totalAscent;
    qDebug() << "totalDescent: " << totalDescent;

    qreal totalSlopeCount = 0;
    for (qint32 i = 0; i < 5; i++)
    {
        qDebug() << "zone: " << i << "Uphill slope: " << zoneItems[i][3].sum / zoneItems[i][3].count;
        qDebug() << "zone: " << i << "Downhill slope: " << zoneItems[i][4].sum / zoneItems[i][4].count;
        totalSlopeCount += zoneItems[i][3].count + zoneItems[i][4].count;
    }
    qDebug() << "totalSlopeCount: " << totalSlopeCount;
}

void CHeartRateZonesDialog::initZoneItems()
{
    for (qint32 i = 0; i < numOfRows; i++)
    {
        for (qint32 j = 0; j < numOfColumns; j++)
        {
            zoneItems[i][j].init();
        }
    }
}

void CHeartRateZonesDialog::paintEvent(QPaintEvent *)
{
    qDebug() << "CHeartRateZonesDialog::paintEvent";
}

// class CPercentBar

void CPercentBar::paintEvent(QPaintEvent *)
{
    qDebug() << "CPercentBar::paintEvent";

    QPainter painter(this);
    setMinimumSize(108, 64);
    painter.setRenderHint(QPainter::Antialiasing);

    const int ascent = QFontMetrics(painter.font()).ascent();
    const int descent = QFontMetrics(painter.font()).descent();

//    constexpr qreal value = 25.0;
    const QString valueStrTop = QString("%1%").arg(value, 0, 'f', 1);

    qreal widgetHeight = ascent + descent + 5 + 20 + 5 + ascent + descent;
    qDebug() << "ascent:" << ascent;
    qDebug() << "descent:" << descent;
    qDebug() << "widgetHeight:" << widgetHeight;

    qreal valuePosTop = adjustTextPosition(valueStrTop);

    painter.drawText(valuePosTop + 4, ascent, valueStrTop);

    QPainterPath outerRect, innerRect;
    painter.setPen(QPen(Qt::black, 0.5));
//        painter.drawRoundedRect(QRectF(0.5, ascent + descent + 5, 159, 20), 2, 2);
    outerRect.addRoundedRect(QRectF(0.5, ascent + descent + 5, 107, 20), 2, 2);
    painter.drawPath(outerRect);
    painter.fillPath(outerRect, Qt::white);

    qreal valuePos = (value > 0 && value < 0.5) ? 0.5 : value;
    innerRect.addRect(QRectF(4 - 0.5, ascent + descent + 5 + 3, valuePos, 14));
//        painter.drawRect(QRectF(4, ascent + descent + 5 + 3, 152, 14));
    painter.fillPath(innerRect, QBrush(color));


    QString valueStrBottom = "88:88:88h";
    qreal valuePosBottom = adjustTextPosition(valueStrBottom);
    painter.drawText(valuePosBottom, ascent + descent + 5 + 20 + 5 + ascent, valueStrBottom);

    painter.setPen(QPen(Qt::black, 0.5));
    painter.drawLine(value + 4 - 0.5, ascent + descent, value + 4 - 0.5, ascent + descent + 5);
    painter.drawLine(value + 4 - 0.5, ascent + descent + 5 + 20, value + 4 - 0.5, ascent + descent + 5 + 20 + 5);

//        const qreal width1 = QFontMetrics(painter.font()).boundingRect("24,1%").width();
//        painter.drawLine(0, 0, 160, 0);
//        painter.drawLine(0, ascent + descent + 5 + 20 + 5 + ascent + descent,
//                         160, ascent + descent + 5 + 20 + 5 + ascent + descent);
}

qreal CPercentBar::adjustTextPosition(const QString &valueStr)
{
    const qreal width = QFontMetrics(QPainter().font()).boundingRect(valueStr).width();

    qreal textPosition = ((value - width / 2) < 0) ? 0 : value - width / 2;
    textPosition = ((value + width / 2) > 100) ? 100 - width : textPosition;

    return textPosition;
}

