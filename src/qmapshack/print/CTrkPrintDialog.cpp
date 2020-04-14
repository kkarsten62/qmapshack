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
#include "print/CTrkPrintDialog.h"
#include "gis/trk/CKnownExtension.h"
#include "helpers/CSettings.h"

CTrkPrintDialog::CTrkPrintDialog(QWidget *parent, const CGisItemTrk &trk) :
    QDialog(parent)
    , trk(trk)
{
    setupUi(this);

//    SETTINGS;
//    maxHr = cfg.value("TrackDetails/HeartRateZones/maxHeartRate", 180).toInt();
//    spinMaxHr->setValue(maxHr);

    QList<CCanvas*> list = CMainWindow::self().getCanvas();

    CCanvas *source;
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



}

CTrkPrintDialog::~CTrkPrintDialog()
{
//    SETTINGS;
//    cfg.setValue("TrackDetails/HeartRateZones/maxHeartRate", spinMaxHr->value());
}

void CTrkPrintDialog::resizeEvent(QResizeEvent * e)
{
    QDialog::resizeEvent(e);
    slotUpdateMetrics();
}


void CTrkPrintDialog::slotUpdateMetrics()
{
qDebug() << "slotUpdateMetrics";

}

