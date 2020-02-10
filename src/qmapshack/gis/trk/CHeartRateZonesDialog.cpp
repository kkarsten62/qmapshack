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
#include "gis/trk/CKnownExtension.h"
#include "helpers/CSettings.h"

CHeartRateZonesDialog::CHeartRateZonesDialog(QWidget *parent, const CGisItemTrk &trk) :
    QDialog(parent)
    , trk(trk)
{
    setupUi(this);

    SETTINGS;
    maxHr = cfg.value("TrackDetails/HeartRateZones/maxHeartRate", 180).toInt();
    spinMaxHr->setValue(maxHr);

    setRowsHrLabel();
    for (qint32 row = 0; row < gridRows.size(); row++)
    {
        struct gridRow_t &gridRow = gridRows[row];

        for (qint32 column = 0; column < gridColumns.size(); column++)
        {
            struct gridCell_t gridCell;
            gridCell.init();

            switch(column)
            {
            case 0:
            {
               gridCell.percentBar = new CPercentBar(CPercentBar::valFormat_e::integer, gridRow.color, this);
               break;
            }
            case 1:
            {
               gridCell.percentBar = new CPercentBar(CPercentBar::valFormat_e::time, gridRow.color, this);
               break;
            }
            case 2:
            {
               gridCell.percentBar = new CPercentBar(CPercentBar::valFormat_e::none, gridRow.color, this);
               break;
            }
            case 3:
            case 4:
            {
               gridCell.percentBar = new CPercentBar(CPercentBar::valFormat_e::elevation, gridRow.color, this);
               break;
            }
            }
            gridLayout->addWidget(gridCell.percentBar, row * 2 + 2, column + 3, 1, 1);
            gridCells << gridCell;
        }
    }
    connect(spinMaxHr, SIGNAL(valueChanged(qint32)), this, SLOT(slotSetMaxHr(qint32)));

    computeCells();
}

CHeartRateZonesDialog::~CHeartRateZonesDialog()
{
    SETTINGS;
    cfg.setValue("TrackDetails/HeartRateZones/maxHeartRate", spinMaxHr->value());
}

void CHeartRateZonesDialog::setRowsHrLabel()
{
    for (qint32 row = 0; row < gridRows.size(); row++)
    {
        struct gridRow_t &gridRow = gridRows[row];

        gridRow.minHr = qRound(gridRow.minPercent * maxHr / 100.0);
        if (row == (gridRows.size() - 1))
        {
            gridRow.maxHr = 220;
        }
        else
        {
            gridRow.maxHr = qRound(gridRow.maxPercent * maxHr / 100.0);
        }
        QString label = QString("%1").arg(gridRow.minHr) + tr("bpm") + " - " + QString("%1").arg(gridRow.maxHr) + tr("bpm");
        static_cast<QLabel *>(gridLayout->itemAtPosition(row * 2 + 2, 2)->widget())->setText(label);
    }
}

void CHeartRateZonesDialog::slotSetMaxHr(qint32 maxHr)
{
    this->maxHr = maxHr;
    setRowsHrLabel();
    computeCells();
}

