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

class CHeartRateZonesDialog : public QDialog, private Ui::IHeartRateZonesDialog
{
    Q_OBJECT

public:
    CHeartRateZonesDialog(QWidget *parent, const CGisItemTrk &trk);
    virtual ~CHeartRateZonesDialog();

    enum columnType_e
    {
        eIsPercent    = 0x00000001
        , eIsSum      = 0x00000002
        , eIsAveradge = 0x00000004
        , eFlagSubpt  = 0x00000008
    };

    struct zoneItem_t
    {
        qint32 count = 0;
        qreal sum = 0;
        void init()
        {
            count = 0;
            sum = 0;
        }
    };

    struct zoneRow_t
    {
        qreal from;
        qreal to;
    };

    struct zoneColumn_t
    {
        columnType_e columnType;
        bool showValue;
        bool showPercent;
    };

    const qint32 numOfRows = 5;
    const qint32 numOfColumns = 5;

    QList<qreal> zones = {60, 70, 80, 90};
    QList<zoneRow_t> zoneRows = { {0, 50}, {50, 60} };
    QList<zoneColumn_t> zoneColumns = { {eIsPercent, true, true}, {eIsSum, true, true} };
    zoneItem_t zoneItems[5][5];

private slots:

    void slotSetMaxHeartRate(qint32 maxHeartRate);
    void slotApply(bool);

private:
    QHash<QString, QColor> colorList =
    {
          {"moderate",  QColor("#33a6cc")}
        , {"fitness",   QColor("#9ACD32")}
        , {"aerobic",   QColor("#FFD700")}
        , {"anaerobic", QColor("#FFA500")}
        , {"warning",   QColor("#CC0000")}
    };
    const CGisItemTrk &trk;
    qint32 maxHeartRate;

    void initZoneItems();

    struct gridRow
    {
        QColor color;
        qint32 minPercent;
        qint32 maxPercent;
        qint32 minHR;
        qint32 maxHR;
    };
    QList <gridRow> gridRows =
    {
        {QColor("#33a6cc"),  0,  60, 0, 0},
        {QColor("#9ACD32"), 60,  70, 0, 0},
        {QColor("#FFD700"), 70,  80, 0, 0},
        {QColor("#FFA500"), 80,  90, 0, 0},
        {QColor("#CC0000"), 90, 100, 0, 0}
    };


    struct gridColumn
    {
        QString name;
        qreal total;
    };
    QList <gridColumn> gridColumns =
    {
        {tr("Track Points"), 0},
        {tr("Moving Time"), 0},
        {tr("Descent"), 0},
        {tr("Ascent"), 0}
    };

    struct gridCell
    {
        qreal cellVal;
        qreal cellPercent;
    };

    struct grid
    {
    };



protected:
    void paintEvent(QPaintEvent *) override;
};

class CPercentBar : public QWidget
{
    Q_OBJECT

public:
    CPercentBar(const CHeartRateZonesDialog::columnType_e &type
                , const QColor &color
                , CHeartRateZonesDialog *parent) : type(type), color(color), parent(parent) {}

protected:
    void paintEvent(QPaintEvent *) override;

private:
    const CHeartRateZonesDialog::columnType_e &type;
    const QColor &color;
    CHeartRateZonesDialog *parent;
    qreal value = 45;

    qreal adjustTextPosition(const QString &valueStr);
};





#endif // CHEARTRATEZONESDIALOG_H
