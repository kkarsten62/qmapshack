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
#include "gis/trk/CKnownExtension.h"
#include "helpers/CDraw.h"
#include "helpers/CProgressDialog.h"
#include "helpers/CSettings.h"
#include "plot/CPlotTrack.h"
#include "print/CTrkPrintDialog.h"

#include <QPrintDialog>


CTrkPrintDialog::CTrkPrintDialog(QWidget *parent, CGisItemTrk &trk) :
    QDialog(parent)
    , trk(trk)
{
    setupUi(this);

//    SETTINGS;
//    maxHr = cfg.value("TrackDetails/HeartRateZones/maxHeartRate", 180).toInt();
//    spinMaxHr->setValue(maxHr);

    QList<CCanvas*> list = CMainWindow::self().getCanvas();

    CCanvas *source = nullptr;
        if (list.size())
    {
        source = list.at(0);
    }
    else
    {
        // Dialog
        rejected();
    }
    // clone canvas by a temporary configuration file
    QTemporaryFile temp;
    temp.open();
    temp.close();

    QSettings view(temp.fileName(), QSettings::IniFormat);
    view.clear();

    source->saveConfig(view);

    canvas = new CCanvas(this, "print trk pages");
    canvas->loadConfig(view);
    canvas->show();
    canvas->allowShowTrackOverlays(false);

    // add canvas canvas to dialog
    QLayout * layout = new QVBoxLayout(frameCanvas);
    layout->addWidget(canvas);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    connect(canvas,         &CCanvas::sigZoom,     this, &CTrkPrintDialog::slotUpdateMetrics);
    connect(canvas,         &CCanvas::sigMove,     this, &CTrkPrintDialog::slotUpdateMetrics);
    connect(pushPrint,      &QPushButton::pressed, this, &CTrkPrintDialog::slotPrint);
}

CTrkPrintDialog::~CTrkPrintDialog()
{
//    SETTINGS;
//    cfg.setValue("TrackDetails/HeartRateZones/maxHeartRate", spinMaxHr->value());
}

void CTrkPrintDialog::resizeEvent(QResizeEvent * e)
{
    qDebug() << "resizeEvent";
    QDialog::resizeEvent(e);
    slotUpdateMetrics();
}