void CHeartRateZonesDialog::computeCells()
{
    quint32 invalidMask = (trk.getAllValidFlags() & CTrackData::trkpt_t::eValidMask) << 16;

    qint32 lastEle = NOINT;
    CTrackData::trkpt_t const *lastTrkpt  = nullptr;

    for(struct gridCell_t &gridCell : gridCells)
    {
        gridCell.init();
    }
    for(struct gridColumn_t &gridColumn : gridColumns)
    {
        gridColumn.init();
    }

    trkHeartBeats = 0;
    trkHeartBeats = 0;
    for(const CTrackData::trkpt_t& trkpt : trk.getTrackData())
    {
        if(trkpt.isInvalid(CTrackData::trkpt_t::invalid_e(invalidMask)) || trkpt.isHidden())
        {
            continue;
        }

        qreal hr = trkpt.extensions["gpxtpx:TrackPointExtension|gpxtpx:hr"].toDouble();
        if (hr <= 0)
        {
            continue;
        }
        minTrkHr = qMin(minTrkHr, hr);
        maxTrkHr = qMax(maxTrkHr, hr);

        qint32 zone = 0;
        for (qint32 i = gridRows.size() - 1; i >= 0; --i)
        {
            if (hr >= gridRows[i].minHr)
            {
                zone = i;
                break;
            }
        }
//        qDebug() << "hr: " << hr << "zone:" << zone;

        // Points
//            qDebug() << "Cell Row0 No=" <<  zone * gridColumns.size() + 0;
        gridCells[zone * gridColumns.size() + 0].count++;
        gridCells[zone * gridColumns.size() + 0].val++;
        gridColumns[0].count++;
        gridColumns[0].val++;

        if(lastTrkpt != nullptr)
        {
            // Moving
            gridCells[zone * gridColumns.size() + 1].count++;
            qreal dt = trkpt.elapsedSecondsMoving - lastTrkpt->elapsedSecondsMoving;
            gridCells[zone * gridColumns.size() + 1].val += dt;
            gridColumns[1].count++;
            gridColumns[1].val += dt;
            trkHeartBeats += dt * (hr / 60.);

//            qDebug() << "#=" << gridColumns[0].count << " dt=" << dt << " hr=" << hr << " beats=" << dt * (hr / 60.);

            // Ascent, Descent
            if(lastEle != NOINT)
            {
                qint32 delta  = trkpt.ele - lastEle;

                if(qAbs(delta) >= ASCENT_THRESHOLD)
                {
                    const qint32 step = (delta / ASCENT_THRESHOLD) * ASCENT_THRESHOLD;

                    if(delta > 0)
                    {
//                        qDebug() << "Cell Row2 No=" <<  zone * gridColumns.size() + 2;
                        gridCells[zone * gridColumns.size() + 3].count++;
                        gridCells[zone * gridColumns.size() + 3].val += step;
                        gridColumns[3].count++;
                        gridColumns[3].val += step;
                    }
                    else
                    {
//                        qDebug() << "Cell Row3 No=" <<  zone * gridColumns.size() + 3;
                        gridCells[zone * gridColumns.size() + 4].count++;
                        gridCells[zone * gridColumns.size() + 4].val += step;
                        gridColumns[4].count++;
                        gridColumns[4].val += step;
                    }
                    lastEle += step;
                }
                else
                {
                    gridCells[zone * gridColumns.size() + 2].count++;
                    gridCells[zone * gridColumns.size() + 2].val++;
                    gridColumns[2].count++;
                    gridColumns[2].val++;
                }
            }

            // slopes uphill, downhill
            if (trkpt.slope2 > 0)
            {
//                zoneItems[zone][3].count++;
//                zoneItems[zone][3].sum += trkpt.slope2;
            }
            if (trkpt.slope2 < 0)
            {
//                zoneItems[zone][4].count++;
//                zoneItems[zone][4].sum += trkpt.slope2;
            }
        }
        else
        {
            lastEle = trkpt.ele;
            gridCells[zone * gridColumns.size() + 2].count++; // First point is flat
            gridCells[zone * gridColumns.size() + 2].val++;
            gridColumns[2].count++;
            gridColumns[2].val++;
        }
        lastTrkpt = &trkpt;
    }

    for(qint32 i = 0; i < gridCells.size(); i++)
    {
        struct gridCell_t &gridCell = gridCells[i];

//        qDebug() << "gridCell i=" << i << " gridCell count=" << gridCell.count;
//        qDebug() << "gridCell i=" << i << " gridCell   val=" << gridCell.val;

        qint32 column = i - qFloor(i / gridColumns.size()) * gridColumns.size();
//        qDebug() << "colum=" << column;

        qreal percent = gridColumns[column].val ? gridCell.val / gridColumns[column].val * 100 : 0;
        gridCell.percentBar->setValues(percent, gridCell.val);
//        qDebug() << "gridCell i=" << i << " gridCell percent=" << percent;
    }

    labelTotalPoints->setText(QString("%L1").arg(gridColumns[0].count));
    QString text, unit;
    IUnit::self().seconds2time(gridColumns[1].val, text, unit);
    labelTotalMoving->setText(QString("%1%2").arg(text).arg(unit));

    IUnit::self().meter2elevation(gridColumns[3].val, text, unit);
    labelTotalAscent->setText(QString("%1%2").arg(text).arg(unit));

    IUnit::self().meter2elevation(gridColumns[4].val, text, unit);
    labelTotalDescent->setText(QString("%1%2").arg(text).arg(unit));

    qDebug() << "trkHeartBeats :" << trkHeartBeats;
    qDebug() << "trkHeartBeats avg:" <<  trkHeartBeats / gridColumns[1].val * 60;
    qDebug() << "Fitness Rate:" << trk.getEnergyCycling().getEnergyUseCycling() * 4.1868 / trkHeartBeats;

    labelMinHr->setText(QString("%1").arg(minTrkHr, 0, 'f', 1) + tr("bpm"));
    labelMaxHr->setText(QString("%1").arg(maxTrkHr, 0, 'f', 1) + tr("bpm"));
    labelAvgHr->setText(QString("%1").arg(trkHeartBeats / gridColumns[1].val * 60, 0, 'f', 1) + tr("bpm"));
    labelHeartBeats->setText(QString("%L1").arg(trkHeartBeats, 0, 'f', 0));

    qreal energyUseCycling = trk.getEnergyCycling().getEnergyUseCycling();
    if (energyUseCycling != NOFLOAT)
    {
        labelJouleHb->setText(QString("%L1").arg(trk.getEnergyCycling().getEnergyUseCycling() * 4.1868 / trkHeartBeats, 0, 'f', 4));
    }

    update();
}

