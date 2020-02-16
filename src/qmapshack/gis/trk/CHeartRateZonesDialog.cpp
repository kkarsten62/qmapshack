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
            gridCell.option = gridColumns[column].option;

            gridCell.percentBar = new CHeartRateZonesWidget(gridCell.option, gridRow.color);

/*            switch(column)
            {
            case 0:
            {
               gridCell.percentBar = new CHeartRateZonesWidget(gridCell.option, gridRow.color);
               break;
            }
            case 2:
            {
               gridCell.percentBar = new CHeartRateZonesWidget(CHeartRateZonesWidget::valFormat_e::time, gridRow.color);
               break;
            }
            case 3:
            {
               gridCell.percentBar = new CHeartRateZonesWidget(CHeartRateZonesWidget::valFormat_e::speed, gridRow.color);
               break;
            }
            case 4:
            {
               gridCell.percentBar = new CHeartRateZonesWidget(CHeartRateZonesWidget::valFormat_e::none, gridRow.color);
               break;
            }
            case 5:
            case 6:
            {
               gridCell.percentBar = new CHeartRateZonesWidget(CHeartRateZonesWidget::valFormat_e::elevation, gridRow.color);
               break;
            }
            }
*/
            gridLayout->addWidget(gridCell.percentBar, row * 2 + 2, column + 3, 1, 1);
            gridCells << gridCell;
        }
    }
    connect(spinMaxHr, SIGNAL(valueChanged(qint32)), this, SLOT(slotSetMaxHr(qint32)));
    connect(toolButtonHelp, SIGNAL(clicked(bool)), this, SLOT(slotShowHelp()));

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

        qint32 firstRowCell = zone * gridColumns.size();
        // Points
        gridCells[firstRowCell + 0].count++;
        gridCells[firstRowCell + 0].val++;
        gridColumns[0].count++;
        gridColumns[0].val++;

        if(lastTrkpt != nullptr)
        {
            // Length
            gridCells[firstRowCell + 1].count++;
            gridCells[firstRowCell + 1].val += trkpt.deltaDistance;
            gridColumns[1].count++;
            gridColumns[1].val += trkpt.deltaDistance;

            // Moving
            gridCells[firstRowCell + 2].count++;
            qreal dt = trkpt.elapsedSecondsMoving - lastTrkpt->elapsedSecondsMoving;
            gridCells[firstRowCell + 2].val += dt;
            gridColumns[2].count++;
            gridColumns[2].val += dt;
            trkHeartBeats += dt * (hr / 60.);

            // Speed
            gridColumns[3].count++;
            gridColumns[3].val = qMax(trkpt.speed, gridColumns[3].val);

            // Flat, Ascent, Descent
            if(lastEle != NOINT)
            {
                qint32 delta  = trkpt.ele - lastEle;

                if(qAbs(delta) >= ASCENT_THRESHOLD)
                {
                    const qint32 step = (delta / ASCENT_THRESHOLD) * ASCENT_THRESHOLD;

                    if(delta > 0) // Ascent
                    {
                        gridCells[firstRowCell + 5].count++;
                        gridCells[firstRowCell + 5].val += step;
                        gridColumns[5].count++;
                        gridColumns[5].val += step;
                    }
                    else // Descent
                    {
                        gridCells[firstRowCell + 6].count++;
                        gridCells[firstRowCell + 6].val += step;
                        gridColumns[6].count++;
                        gridColumns[6].val += step;
                    }
                    lastEle += step;
                }
                else // Flat
                {
                    gridCells[firstRowCell + 4].count++;
                    gridCells[firstRowCell + 4].val++;
                    gridColumns[4].count++;
                    gridColumns[4].val++;
                }
            }

            // slopes uphill, downhill - not implemented yet
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
            gridCells[firstRowCell + 4].count++; // First point is flat
            gridCells[firstRowCell + 4].val++;
            gridColumns[4].count++;
            gridColumns[4].val++;
        }
        lastTrkpt = &trkpt;
    }

    for(qint32 i = 0; i < gridCells.size(); i++)
    {
        struct gridCell_t &gridCell = gridCells[i];

        qint32 column = i - qFloor(i / gridColumns.size()) * gridColumns.size();

        if ((gridCell.option & CHeartRateZonesDialog::eOptAveradge) && gridCell.count)
        {
            gridCell.val /= gridCell.count; // Set averadge
        }

        if (column == 3 && gridCells[i - 1].val) // Compute speed for the cell
        {
             gridCell.val = gridCells[i - 2].val / gridCells[i - 1].val;
        }

        qreal percent = gridColumns[column].val ? gridCell.val / gridColumns[column].val * 100 : 0;
        gridCell.percentBar->setValues(percent, gridCell.val);
    }

    labelTotalPoints->setText(QString("%L1").arg(gridColumns[0].count)); // Points

    QString text, unit;
    IUnit::self().meter2distance(gridColumns[1].val, text, unit); // Length
    labelTotalLength->setText(QString("%1%2").arg(text).arg(unit));

    IUnit::self().seconds2time(gridColumns[2].val, text, unit);  // Moving time
    labelTotalMoving->setText(QString("%1%2").arg(text).arg(unit));

    if (gridColumns[1].val) // Speed
    {
        IUnit::self().meter2speed(gridColumns[3].val, text, unit);
        labelSpeedMax->setText(QString(tr("Max:") + " %1%2").arg(text).arg(unit));

        qreal avgSpeed = gridColumns[1].val / gridColumns[2].val;
        IUnit::self().meter2speed(avgSpeed, text, unit);
        labelSpeedAvg->setText(QString(tr("Avg:") + " %1%2").arg(text).arg(unit));
    }

    IUnit::self().meter2elevation(gridColumns[5].val, text, unit); // Ascent
    labelTotalAscent->setText(QString("%1%2").arg(text).arg(unit));

    IUnit::self().meter2elevation(gridColumns[6].val, text, unit); // Descent
    labelTotalDescent->setText(QString("%1%2").arg(text).arg(unit));

    qDebug() << "trkHeartBeats :" << trkHeartBeats;
    qDebug() << "trkHeartBeats avg:" <<  trkHeartBeats / gridColumns[2].val * 60;
    qDebug() << "Fitness Rate:" << trk.getEnergyCycling().getEnergyUseCycling() * 4.1868 / trkHeartBeats;

    labelMinHr->setText(QString("%1").arg(minTrkHr, 0, 'f', 1) + tr("bpm"));
    labelMaxHr->setText(QString("%1").arg(maxTrkHr, 0, 'f', 1) + tr("bpm"));
    labelAvgHr->setText(QString("%1").arg(trkHeartBeats / gridColumns[2].val * 60, 0, 'f', 1) + tr("bpm"));
    labelHeartBeats->setText(QString("%L1").arg(trkHeartBeats, 0, 'f', 0));

    qreal energyUseCycling = trk.getEnergyCycling().getEnergyUseCycling();
    if (energyUseCycling != NOFLOAT)
    {
        labelJouleHb->setText(QString("%L1").arg(trk.getEnergyCycling().getEnergyUseCycling() * 4.1868 / trkHeartBeats, 0, 'f', 4));
    }

    update();
}

