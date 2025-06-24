/**********************************************************************************************
    Copyright (C) 2025 Oliver Eichler <oliver.eichler@gmx.de>

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

#include "gis/fit2/CFit2Project.h"

#include <fit_decode.hpp>
#include <fit_mesg_broadcaster.hpp>
#include <fstream>

#include "CMainWindow.h"
#include "gis/CGisListWks.h"
#include "gis/trk/CGisItemTrk.h"

const QSet<std::string> CFit2Project::knownMessages = {"file_id",  "session",     "lap",         "event",
                                                       "activity", "record",      "device_info", "file_creator",
                                                       "course",   "course_point"};

CFit2Project::CFit2Project(QFile& file, const QString& filename, IDevice* parent)
    : IGisProject(eTypeFit, filename, parent) {
  if (file.isOpen()) {
    file.close();
  }
  loadFitFromFile(file.fileName(), false);

  setupName(QFileInfo(filename).completeBaseName().replace("_", " "));
}

CFit2Project::CFit2Project(const QString& filename, IDevice* parent) : IGisProject(eTypeFit, filename, parent) {
  loadFitFromFile(filename, false);
  setupName(QFileInfo(filename).completeBaseName().replace("_", " "));
}

CFit2Project::CFit2Project(const QString& filename, CGisListWks* parent) : IGisProject(eTypeFit, filename, parent) {
  loadFitFromFile(filename, false);
  setupName(QFileInfo(filename).completeBaseName().replace("_", " "));
}

void CFit2Project::loadFitFromFile(const QString& filename, bool showErrorMsg) {
  qDebug() << "---------" << filename << "---------";
  setIcon(CGisListWks::eColumnIcon, QIcon("://icons/32x32/FitProject.png"));
  blockUpdateItems(true);
  try {
    decodeFile(filename);
    markAsSaved();
    setToolTip(CGisListWks::eColumnName, getInfo());
    valid = true;
  } catch (const std::exception& e) {
    if (showErrorMsg) {
      QMessageBox::critical(CMainWindow::getBestWidgetForParent(), tr("Failed to load file %1...").arg(filename),
                            e.what(), QMessageBox::Abort);
    } else {
      qWarning() << "Failed to load FIT file:" << e.what();
    }
    valid = false;
  }

  sortItems();
  blockUpdateItems(false);
}

void CFit2Project::decodeFile(const QString& filename) {
  // if the file does not exist, the filename is assumed to be a name for a new project
  if (!QFile::exists(filename)) {
    IGisProject::filename.clear();
    setupName(filename);
    setToolTip(CGisListWks::eColumnName, getInfo());
    valid = true;
    return;
  }

  std::fstream file;
  file.open(filename.toStdString(), std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error(("Error opening file " + filename).toStdString());
  }

  fit::Decode decode;

  if (!decode.CheckIntegrity(file)) {
    qWarning() << "FIT file integrity failed. Attempting to decode...";
  }

  // Geeee, call the 90th to collect their old fashioned c++ code!
  // This is depressing.....
  fit::MesgBroadcaster mesgBroadcaster;
  mesgBroadcaster.AddListener((fit::FileIdMesgListener&)*this);
  mesgBroadcaster.AddListener((fit::DeviceInfoMesgListener&)*this);
  mesgBroadcaster.AddListener((fit::RecordMesgListener&)*this);
  mesgBroadcaster.AddListener((fit::MesgListener&)*this);
  mesgBroadcaster.AddListener((fit::ActivityMesgListener&)*this);
  mesgBroadcaster.AddListener((fit::SessionMesgListener&)*this);
  mesgBroadcaster.AddListener((fit::LapMesgListener&)*this);
  mesgBroadcaster.AddListener((fit::EventMesgListener&)*this);
  mesgBroadcaster.AddListener((fit::FileCreatorMesgListener&)*this);
  mesgBroadcaster.AddListener((fit::CourseMesgListener&)*this);
  mesgBroadcaster.AddListener((fit::CoursePointMesgListener&)*this);

  try {
    decode.Read(&file, &mesgBroadcaster, &mesgBroadcaster, nullptr);
  } catch (const fit::RuntimeException& e) {
    throw std::runtime_error((QStringLiteral("Exception decoding file: ") + e.what()).toStdString());
  }

  createTrack("", "");
}

void CFit2Project::createTrack(const QString& name, const QString& comment) {
  if (!segment.isEmpty()) {
    track.segs.append(segment);
    segment.pts.clear();
  }

  if (track.isEmpty()) {
    return;
  }

  if (name.isEmpty()) {
    track.name = IUnit::datetime2string(track.segs.first().pts.first().time, IUnit::eTimeFormatShort);
  } else {
    track.name = name;
  }

  track.cmt = comment;
  //KKA start
  //new CGisItemTrk(track, this); // KKA: Original
  new CGisItemTrk(track, fitData, this);

  track = CTrackData();
  //KKA start
  fitData = CFitData();
  //KKA end
}

void CFit2Project::OnMesg(fit::Mesg& mesg) {
  if (knownMessages.contains(mesg.GetName())) {
    return;
  }
  qDebug() << "Mesg" << mesg.GetNumFields() << mesg.GetName();
  for (int i = 0; i < mesg.GetNumFields(); i++) {
    fit::Field* filed = mesg.GetFieldByIndex(i);
    qDebug() << "  " << filed->GetName();
  }
}

//KKA start
//void CFit2Project::OnMesg(fit::FileIdMesg& mesg) { /*qDebug() << mesg.GetName();*/ } //Original
void CFit2Project::OnMesg(fit::FileIdMesg& mesg) {
  if (recordType == eRecordType::Course) {
    return;
  }
  if (mesg.IsManufacturerValid()) {
    manufacturer = mesg.GetManufacturer();
    fitData.setIsValid(true);
  }
  if (mesg.IsProductValid()) {
    product = mesg.GetProduct();
  }
}
//KKA end

