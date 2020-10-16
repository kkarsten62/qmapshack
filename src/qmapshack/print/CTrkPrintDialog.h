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

#ifndef CTRKPRINTDIALOG_H
#define CTRKPRINTDIALOG_H

#include "ui_ITrkPrintDialog.h"
#include <QPrinter>

class CGisItemTrk;
//class CTrackData;
class CCanvas;

class CTrkPrintDialog : public QDialog, private Ui::ITrkPrintDialog
{
    Q_OBJECT

public:
    CTrkPrintDialog(QWidget *parent, CGisItemTrk &trk);
    virtual ~CTrkPrintDialog();

protected:
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void slotUpdateMetrics();
    void slotPrint();
    void slotSetPrinter();
    void slotSetOverlap(qint32 ovl);
    void slotLoadPdfFile();
    void slotScaleBar(bool checked);
    void slotPageMarkers(bool checked);
    void slotDistanceMarker(int value);

private:
    void addPageMarkers(QPainter &p, const QRectF &currPage, const QRectF &otherPage, qint32 pageNo);
    void addDistanceMarkers(QPainter &p, const QRectF &currPage);

    void setPdfFileExists();

    struct page_t : public QRectF
    {
        QPointF centerRad; // The center in rad, to be stored prior to canvas->print
        QPageLayout::Orientation orientation; // Landscape or portrait
        qreal distanceStart; // The trk start distance in km for the page
        qreal distanceEnd;  // The trk end distance in km for the page
    };
    QList<struct page_t> pages; // List of all pages

    struct pt_t : public QPointF
    {
        QRectF bb; // Boundingbox of trk in pixel up to this trkpt
        struct page_t page; // The page in screen pixel coordinate system, set for for best fit for the pt boundingBox
        qreal distance; // The trk distance to this pt
    };

    struct distanceMarkerPt_t : public QPointF
    {
        qreal km;
        QLineF lineToText;
    };
    QList<struct distanceMarkerPt_t> distanceMarkerPts;

    CGisItemTrk &trk;
    CCanvas *canvas;
    QPrinter printer;
    qreal scale;
    qreal overlap; // Overlap for sequentially pages in meter on page
    bool printScaleBar = false;
    bool printPageMarkers = true;

    const QList<qint32> distanceMarkers {0, 5, 10, 15, 20, 25, 35, 50};
    qint32 distanceMarker;

};

#endif // CTRKPRINTDIALOG_H
