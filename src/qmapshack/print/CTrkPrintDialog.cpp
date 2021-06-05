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
//    , printer(QPrinter::HighResolution)
{
    setupUi(this);

    setWindowTitle(tr("Print Track"));

    CCanvas* source = CMainWindow::self().getCanvas().at(0);

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

    for (qint32 distanceMarker : distanceMarkers) // Fill the distanceMarker combo
    {
        if (distanceMarker)
        {
            comboDistanceMarker->addItem(QString("%1km").arg(distanceMarker));
        }
        else
        {
            comboDistanceMarker->addItem(QString(tr("0 - None")));
        }
    }

    connect(canvas,              &CCanvas::sigZoom,     this, &CTrkPrintDialog::slotUpdateMetrics);
    connect(canvas,              &CCanvas::sigMove,     this, &CTrkPrintDialog::slotUpdateMetrics);
    connect(pushSetPrinter,      &QPushButton::pressed, this, &CTrkPrintDialog::slotSetPrinter);
    connect(spinOverlap,         SIGNAL(valueChanged(int)), this, SLOT(slotSetOverlap(int)));
    connect(checkScaleBar,       &QCheckBox::toggled,   this, &CTrkPrintDialog::slotScaleBar);
    connect(checkPageMarkers,    &QCheckBox::toggled,   this, &CTrkPrintDialog::slotPageMarkers);
    connect(comboDistanceMarker, SIGNAL(activated(int)), this, SLOT(slotDistanceMarker(int)));
    connect(pushPrint,           &QPushButton::pressed, this, &CTrkPrintDialog::slotPrint);
    connect(pushLoadPdfFile,     &QPushButton::pressed, this, &CTrkPrintDialog::slotLoadPdfFile);

    SETTINGS;
    overlap = cfg.value("Print/Trk/overlap", 0.005).toDouble();
    QString outputFileName = cfg.value("Print/Trk/outputFileName", "").toString();
    QPageSize::PageSizeId pageSize = (QPageSize::PageSizeId)cfg.value("Print/Trk/pageSize", QPageSize::A4).toInt();
    qreal left = cfg.value("Print/Trk/marginLeft", 3.0).toDouble();
    qreal top = cfg.value("Print/Trk/marginTop", 3.0).toDouble();
    qreal right = cfg.value("Print/Trk/marginRight", 3.0).toDouble();
    qreal bottom = cfg.value("Print/Trk/marginBottom", 3.0).toDouble();
    printScaleBar = cfg.value("Print/Trk/printScaleBar", false).toBool();
    printPageMarkers = cfg.value("Print/Trk/printPageMarkers", true).toBool();
    distanceMarker = cfg.value("Print/Trk/distanceMarker", 0).toInt();

    spinOverlap->setValue(overlap * 1000);
    printer.setOutputFileName(outputFileName);
    printer.setPageSize((QPagedPaintDevice::PageSize)pageSize);
    printer.setPageMargins(left, top, right, bottom, QPrinter::Millimeter);
    checkScaleBar->setChecked(printScaleBar);
    checkPageMarkers->setChecked(printPageMarkers);
    comboDistanceMarker->setCurrentIndex(distanceMarker);

    setPdfFileExists();
}

CTrkPrintDialog::~CTrkPrintDialog()
{
    SETTINGS;
    cfg.setValue("Print/Trk/overlap", overlap);
    cfg.setValue("Print/Trk/outputFileName", printer.outputFileName());
    cfg.setValue("Print/Trk/pageSize", printer.pageSize());
    QMarginsF margins = printer.pageLayout().margins(QPageLayout::Millimeter);
    cfg.setValue("Print/Trk/marginLeft", margins.left());
    cfg.setValue("Print/Trk/marginTop", margins.top());
    cfg.setValue("Print/Trk/marginRight", margins.right());
    cfg.setValue("Print/Trk/marginBottom", margins.bottom());
    cfg.setValue("Print/Trk/printScaleBar", printScaleBar);
    cfg.setValue("Print/Trk/printPageMarkers", printPageMarkers);
    cfg.setValue("Print/Trk/distanceMarker", distanceMarker);
}