void CFit2Project::OnMesg(fit::DeviceInfoMesg& mesg) {
  // qDebug() << mesg.GetName() << dateTimeFromFitToQt(mesg.GetTimestamp());
}

void CFit2Project::OnMesg(fit::RecordMesg& mesg) {
  // qDebug() << mesg.GetName();
  // for (int i = 0; i < mesg.GetNumFields(); i++) {
  //   fit::Field* filed = mesg.GetFieldByIndex(i);
  //   qDebug() << "  " << filed->GetName();
  // }

  CTrackData::trkpt_t trkpt;
  if (mesg.IsTimestampValid()) {
    trkpt.time = dateTimeFromFitToQt(mesg.GetTimestamp());
    //KKA start
    trkptTime = trkpt.time; //To be saved to get the last time for a lap
    //KKA end
  }
  if (mesg.IsPositionLatValid() && mesg.IsPositionLongValid()) {
    trkpt.lon = semicircleToDegree(mesg.GetPositionLong());
    trkpt.lat = semicircleToDegree(mesg.GetPositionLat());

    trkpt.valid |= (trkpt.lat < -90) || (trkpt.lat > 90) || (trkpt.lon < -180) || (trkpt.lon > 180)
                       ? quint32(CTrackData::trkpt_t::eInvalidPos)
                       : quint32(CTrackData::trkpt_t::eValidPos);
  }
  if (mesg.IsAltitudeValid()) {
    trkpt.ele = mesg.GetAltitude();
  }
  if (mesg.IsEnhancedAltitudeValid()) {
    trkpt.ele = mesg.GetEnhancedAltitude();
  }
  if (mesg.IsSpeedValid()) {
    trkpt.extensions["speed"] = mesg.GetSpeed();
  }
  if (mesg.IsEnhancedSpeedValid()) {
    trkpt.extensions["fit:speed"] = mesg.GetEnhancedSpeed();
  }
  if (mesg.IsDistanceValid()) {
    trkpt.extensions["fit:distance"] = mesg.GetDistance();
  }
  if (mesg.IsEnhancedRespirationRateValid()) {
    trkpt.extensions["fit:respiration_rate"] = mesg.GetEnhancedRespirationRate();
  }

  if (mesg.IsTemperatureValid()) {
    trkpt.extensions["gpxtpx:TrackPointExtension|gpxtpx:atemp"] = mesg.GetTemperature();
  }
  if (mesg.IsHeartRateValid()) {
    trkpt.extensions["gpxtpx:TrackPointExtension|gpxtpx:hr"] = mesg.GetHeartRate();
  }
  if (mesg.IsCadenceValid()) {
    trkpt.extensions["gpxtpx:TrackPointExtension|gpxtpx:cad"] = mesg.GetCadence();
  }
  if (mesg.IsPowerValid()) {
    trkpt.extensions["gpxtpx:TrackPointExtension|gpxtpx:power"] = mesg.GetPower();
  }

  if (trkpt.isValid(CTrackData::trkpt_t::eValidPos)) {
    segment.pts.append(trkpt);
  } else {
    qWarning() << "invalid track point in FIT record" << trkpt.time << trkpt << trkpt.ele << trkpt.extensions
               << "- skip";
  }
}

