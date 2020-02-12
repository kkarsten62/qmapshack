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

#ifndef CHEARTRATEZONESDIALOG_H
#define CHEARTRATEZONESDIALOG_H

#include "ui_IHeartRateZonesDialog.h"

class CGisItemTrk;
class CTrackData;
class CHeartRateZonesWidget;

class CHeartRateZonesDialog : public QDialog, private Ui::IHeartRateZonesDialog
{
    Q_OBJECT

public:
    CHeartRateZonesDialog(QWidget *parent, const CGisItemTrk &trk);
    virtual ~CHeartRateZonesDialog();

private slots:
    void slotSetMaxHr(qint32 maxHr);
    void slotShowHelp();

private:
    const CGisItemTrk &trk;
    qint32 maxHr;
    qreal minTrkHr = 220.;
    qreal maxTrkHr = 0.;
    qreal trkHeartBeats = 0;

    void initZoneItems();

    struct gridRow_t
    {
        QColor color;
        qint32 minPercent;
        qint32 maxPercent;
        qreal minHr;
        qreal maxHr;
    };
    QList<gridRow_t> gridRows =
    {
        {QColor("#33a6cc"),  0,  60, 0, 0},
        {QColor("#9ACD32"), 60,  70, 0, 0},
        {QColor("#FFD700"), 70,  80, 0, 0},
        {QColor("#FFA500"), 80,  90, 0, 0},
        {QColor("#CC0000"), 90, 100, 0, 0}
    };

    struct gridColumn_t
    {
        qreal val;
        qint32 count;
        void init()
        {
            val = 0;
            count = 0;
        }
    };
    QList<gridColumn_t> gridColumns =
    {
        {0, 0}, // Track points
        {0, 0}, // Moving
        {0, 0}, // Flat
        {0, 0}, // Descent
        {0, 0}  // Ascent
    };

    struct gridCell_t
    {
        qreal val;
        qint32 count;
        CHeartRateZonesWidget *percentBar;
        void init()
        {
            val = 0;
            count = 0;
        }
    };
    QList<struct gridCell_t> gridCells;

    void setRowsHrLabel();
    void computeCells();
};

class CHeartRateZonesWidget : public QWidget
{
    Q_OBJECT

public:
    enum valFormat_e
    {
        none        = 0x00000001
        , integer   = 0x00000002
        , time      = 0x00000004
        , elevation = 0x00000008
        , degree    = 0x00000010
    };

    CHeartRateZonesWidget(valFormat_e valFormat, const QColor &color) :
        valFormat(valFormat)
      , color(color) {}

    void setValues(qreal percent, qreal val) { this->percent = percent; this->val = val; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    valFormat_e valFormat;
    const QColor &color;
    qreal percent = 0;
    qreal val = 0;

    qreal adjustTextPosition(const QString &valueStr, QPainter &painter);
};

#endif // CHEARTRATEZONESDIALOG_H