void CTrkPrintDialog::resizeEvent(QResizeEvent * e)
{
//    qDebug() << "resizeEvent";
    QDialog::resizeEvent(e);
    slotUpdateMetrics();
}

void CTrkPrintDialog::slotUpdateMetrics()
{
//    qDebug() << "slotUpdateMetrics";

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
    }
    else
    {
        qDebug() << "CTrkPrintDialog::slotUpdateMetrics(): Divide by Zero, scaleTrkPx is 0, this should not happen!";
        return;
    }

    QRectF pageRectMeter = printer.pageRect(QPrinter::Millimeter);
    QRectF pageRectPx = printer.pageRect(QPrinter::DevicePixel);

    qreal scalePageMeter = qPow(pageRectMeter.width() / 1000.0, 2) + qPow(pageRectMeter.height() / 1000.0, 2); // Diagonal of paper in meter
    qreal scalePagePx = qPow(pageRectPx.width(), 2) + qPow(pageRectPx.height(), 2);  // Diagonal of paper in pixel
    qreal scalePage;
    if (scalePagePx)
    {
         scalePage = qSqrt(scalePageMeter / scalePagePx); // Meter per pixel on paper
    }
    else
    {
        qDebug() << "CTrkPrintDialog::slotUpdateMetrics(): Divide by Zero, scalePagePx is 0, this should not happen!";
        return;
    }

    scale = scaleTrk / scalePage; // One meter on paper equals to X meter on screen (trk)

    qreal overlapPx = overlap / scalePage; // Overlap converted from meter to pixel

    qreal pageLandscape = pageRectPx.width(); // Page size in pixel
    qreal pagePortrait = pageRectPx.height();
    if (pagePortrait > pageLandscape) // Longer side should always be landscape
    {
        qSwap(pageLandscape, pagePortrait);
    }

    // Build helper list of pixel points from track
    QList<struct pt_t> pts;
    quint32 invalidMask = (trk.getAllValidFlags() & CTrackData::trkpt_t::eValidMask) << 16;
    CTrackData::trkpt_t const *prevTrkpt  = nullptr;
    qint32 nextDistanceMarker = 1;  // The number of next distance marker
    distanceMarkerPts.clear();
    for(const CTrackData::trkpt_t& trkpt : trk.getTrackData())
    {
        if(trkpt.isInvalid(CTrackData::trkpt_t::invalid_e(invalidMask)) || trkpt.isHidden())
        {
            continue;
        }

        struct pt_t pt;

        pt.setX(trkpt.lon * DEG_TO_RAD);
        pt.setY(trkpt.lat * DEG_TO_RAD);
        canvas->convertRad2Px(pt); // Convert x and y of this point to pixel
        pt.distance = trkpt.distance;

        if(prevTrkpt != nullptr) // For pts 1 ... n
        {
            const struct pt_t prevPt = pts.last(); // previous pt in pixel
            pt.bb = QRectF(prevPt, pt).normalized(); // Build normalized boundingBox of this point to previous point

            qreal nextDistance = distanceMarkers[distanceMarker] * nextDistanceMarker * 1000; // The next distance in meter to be reached to print Km marker
            if (distanceMarker && pt.distance > nextDistance) // Distance Marker should be printed and current pt over next distance marker in meter
            {
                qreal factor = (nextDistance - prevPt.distance) / (pt.distance - prevPt.distance); // The factor between prevPt and pt
                QLineF line = QLineF(prevPt, pt); // Distance point in pixel coords
                QPointF kmPt = line.pointAt(factor); // Shorten line to exact kmPt

                line.setLength(line.length() * factor); // Move it to the end of kmPt
                line.translate(line.p2() - line.p1()); // Move it to the end of kmPt
                line.setLength(15); // Set to 15 pixel used as print line to the km text
                line = line.normalVector(); // Rotate by 90 degree to be used for printing the km text

                struct distanceMarkerPt_t dmPt; // Build a new distanceMarkerKm
                dmPt.setX(kmPt.x());
                dmPt.setY(kmPt.y());
                dmPt.km = nextDistance / 1000; // in km
                dmPt.lineToText = line;
                distanceMarkerPts << dmPt; // Add it to the list of distanceMarkers in Km
                ++nextDistanceMarker;
            }
        }

        pts << pt;
        prevTrkpt = &trkpt;
    }

    // Iterate over helper data
    pages.clear();
    QRectF bbPages; // BoundingBox of all pages, to be used for printing
    qreal cutOffset = 0.1; // Offset to move the page a bit away from boundingBox to ensure intersection of both rects
    qreal pageDistanceStart = 0;

    QLineF pageVev(pts.first(), pts.first()); // Steers the track direction to position the page on track boundingBox
    for(qint32 i = 1; i < pts.size(); ++i) // Start at 2nd points to build first line
    {
        struct pt_t &pt = pts[i]; // current pixel point
        struct pt_t &prevPt = pts[i - 1]; // Previous pixel point

        pageVev.setP2(pt); // Set direction from first to the last trkpt on the page

        pt.bb |= prevPt.bb; // Unit the boundingBox of last line with the boundingBox of previous boundingBox for current page
        pt.bb = pt.bb.normalized(); // Always work on normalized rects

        qreal pageX, pageY; // Page dimension used for best fit orientation
        if (qAbs(pt.bb.height()) > qAbs(pt.bb.width())) // Set optimal orientation for page
        {
            pageX = pagePortrait;
            pageY = pageLandscape;
            pt.page.orientation = QPageLayout::Portrait;
        }
        else
        {
            pageX = pageLandscape;
            pageY = pagePortrait;
            pt.page.orientation = QPageLayout::Landscape;
        }

        // Build the page on screen around the track
        pt.page.setRect(pt.bb.left() - cutOffset, pt.bb.top() - cutOffset, pageX - 2 * overlapPx, pageY - 2 * overlapPx); // Build page on screen in normalized form
//        pt.page = QRectF(pt.bb.left() - cutOffset, pt.bb.top() - cutOffset, pageX - 2 * overlapPx, pageY - 2 * overlapPx); // Build page on screen in normalized form
        qreal dx = (pageVev.dx() < 0) ? (-pt.page.width() + pt.bb.width() + 2 * cutOffset) : 0; // Move line based on trk direction in page
        qreal dy = (pageVev.dy() < 0) ? (-pt.page.height() + pt.bb.height() + 2 * cutOffset) : 0;
        pt.page.translate(dx, dy); // Move it

        if(prevPt.page.isNull()) // First line on page: Special treatment when first line on page is longer then page dimensions
        {
            prevPt.page = pt.page; // Take the boundingBox from the next pt
        }

        bool newPage = false;
        // 1st rule: When changing the orientation and current bounding box > previoius boundingBox ==> build new page
        if(pt.page.orientation != prevPt.page.orientation && (pt.bb.width() > pt.page.width() || pt.bb.height() > pt.page.height()))
        {
            newPage = true;
        }
        // 2nd rule: current trkpt is outside to the the corosponding page ==> build new page
        if(!pt.page.contains(pt)) // check whether pixel pt is inside current page rect on screen, if outside try to find intersection of trk line and page on screen
        {
            newPage = true;
        }
        if(newPage) // Build a new page
        {
            QLineF trkLine = QLineF(pt, prevPt); // Last trk line in pixel

            QList<QPointF> pagePts = QPolygonF(prevPt.page).toList(); // Store the 5 points from the page rect in a point list
            bool hasIntersection = false; // There must be an intersection, if not set a debug message
            for (int j = 1; j < pagePts.size(); ++j) // Check for intersection of last trk line against previous page rect
            {
                QLineF pageLine = QLineF(pagePts[j - 1], pagePts[j]); // Build line from page rect
                QPointF ipt; // The intersection point
                QLineF::IntersectType type = pageLine.intersect(trkLine, &ipt); // Try to find an intersection

                if (type == QLineF::BoundedIntersection) // There is an intersection
                {
                    // Fit page on screen to the real cutted trk boundingBox
                    QRectF bb2Ipt = QRectF(prevPt, ipt).normalized(); // BoundingBox of trk line up to intersection point
                    bb2Ipt |= prevPt.bb; // Add to the previous boundingBox
                    bb2Ipt = bb2Ipt.normalized();
                    prevPt.page.moveCenter(bb2Ipt.center()); // Adjust page on screen the to the center of cutted boundingBox

                    pt.bb = QRectF(ipt, pt).normalized(); // Adjust the bb for the current pt up to the intersection pt

                    struct pt_t newPt; // Build a new pt in pixel to be used for next loop
                    newPt.setX(ipt.x());
                    newPt.setY(ipt.y());
                    newPt.bb = QRectF(ipt, QSizeF(0, 0)); // New page start with an empty boundingBox
                    qreal deltaDistance = pt.distance - prevPt.distance;
                    qreal factor;  // For calculating the distance to the newPt
                    if (qAbs(pt.x() - prevPt.x()) > 0 && qAbs(ipt.x() - prevPt.x()) > 0)
                    {
                        factor = qAbs(ipt.x() - prevPt.x()) / qAbs(pt.x() - prevPt.x());
                    }
                    else if (qAbs(pt.y() - prevPt.y()) > 0 && qAbs(ipt.y() - prevPt.y()) > 0)
                    {
                        factor = qAbs(ipt.y() - prevPt.y()) / qAbs(pt.y() - prevPt.y());
                    }
                    else
                    {
                        qDebug() << "CTrkPrintDialog::slotUpdateMetrics(): Two points seems to be on the same position!";
                        return;
                    }
                    newPt.distance = prevPt.distance + deltaDistance * factor;
                    pts.insert(i, newPt);

                    struct page_t page; // Build a new page
                    page = prevPt.page;
                    page += QMarginsF(overlapPx, overlapPx, overlapPx, overlapPx); // Add the overlap margin on each side
                    QPointF centerRad = page.center(); // in pixel
                    canvas->convertPx2Rad(centerRad); // convert to rad to be used for printing
                    page.centerRad = centerRad; // In Rad
                    page.distanceStart = pageDistanceStart;
                    page.distanceEnd = newPt.distance;
                    pages << page;

                    pageDistanceStart = newPt.distance; // Start the new page distance at the newPt

                    bbPages |= page; // Add to the boundingBox of all Pages, used for printing

                    pageVev = QLineF(ipt, ipt); // Set page vector to null for next page
                    hasIntersection = true; // Yeap, an intersection is found
                }
            }
            if (!hasIntersection)
            {
                qDebug() << "Point is outside but no intersection found, this should not happend!";
            }
        }
        else if (i == (pts.size() - 1)) // For last trkpt, build last page or for fullscreen on larger scale then we have only one page
        {
            struct page_t page; // Build new page
            page.setRect(0, 0, pageX, pageY);
            page.moveCenter(pt.bb.center());
            QPointF centerRad = page.center(); // Convert to rad
            canvas->convertPx2Rad(centerRad);
            page.centerRad = centerRad;
            page.orientation = pt.page.orientation;
            page.distanceStart = pageDistanceStart;
            page.distanceEnd = pt.distance;
            pages << page;

            bbPages |= page; // Add to the boundingBox of all Pages
         }
    }

    // Update GUI
    QString labelScaleInfoStr(tr("Zoom with mouse wheel on the map left<br>to change the printing scale.") + "<br><br>");
    labelScaleInfoStr += (tr("Scale of approx. 1:") + QString("%L1")).arg(qRound(scale)) + "<br>";
    labelScaleInfoStr += (tr("1cm on paper page equals %L1m in reality.")).arg(qRound(scale / 100)) + "<br>";
    labelScaleInfoStr += (tr("Be aware, <b>%L1 pages</b> will be needed to print the track."))
            .arg(pages.size()) + "<br><br>";
    if (printer.outputFormat() == QPrinter::PdfFormat)
    {
        labelScaleInfoStr += (tr("Current printer: PDF file")) + "<br>";

        QString fileName = printer.outputFileName();
        if (fileName.size() > 45)
        {
            fileName = "..." + fileName.right(42);
        }
        labelScaleInfoStr += (tr("File: ")) + fileName + "<br>";
    }
    else
    {
        labelScaleInfoStr += (tr("Current printer: ")) + printer.printerName() + "<br>";
    }
    labelScaleInfoStr += tr("Page Size: ") + QPageSize::name((QPageSize::PageSizeId)printer.pageSize()) + "<br>";

    QMarginsF margins = printer.pageLayout().margins(QPageLayout::Millimeter);
    labelScaleInfoStr += tr("Margin Left: %L1mm Top: %L2mm")
            .arg(margins.left(), 0, 'f', 2)
            .arg(margins.top(), 0, 'f', 2) + "<br>";
    labelScaleInfoStr += tr("Margin Right: %L1mm Bottom: %L2mm")
            .arg(margins.right(), 0, 'f', 2)
            .arg(margins.bottom(), 0, 'f', 2)  + "<br>";
    labelScaleInfo->setText(labelScaleInfoStr);

    // Print scene on image
    QSize size = labelPlotTrack->size();
    QImage image(size.width(), size.height(), QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter p;
    p.begin(&image);
    USE_ANTI_ALIASING(p, true);

    qreal w1 = bbPages.width();
    qreal h1 = bbPages.height();
    qreal w2 = size.width() - 10;
    qreal h2 = size.height() - 10;

    qreal scaleX = w1 / w2; //
    qreal scaleY = h1 / h2;
    qreal scaleImg = qMax(scaleX, scaleY); // Use the maximum scale to fit in GUI rect
    qreal offX = (w2 - w1 / scaleImg) / 2; // Offset to fit in the center of the GUI rect
    qreal offY = (h2 - h1 / scaleImg) / 2;

    p.setBrush(Qt::cyan); // Print the pages
    p.setPen(Qt::darkCyan);
    p.setOpacity(0.7); // Some opacity to see the prevoius page
    qint32 pageNo = 0;
    for(const struct page_t& page : pages)
    {
        qreal p1x = 5 + offX + (page.x() - bbPages.x()) / scaleImg; // 5 pixel margin
        qreal p2y = 5 + offY + (page.y() - bbPages.y()) / scaleImg;
        p.drawRect(p1x, p2y, page.width() / scaleImg, page.height() / scaleImg);
    }

    p.setOpacity(1); // Print the track, no opacity
    p.setPen(QPen(Qt::darkBlue, 2));
    QPolygonF polyLine;
    for(const struct pt_t& pt : pts)
    {
        qreal ptx = 5 + offX + (pt.x() - bbPages.x()) / scaleImg;
        qreal pty = 5 + offY + (pt.y() - bbPages.y()) / scaleImg;
        polyLine << QPointF(ptx, pty);
    }
    p.drawPolyline(polyLine);

    p.setPen(QPen());  // Print the pageNo, default pen
    if (pages.size() <= 50) // Max 50 pageNo shown
    {
        for(const struct page_t& page : pages)
        {
            QPointF center = page.center(); // Center in pixel
            qreal p1x = 5 + offX + (center.x() - bbPages.x()) / scaleImg - 10; // 10 text rect 20x20 pixel
            qreal p1y = 5 + offY + (center.y() - bbPages.y()) / scaleImg - 10;
            p.drawText(QRectF(p1x, p1y, 20, 20), Qt::AlignCenter, QString("%1").arg(++pageNo)); // Print the page number in center of page
        }
    }
    p.end();

    labelPlotTrack->setPixmap(QPixmap::fromImage(image)); // Assign the img to the GUI
}