void CFit2Project::OnMesg(fit::ActivityMesg& mesg) {
  // qDebug() << mesg.GetName() << dateTimeFromFitToQt(mesg.GetTimestamp()) << mesg.GetEventType();

  // for (int i = 0; i < mesg.GetNumFields(); i++) {
  //   fit::Field* filed = mesg.GetFieldByIndex(i);
  //   qDebug() << "  " << filed->GetName();
  // }
}

void CFit2Project::OnMesg(fit::SessionMesg& mesg) {
  // qDebug() << mesg.GetName() << dateTimeFromFitToQt(mesg.GetTimestamp()) << mesg.GetEventType();
  // for (int i = 0; i < mesg.GetNumFields(); i++) {
  //   fit::Field* filed = mesg.GetFieldByIndex(i);
  //   qDebug() << "  " << filed->GetName();
  // }

  //KKA start
  if (recordType == eRecordType::Course) {
    return;
  }
  CFitData::lap_t session;

  session.manufacturer = manufacturer; //quint16
  session.product = product; //quint16
  if (mesg.IsNumLapsValid()) {
    session.no = mesg.GetNumLaps(); //uint16
  }
  session.type = CFitData::eTypeSession;
  if (mesg.IsStartTimeValid()) {
    session.startTime = dateTimeFromFitToQt(mesg.GetStartTime()); //uint32
  }
  if (mesg.IsTotalElapsedTimeValid()) {
    session.elapsedTime = mesg.GetTotalElapsedTime(); //uint32, second => float
  }
  if (mesg.IsTotalTimerTimeValid()) {
    session.timerTime = mesg.GetTotalTimerTime(); //uint32, second => float
  }
  if (mesg.IsTotalDistanceValid()) {
    session.distance = mesg.GetTotalDistance(); //uint32, meter => float
  }
  if (mesg.IsAvgSpeedValid()) {
    session.avgSpeed = mesg.GetAvgSpeed(); //uint32, meter/second => float
  }
  if (mesg.IsEnhancedAvgSpeedValid()) {
    session.avgSpeed = mesg.GetEnhancedAvgSpeed(); //uint32, meter/second => float
  }
  if (mesg.IsMaxSpeedValid()) {
    session.maxSpeed = mesg.GetMaxSpeed(); //uint32, meter/second => float
  }
  if (mesg.IsEnhancedMaxSpeedValid()) {
    session.maxSpeed = mesg.GetEnhancedMaxSpeed();//uint32, second => float
  }
  if (mesg.IsTotalAscentValid()) {
    session.ascent = mesg.GetTotalAscent(); //uint16, meter
  }
  if (mesg.IsTotalDescentValid()) {
    session.descent = mesg.GetTotalDescent(); //uint16, meter
  }
  if (mesg.IsAvgHeartRateValid()) {
    session.avgHr = mesg.GetAvgHeartRate(); //uint8, beep/minute
  }
  if (mesg.IsMaxHeartRateValid()) {
    session.maxHr = mesg.GetMaxHeartRate(); //uint8, beep/minute
  }
  if (mesg.IsAvgCadenceValid()) {
    session.avgCad = mesg.GetAvgCadence(); //uint8, revolution/minute
  }
  if (mesg.IsMaxCadenceValid()) {
    session.maxCad = mesg.GetMaxCadence(); //uint8, revolution/minute
  }
  if (mesg.IsAvgPowerValid()) {
    session.avgPower = mesg.GetAvgPower(); //uint16, watt
  }
  if (mesg.IsMaxPowerValid()) {
    session.maxPower = mesg.GetMaxPower(); //uint16, watt
  }
  if (mesg.IsNormalizedPowerValid()) {
    session.normPower = mesg.GetNormalizedPower(); //uint16, watt
  }
  if (mesg.IsLeftRightBalanceValid()) {
    session.leftRightBalance = mesg.GetLeftRightBalance(); // uint16, bitfield FIT_LEFT_RIGHT_BALANCE_100
  }
  if (mesg.IsAvgLeftPedalSmoothnessValid()) {
    session.leftPedalSmooth = mesg.GetAvgLeftPedalSmoothness(); //uint8, percent => float
  }
  if (mesg.IsAvgRightPedalSmoothnessValid()) {
    session.rightPedalSmooth = mesg.GetAvgRightPedalSmoothness(); //uint8, percent => float
  }
  if (mesg.IsAvgLeftTorqueEffectivenessValid()) {
    session.leftTorqueEff = mesg.GetAvgLeftTorqueEffectiveness(); //uint8, percent => float
  }
  if (mesg.IsAvgRightTorqueEffectivenessValid()) {
    session.rightTorqueEff = mesg.GetAvgRightTorqueEffectiveness(); //uint8, percent => float
  }
  if (mesg.IsAvgLeftPcoValid()) {
    session.leftPco = mesg.GetAvgLeftPco(); //uint8, mm, plus to outer the bike, minus to inner the bike
  }
  if (mesg.IsAvgRightPcoValid()) {
    session.rightPco = mesg.GetAvgRightPco(); //uint8, mm, plus to outer the bike, minus to inner the bike
  }
  for (qint32 i = 0; i < 4; ++i) {
    if (mesg.IsAvgLeftPowerPhaseValid(i)) {
        session.powerPhases.append(mesg.GetAvgLeftPowerPhase(i)); //float
    }
  }
  for (qint32 i = 0; i < 4; ++i) {
    if (mesg.IsAvgLeftPowerPhasePeakValid(i)) {
        session.powerPhases.append(mesg.GetAvgLeftPowerPhasePeak(i)); //float
    }
  }
  for (qint32 i = 0; i < 4; ++i) {
    if (mesg.IsAvgRightPowerPhaseValid(i)) {
        session.powerPhases.append(mesg.GetAvgRightPowerPhase(i)); //float
    }
  }
  for (qint32 i = 0; i < 4; ++i) {
    if (mesg.IsAvgRightPowerPhasePeakValid(i)) {
        session.powerPhases.append(mesg.GetAvgRightPowerPhasePeak(i)); //float
    }
  }
  if(mesg.IsThresholdPowerValid())
  {
    session.functionalThresholdPower = mesg.GetThresholdPower(); //uint16 session only
  }
  if(mesg.IsIntensityFactorValid())
  {
    session.intensityFactor = mesg.GetIntensityFactor(); //float session only
  }
  if(mesg.IsTrainingStressScoreValid())
  {
    session.trainingStressScore = mesg.GetTrainingStressScore(); //float session only
  }
  if (mesg.IsTotalWorkValid()) {
    session.work = mesg.GetTotalWork(); // uint32, joule
  }
  if (mesg.IsTotalCaloriesValid()) {
    session.energy = mesg.GetTotalCalories(); // uint16, kcal
  }
  fitData.setLap(fitData.getNoOfLaps(), session); //Set the session always at the end of laps list
  //********************
  //KKA end

  QString comment = "<div><b>Device Statistic</b><br/>";
  QString val, unit;
  if (mesg.IsTotalElapsedTimeValid()) {
    IUnit::self().seconds2time(mesg.GetTotalElapsedTime(), val, unit);
    comment += tr("total elapsed time: %1%2<br/>").arg(val, unit);
  }

  if (mesg.IsTotalDistanceValid()) {
    IUnit::self().meter2distance(mesg.GetTotalDistance(), val, unit);
    comment += tr("total distance: %1%2<br/>").arg(val, unit);
  }

  if (mesg.IsEnhancedMaxSpeedValid()) {
    IUnit::self().meter2speed(mesg.GetEnhancedMaxSpeed(), val, unit);
    comment += tr("enhanced max speed: %1%2<br/>").arg(val, unit);
  }

  if (mesg.IsEnhancedMinAltitudeValid()) {
    IUnit::self().meter2elevation(mesg.GetEnhancedMinAltitude(), val, unit);
    comment += tr("enhanced min altitude: %1%2<br/>").arg(val, unit);
  }

  if (mesg.IsEnhancedMaxAltitudeValid()) {
    IUnit::self().meter2elevation(mesg.GetEnhancedMaxAltitude(), val, unit);
    comment += tr("enhanced max altitude: %1%2<br/>").arg(val, unit);
  }

  if (mesg.IsTotalCaloriesValid()) {
    comment += tr("total calories: %1kcal<br/>").arg(mesg.GetTotalCalories());
  }

  if (mesg.GetTotalAscent()) {
    IUnit::self().meter2elevation(mesg.GetTotalAscent(), val, unit);
    comment += tr("total ascent: %1%2<br/>").arg(val, unit);
  }

  if (mesg.GetTotalDescent()) {
    IUnit::self().meter2elevation(mesg.GetTotalDescent(), val, unit);
    comment += tr("total descent: %1%2<br/>").arg(val, unit);
  }

  if (mesg.IsNumLapsValid()) {
    comment += tr("number of laps: %1<br/>").arg(mesg.GetNumLaps());
  }

  comment += "</div>";

  QString name;
  if (mesg.IsStartTimeValid()) {
    name = IUnit::datetime2string(dateTimeFromFitToQt(mesg.GetStartTime()), IUnit::eTimeFormatShort);
  }

  createTrack(name, comment);
}