void CTrkPrintDialog::slotUpdateMetrics()
{
    qDebug() << "slotUpdateMetrics";
    QTime timer;
    timer.start();

    QRectF trkRectRad = trk.getBoundingRect();

    qreal trkRectDxMeter = GPS_Math_Distance(trkRectRad.left(), trkRectRad.bottom(), trkRectRad.right(), trkRectRad.bottom());
    qreal trkRectDyMeter = GPS_Math_Distance(trkRectRad.left(), trkRectRad.bottom(), trkRectRad.left(), trkRectRad.top());

    QPointF trkRectPt1(trkRectRad.left(), trkRectRad.bottom());
    QPointF trkRectPt2(trkRectRad.right(), trkRectRad.top());

    canvas->convertRad2Px(trkRectPt1); // Bottom left in pixel
    canvas->convertRad2Px(trkRectPt2); // Top right in pixel
    qreal trkRectPxDx = trkRectPt2.x() - trkRectPt1.x(); // Trk bounding box width in pixel
    qreal trkRectPxDy = trkRectPt2.y() - trkRectPt1.y(); // Same for height

    qreal scaleTrkMeter = qPow(trkRectDxMeter, 2) + qPow(trkRectDyMeter, 2); // Diagonal of bounding box in meter
    qreal scaleTrkPx = qPow(trkRectPxDx, 2) + qPow(trkRectPxDy, 2); // // Diagonal of bounding box in pixel
    qreal scaleTrk;
    if (scaleTrkPx)
    {
        scaleTrk = qSqrt(scaleTrkMeter / scaleTrkPx); // Meter per pixel on trk (screen)
        //        qDebug() << "scaleTrk=" << scaleTrk << " Meter per pixel on trk";
    }
    else
    {
        qDebug() << "scaleTrkPx is 0";
        return;
    }

    QRectF pageRectMeter  = printer.pageRect(QPrinter::Millimeter);
    QRectF pageRectPx  = printer.pageRect(QPrinter::DevicePixel);
    qreal scalePageMeter = qPow(pageRectMeter.width() / 1000.0, 2) + qPow(pageRectMeter.height() / 1000.0, 2); // Diagonal of paper in meter
    qreal scalePagePx = qPow(pageRectPx.width(), 2) + qPow(pageRectPx.height(), 2);  // Diagonal of paper in pixel
    qreal scalePage;
    if (scalePagePx)
    {
         scalePage = qSqrt(scalePageMeter / scalePagePx); // Meter per pixel on paper
         //    qDebug() << "scalePage=" << scalePage << " Meter per pixel on paper";
    }
    else
    {
        qDebug() << "scalePagePx is 0";
        return;
    }

    scale = scaleTrk / scalePage; // One meter on paper equals to X meter on screen (trk)

    qreal overlapPx = overlap / scalePage; // Overlap converted from meter to pixel
    //        qDebug() << "overlap of=" << overlap * 1000 << "mm means=" << overlapPx << "pixel ==> means=" << overlapPx * scaleTrk<< "m on screen";

    qreal pageLandscape = pageRectPx.width(); // Page size in pixel
    qreal pagePortrait = pageRectPx.height();
    if (pagePortrait > pageLandscape) // Longer side should always be landscape
    {
        qSwap(pageLandscape, pagePortrait);
    }

    // Build helper list of pixel points from track
    QList<struct pt_t> pts;
    quint32 invalidMask = (trk.getAllValidFlags() & CTrackData::trkpt_t::eValidMask) << 16;
    CTrackData::trkpt_t const *lastTrkpt  = nullptr;
    for(const CTrackData::trkpt_t &trkpt : trk.getTrackData())
    {
        if(trkpt.isInvalid(CTrackData::trkpt_t::invalid_e(invalidMask)) || trkpt.isHidden())
        {
            continue;
        }

        struct pt_t pt;

        pt.setX(trkpt.lon * DEG_TO_RAD);
        pt.setY(trkpt.lat * DEG_TO_RAD);
        canvas->convertRad2Px(pt); // Convert x and y of this point to pixel

        if(lastTrkpt != nullptr)
        {
            const struct pt_t ptLast = pts.last(); // last pt in pixel
            pt.bb = QRectF(ptLast, pt).normalized(); // Build boundingBox of this point to previous point
        }

        pts << pt;
        lastTrkpt = &trkpt;
    }

    // Iterate over helper data
    pages.clear();
    QRectF bbPages; // BoundingBox of all pages, to be used for printing
    qreal cutOffset = 0.1; // Offset to move the page a bit away from boundingBox to ensure intersection of both rects
//    qDebug() << "cutOffset=" << cutOffset << " means " << cutOffset * scaleTrk << " meter on track";

    QLineF pageVev(pts.first(), pts.first()); // Steers the track direction to position the page on track boundingBox
    for(qint32 i = 1; i < pts.size(); ++i) // Start at 2nd points to build first line
    {
        struct pt_t &pt = pts[i]; // current pixel point
        struct pt_t &prevPt = pts[i - 1]; // Previous pixel point

        pageVev.setP2(pt); // Set direction from first to the last trkpt on the page

        pt.bb |= prevPt.bb; // Unit the boundingBox of last line with the boundingBox of previous boundingBox for current page
        pt.bb = pt.bb.normalized(); // Always work on normalized rects

        qreal pageX, pageY; // Page dimension used for best fit orientation
        QPageLayout::Orientation orientation;
        if (qAbs(pt.bb.height()) > qAbs(pt.bb.width())) // Set optimal orientation for page
        {
            pageX = pagePortrait;
            pageY = pageLandscape;
            orientation = QPageLayout::Portrait;
        }
        else
        {
            pageX = pageLandscape;
            pageY = pagePortrait;
            orientation = QPageLayout::Landscape;
        }

        // Build the page on screen around the track
        pt.pageScreen = QRectF(pt.bb.left() - cutOffset, pt.bb.top() - cutOffset, pageX - 2 * overlapPx, pageY - 2 * overlapPx); // Build page on screen in normalized form
        qreal dx = (pageVev.dx() < 0) ? (-pt.pageScreen.width() + pt.bb.width() + 2 * cutOffset) : 0; // Move line based on trk direction in page
        qreal dy = (pageVev.dy() < 0) ? (-pt.pageScreen.height() + pt.bb.height() + 2 * cutOffset) : 0;
        pt.pageScreen.translate(dx, dy); // Move it

        if(prevPt.pageScreen.isNull()) // First line on page: Special treatment when first line on page is longer then page dimensions
        {
            prevPt.pageScreen = pt.pageScreen; // Take the boundingBox from the next pt
        }

        if(!pt.pageScreen.contains(pt)) // check whether pixel pt is inside current page rect on screen, if outside try to find intersection of trk line and page on screen
        { // Trkpt is outside the page
            QLineF trkLine = QLineF(pt, prevPt); // Last trk line in pixel

            QList<QPointF> pagePts; // To store the 5 points from the page on screen
            pagePts = QPolygonF(prevPt.pageScreen).toList(); // Convert previous page on screen into polyline to get page pts
            bool hasIntersection = false; // There must be an intersection, if not set debug message
            for (int j = 1; j < pagePts.size(); ++j) // Check for intersection of trk line against previous page rect
            {
                QLineF pageLine = QLineF(pagePts[j - 1], pagePts[j]); // Build line from page rect
                QPointF ipt; // Intersection point
                QLineF::IntersectType type = pageLine.intersect(trkLine, &ipt); // Try to find an intersection

                if (type == QLineF::BoundedIntersection) // There is an intersection
                {
                    // Fit page on screen to the real cutted trk boundingBox
                    QRectF bb2Ipt = QRectF(prevPt, ipt).normalized(); // BoundingBox of trk line up to intersection point
                    bb2Ipt |= prevPt.bb; // Add to the previous boundingBox
                    bb2Ipt = bb2Ipt.normalized();
                    prevPt.pageScreen.moveCenter(bb2Ipt.center()); // Adjust page on screen the to the center of cutted boundingBox

                    QPointF center = prevPt.pageScreen.center(); // in pixel
                    canvas->convertPx2Rad(center); // convert to rad to be used for printing

                    struct page_t page; // Build a new page
                    page.orientation = orientation;
                    page.pageScreen = prevPt.pageScreen.marginsAdded(QMarginsF(overlapPx, overlapPx, overlapPx, overlapPx)); // Add the overlap to the page for printing
                    page.center = center; // In Rad
                    pages << page;

                    bbPages |= page.pageScreen; // Add to the boundingBox of all Pages, used for printing

                    pt.bb = QRectF(ipt, pt).normalized(); // Adjust the bb for the current pt up to the intersection pt

                    struct pt_t newPt; // Build a new pt in pixel to be used for next loop
                    newPt.setX(ipt.x());
                    newPt.setY(ipt.y());
                    newPt.bb = QRectF(ipt, QSizeF(0, 0)); // New page start with an empty boundingBox
                    pts.insert(i, newPt);
                    pageVev = QLineF(ipt, ipt); // Set page vector to null for next page
                    hasIntersection = true; // Yeap, an intersection is found
                }
            }
            if (!hasIntersection)
            {
                qDebug() << "Point is outside but no intersection found, this should not happend!";
            }
        }
        else if (i == (pts.size() - 1)) // For last trkpt, build last page or for fullscreen on larger scale ==> one page
        {
            QRectF pageScreen = QRectF(0, 0, pageX, pageY); // Create a new page
            pageScreen.moveCenter(pt.bb.center()); // And move it to the center of screen page
            QPointF center = pageScreen.center(); // Convert to rad
            canvas->convertPx2Rad(center);

            struct page_t page; // Build new page
            page.orientation = orientation;
            page.center = center;
            page.pageScreen = pageScreen;
            pages << page;
            bbPages |= page.pageScreen; // Add to the boundingBox of all Pages
         }
    }

    // Update GUI
    QString labelScaleInfoStr(tr("Zoom with mouse wheel on the map left<br>to change the printing scale.<br><br>"));
    labelScaleInfoStr += (tr("Scale of approx. 1:%L1<br>")).arg(qRound(scale));
    labelScaleInfoStr += (tr("1cm on paper page equals %L1m in reality.<br><br>")).arg(qRound(scale / 100));
    labelScaleInfoStr += (tr("Be aware: <b>%L1 pages</b> will be needed to print the track.")).arg(pages.size());
    labelScaleInfo->setText(labelScaleInfoStr);

    // Print scene on image
    QSize size = labelPlotTrack->size();
    QImage image(size.width(), size.height(), QImage::Format_ARGB32);
    QPainter p;
    p.begin(&image);
    USE_ANTI_ALIASING(p, true);
    //    p.drawRect(0, 0, size.width() - 1, size.height() - 1);

    qreal w1 = bbPages.width();
    qreal h1 = bbPages.height();
    qreal w2 = size.width() - 10;
    qreal h2 = size.height() - 10;

    qreal scaleX = w1 / w2; //
    qreal scaleY = h1 / h2;
    qreal scale = qMax(scaleX, scaleY); // Use the maximum scale to fit in GUI rect
    qreal offX = (w2 - w1 / scale) / 2; // Offset to fit in the center of the GUI rect
    qreal offY = (h2 - h1 / scale) / 2;

    p.setBrush(Qt::cyan); // Print the pages
    p.setPen(Qt::darkCyan);
    p.setOpacity(0.7); // Some opacity to see the prevoius page
    qint32 pageNo = 0;
    for(const struct page_t page : pages)
    {
        qreal p1x = 5 + offX + (page.pageScreen.x() - bbPages.x()) / scale; // 5 pixel margin
        qreal p2y = 5 + offY + (page.pageScreen.y() - bbPages.y()) / scale;
        p.drawRect(p1x, p2y,
                   page.pageScreen.width() / scale, page.pageScreen.height() / scale);
    }

    p.setOpacity(1); // Print the track, no opacity
    p.setPen(QPen(Qt::darkBlue, 2));
    QPolygonF polyLine;
    for(const struct pt_t pt : pts)
    {
        qreal ptx = 5 + offX + (pt.x() - bbPages.x()) / scale;
        qreal pty = 5 + offY + (pt.y() - bbPages.y()) / scale;
        polyLine << QPointF(ptx, pty);
    }
    p.drawPolyline(polyLine);

    p.setPen(QPen());  // Print the pageNo
    if (pages.size() <= 50) // Max 50 pageNo shown
    {
        for(const struct page_t page : pages)
        {
            QPointF center = page.pageScreen.center(); // Center in pixel
            qreal p1x = 5 + offX + (center.x() - bbPages.x()) / scale - 10; // 10 text rect 20x20 pixel
            qreal p1y = 5 + offY + (center.y() - bbPages.y()) / scale - 10;
            p.drawText(QRectF(p1x, p1y, 20, 20), Qt::AlignCenter, QString("%1").arg(++pageNo)); // Print the page number in center of page
        }
    }
    p.end();

    labelPlotTrack->setPixmap(QPixmap::fromImage(image)); // Assign the img to the GUI

//    qDebug("Time elapsed: %d ms", timer.elapsed());
}