void CTrkPrintDialog::slotPrint()
{
    //    qDebug() << "Print";
    pushLoadPdfFile->setEnabled(false); // No load of PDF file during printing

    slotUpdateMetrics();

    qint32 fromPage = printer.fromPage();
    qint32 toPage = printer.toPage();

    if(printer.fromPage() > 0)
    {
        --fromPage;
    }
    if(fromPage > pages.size())
    {
        fromPage = pages.size() - 1;
    }

    if(printer.toPage() == 0)
    {
        toPage = pages.size();
    }
    if(toPage > pages.size())
    {
        toPage = pages.size();
    }

    QPainter p;
    if (pages.size())
    {
        printer.setPageOrientation(pages[fromPage].orientation); // To be set before p.begin for the first page!
    }
    p.begin(&printer);

    PROGRESS_SETUP(tr("Printing pages."), fromPage, toPage - 1, this);

    bool newPage = false;
    for(qint32 i = fromPage; i < toPage; ++i)
    {
        const struct page_t &currPage = pages[i];
        const struct page_t &prevPage = (i > 0) ? pages[i - 1] : currPage;  // prevPage and nextPage, to be used to set the page marker on current page
        const struct page_t &nextPage = (i < (pages.size() - 1)) ? pages[i + 1] : currPage;

        if(newPage) // Set the right orientation before setting a new page and start printing on this new page
        {
            printer.setPageOrientation(currPage.orientation); // Change of orientation right before printer.newPage
            printer.newPage();
        }
        QRectF pageToPrint = QRectF(0, 0, currPage.width(), currPage.height()); // The paper page
        p.setClipRect(pageToPrint);

        canvas->print(p, pageToPrint, currPage.centerRad, printScaleBar); // Print the canvas content

        newPage = true; // Output a new page on next loop

        // Draw the scale and page number to bottom right
        p.setOpacity(0.7); // Some opacity to see a bit the underlaying map

        QString scaleStr = QString("%L1km, %L2km - %L3km | 1cm=%L4m | %5")
                .arg((currPage.distanceEnd - currPage.distanceStart) / 1000, 0, 'f', 1)
                .arg(currPage.distanceStart / 1000, 0, 'f', 1)
                .arg(currPage.distanceEnd / 1000, 0, 'f', 1)
                .arg(qRound(scale / 100))
                .arg(i + 1);
        QRectF textRect = p.boundingRect(QRectF(), Qt::AlignRight | Qt::AlignBottom, scaleStr);
        p.fillRect(QRectF(currPage.width() - textRect.width(),
                          currPage.height() - textRect.height(),
                          textRect.width(), textRect.height()),
                   Qt::white);
        p.setOpacity(1);
        p.setPen(QPen());  // Print the text with default pen
        p.drawText(QRectF(currPage.width() - textRect.width(),
                          currPage.height() - textRect.height(),
                          textRect.width(), textRect.height()),
                   Qt::AlignRight | Qt::AlignBottom, scaleStr);

        if (i > fromPage && printPageMarkers) // Print page markers from the previous page
        {
            addPageMarkers(p, currPage, prevPage, i);
        }
        if (i < (toPage - 1) && printPageMarkers) // Print page markers from the next page
        {
            addPageMarkers(p, currPage, nextPage, i + 2);
        }

        if(distanceMarker && distanceMarkerPts.size())
        {
            addDistanceMarkers(p, currPage);
        }

        PROGRESS(i, break);
    }
    p.end();

    setPdfFileExists();
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
            QLineF lineNext(currPt, nextPt); // Build next line
            lineNext.setLength(20); // And shorten the line
            p.drawLine(lineNext); // Draw with the painter

            qint32 prev = (i == 0) ? otherPts.size() - 2 : i - 1; // Special for first point, prev point is the last point of rect
            QPointF prevPt = otherPts[prev] - currPage.topLeft();
            QLineF linePrev(currPt, prevPt); // Build previous line
            linePrev.setLength(20);
            p.drawLine(linePrev);

            QRectF textRect(linePrev.p2(), lineNext.p2());
            QString pageNoStr = QString("%1").arg(pageNo);
            p.setOpacity(0.7); // Some opacity to see a bit the underlaying map
            p.fillRect(p.boundingRect(textRect, Qt::AlignCenter, pageNoStr), Qt::white); // Fill text box with a white rect
            p.setOpacity(1);
            p.drawText(textRect, Qt::AlignCenter, pageNoStr); // Print the page number between the marker lines
            p.setBrush(Qt::cyan); // And a small filled circle in the origin
            p.drawEllipse(currPt, 3, 3);
        }
    }
}