void CHeartRateZonesDialog::slotShowHelp()
{
    QString msg = tr("<p><b>Set Energy Use for Cycling</b></p>"
                     "<p>Within this dialog your personal energy use (consumption) for a cycling tour can be computed.</p>"
                     "<p>The computed value of \"Energy Use Cycling\" can be see as an indicator for the exertion of a cycling tour.</p>"
                     "<p>The tour length, speed and slope values will be taken into account.</p>"
                     "<p>To individualize your personal energy use the following input data are more needed:"
                     "<ul>"
                     "<li>Driver and bicyle weight</li>"
                     "<li>Air density, wind speed and position to the wind to consider the wind drag resistance</li>"
                     "<li>Ground situation (tyre and ground) to consider the rolling resistance</li>"
                     "<li>Average pedal cadence for the computation of pedal force</li>"
                     "</ul></p>"
                     "<p>The individualize data will be defined in this dialog and more computed values will be shown here.</p>"
                     "<p>When loading older tracks or switching in history to tracks with a different parameter set compared to the previous saved parameter set"
                     ", the shown parameter set in this dialog can be replaced by the previous saved parameter set."
                     "<p>The energy use in unit \"kcal\" will be stored in the track (qms format only) and can be remove later on when no longer needed.</p>"
                     "<p>For more information see tooltips on input and output values.</p>");

    QMessageBox::information(CMainWindow::getBestWidgetForParent(), tr("Help"), msg);
}

// class CPercentBar
void CHeartRateZonesWidget::paintEvent(QPaintEvent *)
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

    if (!(option & CHeartRateZonesDialog::eOptNoPercent))
    {
        painter.drawLine(percent + 4 - 0.5, ascent + descent, percent + 4 - 0.5, ascent + descent + 5);
        const QString percentStr = QString("%1%").arg(percent, 0, 'f', 1);
        qreal percentPos = adjustTextPosition(percentStr, painter);
        painter.drawText(percentPos + 4, ascent, percentStr);
    }
/*
    if (valFormat != valFormat_e::none)
    {
        painter.drawLine(percent + 4 - 0.5, ascent + descent + 5 + 20, percent + 4 - 0.5, ascent + descent + 5 + 20 + 5);
    }
*/

    if (!(option & CHeartRateZonesDialog::eOptNoVal))
    {
        painter.drawLine(percent + 4 - 0.5, ascent + descent + 5 + 20, percent + 4 - 0.5, ascent + descent + 5 + 20 + 5);

        QString str, unit, valStr;
        switch (CHeartRateZonesDialog::eOptAllFormats & option)
        {
        case CHeartRateZonesDialog::eOptInt:
        {
            valStr = QString("%L1").arg(val, 0, 'f', 0);
            break;
        }
        case CHeartRateZonesDialog::eOptLength:
        {
            IUnit::self().meter2distance(val, str, unit);
            valStr = QString("%1%2").arg(str).arg(unit);
            break;
        }
        case CHeartRateZonesDialog::eOptTime:
        {
            IUnit::self().seconds2time(val, str, unit);
            valStr = QString("%1%2").arg(str).arg(unit);
            break;
        }
        case CHeartRateZonesDialog::eOptSpeed:
        {
            IUnit::self().meter2speed(val, str, unit);
            valStr = QString("%1%2").arg(str).arg(unit);
            break;
        }
        case CHeartRateZonesDialog::eOptElevation:
        {
            IUnit::self().meter2elevation(val, str, unit);
            valStr = QString("%1%2").arg(str).arg(unit);
            break;
        }
        }
        qreal valStrPos = adjustTextPosition(valStr, painter);
        painter.drawText(valStrPos, ascent + descent + 5 + 20 + 5 + ascent, valStr);
    }
}

qreal CHeartRateZonesWidget::adjustTextPosition(const QString &valueStr, QPainter &painter)
{
    const qreal width = QFontMetrics(painter.font()).boundingRect(valueStr).width();

    qreal textPosition = ((percent - width / 2) < 0) ? 0 : percent - width / 2;
    textPosition = ((percent + width / 2) > 100) ? 100 - width : textPosition;

    return textPosition;
}