void CTrkPrintDialog::slotPrint()
{
    qDebug() << "Print";
    printer.setOutputFileName("/home/karl/print11.pdf");
    QPainter p;
    if (pages.size())
    {
        printer.setPageOrientation(pages[0].orientation); // To be set before p.begin for the first page!
    }
    //    printer.setResolution(300);
    p.begin(&printer);
    USE_ANTI_ALIASING(p, true);
    int n = 0;
    PROGRESS_SETUP(tr("Printing pages."), 0, pages.size(), this);
    qint32 pageNo = 0;

    for(qint32 i = 0; i < pages.size(); ++i)
    {
        const struct page_t &currPage = pages[i];
        const struct page_t &prevPage = (i > 0) ? pages[i - 1] : currPage;  // prevPage and nextPage, to be used to set the page marker on current page
        const struct page_t &nextPage = (i < (pages.size() - 1)) ? pages[i + 1] : currPage;
        if(i > 0) // Set the right orientation before setting a new page and start printing on the page
        {
            printer.setPageOrientation(currPage.orientation); // Change of orientation right before newPage
            printer.newPage();
        }
        QRectF pageToPrint = QRectF(0, 0, currPage.pageScreen.width(), currPage.pageScreen.height()); // The paper page
        p.setClipRect(pageToPrint);

//        QPointF center = QPointF(currPage.center); // Convert to rad
//        canvas->convertPx2Rad(center);
        canvas->print(p, pageToPrint, currPage.center, true); // Print the canvas content
        // Draw the scale and page number to bottom right
        p.drawText(QRectF(currPage.pageScreen.width() - 245, currPage.pageScreen.height() - 25, 200, 20),
                   Qt::AlignRight | Qt::AlignBottom, QString("1cm=%L1m | %2").arg(scale / 100, 0, 'f', 0).arg(++pageNo));

        if (i < (pages.size() - 1)) // Print page markers from the next page
        {
            addPageMarkers(p, currPage.pageScreen, nextPage.pageScreen, pageNo + 1);
        }
        if (i > 0) // Print page markers from the previous page
        {
            addPageMarkers(p, currPage.pageScreen, prevPage.pageScreen, pageNo - 1);
        }

        PROGRESS(++n, break);
        //        if (i == 3) break;

    }
    p.end();
}