void CTrkPrintDialog::addDistanceMarkers(QPainter &p, const QRectF &currPage)
{
    for (struct distanceMarkerPt_t& dmPt : distanceMarkerPts)
    {
        if (currPage.contains(dmPt)) // Find the the best fitting postion from the line to the text box
        {
            qreal d = 22.5; // Degree range ==> 360 / 16
            qreal a = dmPt.lineToText.angle(); // The angel in 0-360 degree
            int flag;
            if (a >= (1 * d) && a < (3 * d))
            {
                flag = Qt::AlignLeft | Qt::AlignBottom;
            }
            else if (a >= (3 * d) && a < (5 * d))
            {
                flag = Qt::AlignHCenter | Qt::AlignBottom;
            }
            else if (a >= (5 * d) && a < (7 * d))
            {
                flag = Qt::AlignRight | Qt::AlignBottom;
            }
            else if (a >= (7 * d) && a < (9 * d))
            {
                flag = Qt::AlignRight | Qt::AlignVCenter;
            }
            else if (a >= (9 * d) && a < (11 * d))
            {
                flag = Qt::AlignRight | Qt::AlignTop;
            }
            else if (a >= (11 * d) && a < (13 * d))
            {
                flag = Qt::AlignHCenter | Qt::AlignTop;
            }
            else if (a >= (13 * d) && a < (15 * d))
            {
                flag = Qt::AlignLeft | Qt::AlignTop;
            }
            else
            {
                flag = Qt::AlignLeft | Qt::AlignVCenter;
            }

            QPointF ptOnPage = dmPt.lineToText.p2() - currPage.topLeft(); // The point on the track
            QString kmStr = QString("%1km").arg(dmPt.km);
            QRectF textRect = p.boundingRect(QRectF(ptOnPage - QPointF(1, 1), ptOnPage + QPointF(1, 1)), flag, kmStr); // The needed text rect
            p.setOpacity(0.7); // Some opacity to see a bit the underlaying map
            p.fillRect(textRect, Qt::white); // Fill text box with a white rect
            p.setOpacity(1);
            p.drawText(textRect, flag, kmStr); // Print the km text

            dmPt.lineToText.setLength(dmPt.lineToText.length() - 3); // Shorten a bit the line;
            p.drawLine(dmPt.lineToText.p1() - currPage.topLeft(), dmPt.lineToText.p2() - currPage.topLeft()); // Draw the line from track to text box
            p.setBrush(Qt::darkRed);
            p.drawEllipse(dmPt - currPage.topLeft(), 3, 3); // Draw a small circle on the track
        }
    }
}