// class CPercentBar
void CPercentBar::paintEvent(QPaintEvent *)
{
//    qDebug() << "CPercentBar::paintEvent";

    QPainter painter(this);
    setMinimumSize(108, 64);
    painter.setRenderHint(QPainter::Antialiasing);

    const int ascent = QFontMetrics(painter.font()).ascent();
    const int descent = QFontMetrics(painter.font()).descent();

    QPainterPath outerRect, innerRect;
    painter.setPen(QPen(Qt::black, 0.5));
    outerRect.addRoundedRect(QRectF(0.5, ascent + descent + 5, 107, 20), 2, 2);
    painter.drawPath(outerRect);
    painter.fillPath(outerRect, Qt::white);

    qreal valuePos = (percent > 0 && percent < 0.5) ? 0.5 : percent;
    innerRect.addRect(QRectF(4 - 0.5, ascent + descent + 5 + 3, valuePos, 14));
    painter.fillPath(innerRect, QBrush(color));

    // Two small tick lines nearby the texts
    painter.setPen(QPen(Qt::black, 0.5));
    painter.drawLine(percent + 4 - 0.5, ascent + descent, percent + 4 - 0.5, ascent + descent + 5);
    if (valFormat != valFormat_e::none)
    {
        painter.drawLine(percent + 4 - 0.5, ascent + descent + 5 + 20, percent + 4 - 0.5, ascent + descent + 5 + 20 + 5);
    }

    const QString percentStr = QString("%1%").arg(percent, 0, 'f', 1);
    qreal percentPos = adjustTextPosition(percentStr, painter);
    painter.drawText(percentPos + 4, ascent, percentStr);

    QString valStr;
    switch (valFormat)
    {
    case valFormat_e::none:
    {
        valStr = "";
        break;
    }
    case valFormat_e::integer:
    {
        valStr = QString("%L1").arg(val, 0, 'f', 0);
        break;
    }
    case valFormat_e::time:
    {
        QString timeStr, unit;
        IUnit::self().seconds2time(val, timeStr, unit);
        valStr = QString("%1%2").arg(timeStr).arg(unit);
        break;
    }
    case valFormat_e::elevation:
    {
        QString elevationStr, unit;
        IUnit::self().meter2elevation(val, elevationStr, unit);
        valStr = QString("%1%2").arg(elevationStr).arg(unit);
        break;
    }
    }
    qreal valStrPos = adjustTextPosition(valStr, painter);
    painter.drawText(valStrPos, ascent + descent + 5 + 20 + 5 + ascent, valStr);
}

qreal CPercentBar::adjustTextPosition(const QString &valueStr, QPainter &painter)
{
    const qreal width = QFontMetrics(painter.font()).boundingRect(valueStr).width();

    qreal textPosition = ((percent - width / 2) < 0) ? 0 : percent - width / 2;
    textPosition = ((percent + width / 2) > 100) ? 100 - width : textPosition;

    return textPosition;
}