void CTrkPrintDialog::addPageMarkers(QPainter &p, const QRectF &currPage, const QRectF &otherPage, qint32 pageNo)
{
    QList<QPointF> otherPts = QPolygonF(otherPage).toList(); // Convert into polyline to get page pts
    for (qint32 i = 0; i < otherPts.size() - 1; ++i) // Check for intersection for the other point on current page
    {
        if(currPage.contains(otherPts[i])) // Other page point is in the current page
        {
            QPointF currPt = otherPts[i] - currPage.topLeft(); // Adjust coordinates of other point to the 1st page
            QPointF nextPt = otherPts[i + 1] - currPage.topLeft();
            QLineF lineNext = QLineF(currPt, nextPt); // Build next line
            lineNext.setLength(20); // And shorten the line
            p.drawLine(lineNext); // Draw with the painter

            qint32 prev = (i == 0) ? otherPts.size() - 2 : i - 1; // Special for first point, prev point is the last point of rect
            QPointF prevPt = otherPts[prev] - currPage.topLeft();
            QLineF linePrev = QLineF(currPt, prevPt); // Build previous line
            linePrev.setLength(20);
            p.drawLine(linePrev);

            p.drawText(QRectF(linePrev.p2(), lineNext.p2()), Qt::AlignCenter, QString("%1").arg(pageNo)); // Print the page number between the marker lines
            p.setBrush(Qt::cyan); // And a small filled circle in the origin
            p.drawEllipse(currPt, 3, 3);
        }
    }
}