void CTrkPrintDialog::slotSetPrinter()
{
    QPrintDialog dlg(&printer, this);
    QDialogButtonBox *bbox = dlg.findChild<QDialogButtonBox *>();
    bbox->button(QDialogButtonBox::Ok)->setText(tr("OK")); // Instead of default text "Print"

    dlg.setWindowTitle(tr("Printer Properties..."));
    qint32 ret = dlg.exec();

    if(ret == QDialog::Accepted)
    {
        slotUpdateMetrics();
    }
    setPdfFileExists();
}

void CTrkPrintDialog::slotSetOverlap(qint32 ovl)
{
    if (((qreal)(ovl) / 1000) != overlap)
    {
        overlap = (qreal)ovl / 1000;
        slotUpdateMetrics();
    }
}

void CTrkPrintDialog::setPdfFileExists()
{
    QString outputFileName = printer.outputFileName();
    if(QFileInfo(outputFileName).exists())
    {
        pushLoadPdfFile->setEnabled(true);
    }
    else
    {
        pushLoadPdfFile->setEnabled(false);
    }
}

void CTrkPrintDialog::slotLoadPdfFile()
{
    qDebug() << "slotLoadPdfFile";

    QString outputFileName = printer.outputFileName();
    QDesktopServices::openUrl(QUrl("file:///" + outputFileName, QUrl::TolerantMode));
}

void CTrkPrintDialog::slotScaleBar(bool checked)
{
    printScaleBar = checked;
}

void CTrkPrintDialog::slotPageMarkers(bool checked)
{
    printPageMarkers = checked;
}

void CTrkPrintDialog::slotDistanceMarker(int value)
{
    if (distanceMarker != value)
    {
        distanceMarker = value;
        slotUpdateMetrics();
    }
}