void CFit2Project::OnMesg(fit::LapMesg& mesg) {
  // qDebug() << mesg.GetName() << dateTimeFromFitToQt(mesg.GetTimestamp());
  // for (int i = 0; i < mesg.GetNumFields(); i++) {
  //   fit::Field* filed = mesg.GetFieldByIndex(i);
  //   qDebug() << "  " << filed->GetName();
  // }
  if (!segment.isEmpty()) {
    track.segs.append(segment);
    segment.pts.clear();
  }
  CFitData::lap_t lap;

  //KKA start
  if (recordType == eRecordType::Course) {
    return;
  }
  lap.manufacturer = manufacturer; //quint16
  lap.product = product; //quint16
  //lap.no = fitData.getNoOfLaps();
  lap.no = lapNo++;
  lap.type = CFitData::eTypeLap;
  if (mesg.IsStartTimeValid()) {
    lap.startTime = dateTimeFromFitToQt(mesg.GetStartTime()); //uint32
  }
  if (mesg.IsTotalElapsedTimeValid()) {
    lap.elapsedTime = mesg.GetTotalElapsedTime(); //uint32, second => float
  }
  if (mesg.IsTotalTimerTimeValid()) {
    lap.timerTime = mesg.GetTotalTimerTime(); //uint32, second => float
  }
  if (mesg.IsTotalDistanceValid()) {
    lap.distance = mesg.GetTotalDistance(); //uint32, meter => float
  }
  if (mesg.IsAvgSpeedValid()) {
    lap.avgSpeed = mesg.GetAvgSpeed(); //uint32, meter/second => float
  }
  if (mesg.IsEnhancedAvgSpeedValid()) {
    lap.avgSpeed = mesg.GetEnhancedAvgSpeed(); //uint32, meter/second => float
  }
  if (mesg.IsMaxSpeedValid()) {
    lap.maxSpeed = mesg.GetMaxSpeed(); //uint32, meter/second => float
  }
  if (mesg.IsEnhancedMaxSpeedValid()) {
    lap.maxSpeed = mesg.GetEnhancedMaxSpeed();//uint32, second => float
  }
  if (mesg.IsTotalAscentValid()) {
    lap.ascent = mesg.GetTotalAscent(); //uint16, meter
  }
  if (mesg.IsTotalDescentValid()) {
    lap.descent = mesg.GetTotalDescent(); //uint16, meter
  }
  if (mesg.IsAvgHeartRateValid()) {
    lap.avgHr = mesg.GetAvgHeartRate(); //uint8, beep/minute
  }
  if (mesg.IsMaxHeartRateValid()) {
    lap.maxHr = mesg.GetMaxHeartRate(); //uint8, beep/minute
  }
  if (mesg.IsAvgCadenceValid()) {
    lap.avgCad = mesg.GetAvgCadence(); //uint8, revolution/minute
  }
  if (mesg.IsMaxCadenceValid()) {
    lap.maxCad = mesg.GetMaxCadence(); //uint8, revolution/minute
  }
  if (mesg.IsAvgPowerValid()) {
    lap.avgPower = mesg.GetAvgPower(); //uint16, watt
  }
  if (mesg.IsMaxPowerValid()) {
    lap.maxPower = mesg.GetMaxPower(); //uint16, watt
  }
  if (mesg.IsNormalizedPowerValid()) {
    lap.normPower = mesg.GetNormalizedPower(); //uint16, watt
  }
  if (mesg.IsLeftRightBalanceValid()) {
    lap.leftRightBalance = mesg.GetLeftRightBalance(); // uint16, bitfield FIT_LEFT_RIGHT_BALANCE_100
  }
  if (mesg.IsAvgLeftPedalSmoothnessValid()) {
    lap.leftPedalSmooth = mesg.GetAvgLeftPedalSmoothness(); //uint8, percent => float
  }
  if (mesg.IsAvgRightPedalSmoothnessValid()) {
    lap.rightPedalSmooth = mesg.GetAvgRightPedalSmoothness(); //uint8, percent => float
  }
  if (mesg.IsAvgLeftTorqueEffectivenessValid()) {
    lap.leftTorqueEff = mesg.GetAvgLeftTorqueEffectiveness(); //uint8, percent => float
  }
  if (mesg.IsAvgRightTorqueEffectivenessValid()) {
    lap.rightTorqueEff = mesg.GetAvgRightTorqueEffectiveness(); //uint8, percent => float
  }
  if (mesg.IsAvgLeftPcoValid()) {
    lap.leftPco = mesg.GetAvgLeftPco(); //uint8, mm
  }
  if (mesg.IsAvgRightPcoValid()) {
    lap.rightPco = mesg.GetAvgRightPco(); //uint8, mm
  }
  for (qint32 i = 0; i < 4; ++i) {
    if (mesg.IsAvgLeftPowerPhaseValid(i)) {
        lap.powerPhases.append(mesg.GetAvgLeftPowerPhase(i)); //float
    }
  }
  for (qint32 i = 0; i < 4; ++i) {
    if (mesg.IsAvgLeftPowerPhasePeakValid(i)) {
        lap.powerPhases.append(mesg.GetAvgLeftPowerPhasePeak(i)); //float
    }
  }
  for (qint32 i = 0; i < 4; ++i) {
    if (mesg.IsAvgRightPowerPhaseValid(i)) {
        lap.powerPhases.append(mesg.GetAvgRightPowerPhase(i)); //float
    }
  }
  for (qint32 i = 0; i < 4; ++i) {
    if (mesg.IsAvgRightPowerPhasePeakValid(i)) {
        lap.powerPhases.append(mesg.GetAvgRightPowerPhasePeak(i)); //float
    }
  }
  if (mesg.IsTotalWorkValid()) {
    lap.work = mesg.GetTotalWork(); // uint32, joule
  }
  if (mesg.IsTotalCaloriesValid()) {
    lap.energy = mesg.GetTotalCalories(); // uint16, kcal
  }
  fitData.setLap(lap.no, lap);

  //Original, all commented
  /*
  if (recordType == eRecordType::Course) {
    QString val, unit;
    QString comment = "<div>";
    if (mesg.IsTotalTimerTimeValid()) {
      IUnit::self().seconds2time(mesg.GetTotalTimerTime(), val, unit);
      comment += tr("total timer time: %1%2<br/>").arg(val, unit);
    }
    if (mesg.IsTotalDistanceValid()) {
      IUnit::self().meter2distance(mesg.GetTotalDistance(), val, unit);
      comment += tr("total distance: %1%2<br/>").arg(val, unit);
    }
    if (mesg.IsEnhancedAvgSpeedValid()) {
      IUnit::self().meter2speed(mesg.GetEnhancedAvgSpeed(), val, unit);
      comment += tr("enhanced average speed: %1%2<br/>").arg(val, unit);
    }
    if (mesg.GetTotalAscent()) {
      IUnit::self().meter2elevation(mesg.GetTotalAscent(), val, unit);
      comment += tr("total ascent: %1%2<br/>").arg(val, unit);
    }
    if (mesg.GetTotalDescent()) {
      IUnit::self().meter2elevation(mesg.GetTotalDescent(), val, unit);
      comment += tr("total descent: %1%2<br/>").arg(val, unit);
    }

    comment += "<div>";
    track.cmt = comment;
  }
  */
  //KKA end
}

