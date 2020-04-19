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
//#include <QtPrintSupport>

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

private:
    void addPageMarkers(QPainter &p, const QRectF &currPage, const QRectF &otherPage, qint32 pageNo);

    struct pt_t : public QPointF
    {
        QRectF bb; // Boundingbox of trk in pixel up to this trkpt
        QRectF pageScreen; // The page in screen pixel coordinate system, set for for best fit for the pt boundingBox
    };

    struct page_t
    {
        QPointF center;
        QPageLayout::Orientation orientation;
        QRectF pageScreen;
    };
    QList<struct page_t> pages;

    CGisItemTrk &trk;
    CCanvas *canvas;
    QPrinter printer;

    qreal scale;
    qreal overlap = 0.01; // Overlap for sequentially pages in meter on page
};

#endif // CTRKPRINTDIALOG_H