void CFit2Project::OnMesg(fit::EventMesg& mesg) {
  // qDebug() << mesg.GetName() << dateTimeFromFitToQt(mesg.GetTimestamp()) << mesg.GetEventType();
  if (mesg.IsEventTypeValid()) {
    switch (mesg.GetEventType()) {
      case FIT_EVENT_TYPE_START:
      case FIT_EVENT_TYPE_STOP:
      case FIT_EVENT_TYPE_STOP_ALL:
        if (recordType == eRecordType::Course) {
          createTrack("", "");
        } else if (!segment.isEmpty()) {
          track.segs.append(segment);
          segment.pts.clear();
        }
        break;
    }
  }
}

void CFit2Project::OnMesg(fit::FileCreatorMesg& mesg) { /*qDebug() << mesg.GetName();*/ }

void CFit2Project::OnMesg(fit::CourseMesg& mesg) {
  qDebug() << mesg.GetName() << mesg.GetSport() << Qt::hex << mesg.GetCapabilities();
  recordType = eRecordType::Course;
  track.name = QString::fromStdWString(mesg.GetName());
  // sport to qms activity?
  //KKA start
  fitData.setIsValid(false);
  //KKA end
}

constexpr int kNumKnownSymbols = 26;
const QString wptIconNames[kNumKnownSymbols]{
    "Flag, Blue",   "Summit",     "Valley",   "Water",       "Food",        "Danger",      "Left",
    "Right",        "Straight",   "FirstAid", "4thCategory", "3rdCategory", "2ndCategory", "1stCategory",
    "HorsCategory", "Sprint",     "LeftFork", "RightFork",   "MiddleFork",  "SlightLeft",  "SharpLeft",
    "SlightRight",  "SharpRight", "UTurn",    "Start",       "End"};

void CFit2Project::OnMesg(fit::CoursePointMesg& mesg) {
  // qDebug() << mesg.GetName() << dateTimeFromFitToQt(mesg.GetTimestamp()) << mesg.GetType();
  CGisItemWpt::wpt_t wpt;
  if (mesg.IsNameValid()) {
    wpt.name = QString::fromStdWString(mesg.GetName());
  }
  if (mesg.IsTimestampValid()) {
    wpt.time = dateTimeFromFitToQt(mesg.GetTimestamp());
  }
  if (mesg.IsTypeValid() && mesg.GetType() < kNumKnownSymbols) {
    wpt.sym = wptIconNames[mesg.GetType()];
  } else {
    wpt.sym = "City (Small)";
  }
  if (mesg.IsPositionLatValid() && mesg.IsPositionLongValid()) {
    wpt.lon = semicircleToDegree(mesg.GetPositionLong());
    wpt.lat = semicircleToDegree(mesg.GetPositionLat());
  }
  if ((wpt.lat >= -90) || (wpt.lat <= 90) || (wpt.lon >= -180) || (wpt.lon <= 180)) {
    new CGisItemWpt(wpt, this);
  }
}
