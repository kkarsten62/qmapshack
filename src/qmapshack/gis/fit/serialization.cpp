/**********************************************************************************************
    Copyright (C) 2015 Ivo Kronenberg <>

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

#include "gis/fit/defs/fit_enums.h"
#include "gis/fit/defs/fit_fields.h"
#include "gis/rte/CGisItemRte.h"
#include "gis/trk/CGisItemTrk.h"
#include "gis/wpt/CGisItemWpt.h"

static const qreal degrees = 180.0;
static const qreal twoPow31 = qPow(2, 31);
static const uint sec1970to1990 = 631065600;

/**
 * converts the semicircle to the WGS-84 geoids (Degrees Decimal Minutes (DDD MM.MMM)).
 * North latitude +, South latitude -
 * East longitude +, West longitude -
 *
   return: the given semicircle value converted to degree.
 */
static qreal toDegree(qint32 semicircles)
{
    return semicircles * (degrees / twoPow31);
}

/**
   timestamp: seconds since UTC 00:00 Dec 31 1989
 */
static QDateTime toDateTime(quint32 timestamp)
{
    QDateTime dateTime;
    dateTime.setTime_t(sec1970to1990 + timestamp);
    return dateTime.toUTC();
}

static QString dateTimeAsString(quint32 timestamp)
{
    QDateTime dateTime = toDateTime(timestamp);
    return IUnit::datetime2string(dateTime, true);
}

template<typename T>
static void readKnownExtensions(T& exts, const CFitMessage& mesg)
{
    // see gis/trk/CKnownExtension for the keys of the extensions
    if(mesg.isFieldValueValid(eRecordHeartRate))
    {
        exts["gpxtpx:TrackPointExtension|gpxtpx:hr"] = mesg.getFieldValue(eRecordHeartRate);
    }
    if(mesg.isFieldValueValid(eRecordTemperature))
    {
        exts["gpxtpx:TrackPointExtension|gpxtpx:atemp"] = mesg.getFieldValue(eRecordTemperature);
    }
    if(mesg.isFieldValueValid(eRecordCadence))
    {
        exts["gpxtpx:TrackPointExtension|gpxtpx:cad"] = mesg.getFieldValue(eRecordCadence);
    }
    if(mesg.isFieldValueValid(eRecordPower))
    {
        exts["gpxtpx:TrackPointExtension|gpxtpx:power"] = mesg.getFieldValue(eRecordPower);
    }
    if(mesg.isFieldValueValid(eRecordSpeed))
    {
        const QVariant& speed = mesg.getFieldValue(eRecordSpeed);
        exts["speed"] = speed.toDouble() / 1000.;
    }
}

static bool readFitRecord(const CFitMessage& mesg, IGisItem::wpt_t& pt)
{
    if(mesg.isFieldValueValid(eRecordPositionLong) && mesg.isFieldValueValid(eRecordPositionLat))
    {
        pt.lon = toDegree(mesg.getFieldValue(eRecordPositionLong).toInt());
        pt.lat = toDegree(mesg.getFieldValue(eRecordPositionLat).toInt());
        if(mesg.isFieldValueValid(eRecordEnhancedAltitude))
        {
            pt.ele = mesg.getFieldValue(eRecordEnhancedAltitude).toInt();
        }
        pt.time = toDateTime(mesg.getFieldValue(eRecordTimestamp).toUInt());

        readKnownExtensions(pt.extensions, mesg);

        return true;
    }
    return false;
}

static bool readFitRecord(const CFitMessage& mesg, CTrackData::trkpt_t& pt)
{
    if(readFitRecord(mesg, (IGisItem::wpt_t&)pt))
    {
        pt.speed = mesg.getFieldValue(eRecordSpeed).toDouble();

        readKnownExtensions(pt.extensions, mesg);

        pt.extensions.squeeze();
        return true;
    }
    return false;
}

static void readFitLocation(const CFitMessage& mesg, IGisItem::wpt_t& wpt)
{
    if(mesg.isFieldValueValid(eLocationName))
    {
        wpt.name = mesg.getFieldValue(eLocationName).toString();
    }
    if(mesg.isFieldValueValid(eLocationTimestamp))
    {
        wpt.time = toDateTime(mesg.getFieldValue(eLocationTimestamp).toUInt());
    }
    if(mesg.isFieldValueValid(eLocationPositionLong) && mesg.isFieldValueValid(eLocationPositionLat))
    {
        wpt.lon = toDegree(mesg.getFieldValue(eLocationPositionLong).toInt());
        wpt.lat = toDegree(mesg.getFieldValue(eLocationPositionLat).toInt());
    }
    if(mesg.isFieldValueValid(eLocationAltitude))
    {
        wpt.ele = mesg.getFieldValue(eLocationAltitude).toInt();
    }
    if(mesg.isFieldValueValid(eLocationComment))
    {
        wpt.desc = mesg.getFieldValue(eLocationComment).toString();
    }
    wpt.sym = "Default";
}


QString wptIconNames[26] { "Default", "Summit", "Valley", "Water", "Food", "Danger", "Left", "Right", "Straight",
                           "FirstAid", "4thCategory", "3rdCategory", "2ndCategory", "1stCategory", "HorsCategory",
                           "Sprint"
                           , "LeftFork", "RightFork", "MiddleFork", "SlightLeft", "SharpLeft",
                           "SlightRight", "SharpRight", "UTurn", "Start", "End" };


static void readFitCoursePoint(const CFitMessage& mesg, IGisItem::wpt_t& wpt)
{
    if(mesg.isFieldValueValid(eCoursePointName))
    {
        wpt.name = mesg.getFieldValue(eCoursePointName).toString();
    }
    if(mesg.isFieldValueValid(eCoursePointTimestamp))
    {
        wpt.time = toDateTime(mesg.getFieldValue(eCoursePointTimestamp).toUInt());
    }

    if(mesg.isFieldValueValid(eCoursePointPositionLong) && mesg.isFieldValueValid(eCoursePointPositionLat))
    {
        wpt.lon = toDegree(mesg.getFieldValue(eCoursePointPositionLong).toInt());
        wpt.lat = toDegree(mesg.getFieldValue(eCoursePointPositionLat).toInt());
    }

    if (mesg.isFieldValueValid(eCoursePointType))
    {
        wpt.sym = wptIconNames[mesg.getFieldValue(eCoursePointType).toInt()];
    }
}


static bool readFitSegmentPoint(const CFitMessage& mesg, CTrackData::trkpt_t& pt, quint32 timeCreated)
{
    if(mesg.isFieldValueValid(eSegmentPointPositionLong) && mesg.isFieldValueValid(eSegmentPointPositionLat))
    {
        pt.lon = toDegree(mesg.getFieldValue(eSegmentPointPositionLong).toInt());
        pt.lat = toDegree(mesg.getFieldValue(eSegmentPointPositionLat).toInt());
        if(mesg.isFieldValueValid(eSegmentPointAltitude))
        {
            pt.ele = mesg.getFieldValue(eSegmentPointAltitude).toInt();
        }
        // sum with file_id time_created
        pt.time = toDateTime(timeCreated + mesg.getFieldValue(eSegmentPointLeaderTime).toUInt());
        return true;
    }
    return false;
}


static QString evaluateTrkName(CFitStream& stream)
{
    const CFitMessage& segmentIdMesg = stream.firstMesgOf(eMesgNumSegmentId);
    if(segmentIdMesg.isFieldValueValid(eSegmentIdName))
    {
        return segmentIdMesg.getFieldValue(eSegmentIdName).toString();
    }

    const CFitMessage& courseMesg = stream.firstMesgOf(eMesgNumCourse);
    if(courseMesg.isFieldValueValid(eCourseName))
    {
        // first place: take the course name
        // course files can have a name but activities don't.
        return courseMesg.getFieldValue(eCourseName).toString();
    }

    const CFitMessage& sessionMesg = stream.firstMesgOf(eMesgNumSession);
    if(sessionMesg.isFieldValueValid(eSessionStartTime))
    {
        // second place: take the session start time
        return dateTimeAsString(sessionMesg.getFieldValue(eSessionStartTime).toUInt());
    }

    const CFitMessage& fileidMesg = stream.firstMesgOf(eMesgNumFileId);
    if(fileidMesg.isFieldValueValid(eFileIdTimeCreated))
    {
        // third place: take the file created timestamp
        // (is typically the same as the start time with a offset of several seconds)
        return dateTimeAsString(fileidMesg.getFieldValue(eFileIdTimeCreated).toUInt());
    }

    // fourth place: take the filename of the fit file
    return QFileInfo(stream.getFileName()).completeBaseName().replace("_", " ");
}


void CGisItemTrk::readTrkFromFit(CFitStream& stream)
{
    trk.name = evaluateTrkName(stream);

    quint32 timeCreated = 0;
    const CFitMessage& fileIdMesg = stream.firstMesgOf(eMesgNumFileId);
    if(fileIdMesg.isFieldValueValid(eFileIdTimeCreated))
    {
        timeCreated = fileIdMesg.getFieldValue(eFileIdTimeCreated).toUInt();
    }
    if(fileIdMesg.isFieldValueValid(eFileIdProduct))
    {
        getFitData().setIsValid(true);
        getFitData().setProduct(fileIdMesg.getFieldValue(eFileIdProduct).toUInt());
    }
    stream.reset();

    // note to the FIT specification: the specification allows different ordering of the messages.
    // Record messages can either be at the beginning or in chronological order within the record
    // messages. Garmin devices uses the chronological ordering. We only consider the chronological
    // order, otherwise timestamps (of records and events) must be compared to each other.
    CTrackData::trkseg_t seg;
    do
    {
        const CFitMessage& mesg = stream.nextMesg();

        if(mesg.getGlobalMesgNr() == eMesgNumRecord)
        {
            // for documentation: MesgNumActivity, MesgNumSession, MesgNumLap, MesgNumLength could also contain data
            CTrackData::trkpt_t pt;
            if(readFitRecord(mesg, pt))
            {
                seg.pts.append(std::move(pt));
            }
        }
        else if(mesg.getGlobalMesgNr() == eMesgNumEvent)
        {
            if(mesg.getFieldValue(eEventEvent).toUInt() == eEventTimer)
            {
                uint event = mesg.getFieldValue(eEventEventType).toUInt();
                if(event == eEventTypeStop || event == eEventTypeStopAll || event == eEventTypeStopDisableAll)
                {
                    if(!seg.pts.isEmpty())
                    {
                        trk.segs.append(seg);
                        seg = CTrackData::trkseg_t();
                    }
                }
            }
        }
        else if(mesg.getGlobalMesgNr() == eMesgNumSegmentPoint)
        {
            CTrackData::trkpt_t pt;
            if(readFitSegmentPoint(mesg, pt, timeCreated))
            {
                seg.pts.append(std::move(pt));
            }
        }
        else if(mesg.getGlobalMesgNr() == eMesgNumLap)
        {
            qDebug() << "\n";

            CFitData::lap_t lap;

//            qDebug() << "Start pt.lon=" << toDegree(mesg.getFieldValue(3).toInt());
//            qDebug() << "Start pt.lat=" << toDegree(mesg.getFieldValue(4).toInt());
//            qDebug() << "End   pt.lon=" << toDegree(mesg.getFieldValue(5).toInt());
//            qDebug() << "End   pt.lat=" << toDegree(mesg.getFieldValue(6).toInt());

            lap.type = CFitData::eTypeLap;

            if(mesg.isFieldValueValid(eLapMessageIndex))
            {
                lap.no = mesg.getFieldValue(eLapMessageIndex).toUInt() + 1; // uint16
                qDebug() << "eLapMessageIndex=" << lap.no;
            }
            if(mesg.isFieldValueValid(eLapTotalElapsedTime))
            {
                lap.elapsedTime = mesg.getFieldValue(eLapTotalElapsedTime).toUInt(); // uint32, second
                qDebug() << "eLapTotalElapsedTime=" << lap.elapsedTime;
            }
            if(mesg.isFieldValueValid(eLapTotalTimerTime))
            {
                lap.timerTime = mesg.getFieldValue(eLapTotalTimerTime).toUInt(); // uint32, second
                qDebug() << "eLapTotalTimerTime=" << lap.timerTime;
            }
            if(mesg.isFieldValueValid(eLapTotalDistance))
            {
                lap.distance = mesg.getFieldValue(eLapTotalDistance).toUInt(); // uint32, meter
                qDebug() << "eLapTotalDistance=" << lap.distance;
            }
            if(mesg.isFieldValueValid(eLapAvgSpeed))
            {
                lap.avgSpeed = mesg.getFieldValue(eLapAvgSpeed).toUInt(); // uint16, meter/second
                qDebug() << "eLapAvgSpeed=" << lap.avgSpeed;
            }
            if(mesg.isFieldValueValid(eLapMaxSpeed))
            {
                lap.maxSpeed = mesg.getFieldValue(eLapMaxSpeed).toUInt(); // uint16, meter/second
                qDebug() << "eLapMaxSpeed=" << lap.maxSpeed;
            }
            if(mesg.isFieldValueValid(eLapAvgHeartRate))
            {
                lap.avgHr = mesg.getFieldValue(eLapAvgHeartRate).toUInt(); // uint8, beep/minute
                qDebug() << "eLapAvgHeartRate=" << lap.avgHr;
            }
            if(mesg.isFieldValueValid(eLapMaxHeartRate))
            {
                lap.maxHr = mesg.getFieldValue(eLapMaxHeartRate).toUInt(); // uint8, beep/minute
                qDebug() << "eLapMaxHeartRate=" << lap.maxHr;
            }
            if(mesg.isFieldValueValid(eLapAvgCadence))
            {
                lap.avgCad = mesg.getFieldValue(eLapAvgCadence).toUInt(); // uint8, revolution/minute
                qDebug() << "eLapAvgCadence=" << lap.avgCad;
            }
            if(mesg.isFieldValueValid(eLapMaxCadence))
            {
                lap.maxCad = mesg.getFieldValue(eLapMaxCadence).toUInt(); // uint8, revolution/minute
                qDebug() << "eLapMaxCadence=" << lap.maxCad;
            }
            if(mesg.isFieldValueValid(eLapTotalAscent))
            {
                lap.ascent = mesg.getFieldValue(eLapTotalAscent).toUInt(); // uint16, meter
                qDebug() << "eLapTotalAscent=" << lap.ascent;
            }
            if(mesg.isFieldValueValid(eLapTotalDescent))
            {
                lap.descent = mesg.getFieldValue(eLapTotalDescent).toUInt(); // uint16, meter
                qDebug() << "eLapTotalDescent=" << lap.descent;
            }
            if(mesg.isFieldValueValid(eLapAvgPower))
            {
                lap.avgPower = mesg.getFieldValue(eLapAvgPower).toUInt(); // uint16, watt
                qDebug() << "eLapAvgPower=" << lap.avgPower;
            }
            if(mesg.isFieldValueValid(eLapMaxPower))
            {
                lap.maxPower = mesg.getFieldValue(eLapMaxPower).toUInt(); // uint16, watt
                qDebug() << "eLapMaxPower=" << lap.maxPower;
            }
            if(mesg.isFieldValueValid(eLapNormalizedPower))
            {
                lap.normPower = mesg.getFieldValue(eLapNormalizedPower).toUInt(); // uint16, watt
                qDebug() << "eLapNormalizedPower=" << lap.normPower;
            }
            if(mesg.isFieldValueValid(eLapLeftRightBalance)) // uint16, bitmask
            {
                lap.rightBalance = (mesg.getFieldValue(eLapLeftRightBalance).toUInt() & 0x3FFF) / 100.;
                lap.leftBalance = 100 - lap.rightBalance;
                qDebug() << "leftBalance=" << lap.leftBalance;
                qDebug() << "rightBalance=" << lap.rightBalance;
            }
            if(mesg.isFieldValueValid(eLapAvgLeftPedalSmoothness))
            {
                lap.leftPedalSmooth = mesg.getFieldValue(eLapAvgLeftPedalSmoothness).toUInt(); // uint8, percent
                qDebug() << "eLapAvgLeftPedalSmoothness=" << lap.leftPedalSmooth;
            }
            if(mesg.isFieldValueValid(eLapAvgRightPedalSmoothness))
            {
                lap.rightPedalSmooth = mesg.getFieldValue(eLapAvgRightPedalSmoothness).toUInt(); // uint8, percent
                qDebug() << "eLapAvgRightPedalSmoothness=" << lap.rightPedalSmooth;
            }
            if(mesg.isFieldValueValid(eLapAvgLeftTorqueEffectiveness))
            {
                lap.leftTorqueEff = mesg.getFieldValue(eLapAvgLeftTorqueEffectiveness).toUInt(); // uint8, percent
                qDebug() << "eLapAvgLeftTorqueEffectiveness=" << lap.leftTorqueEff;
            }
            if(mesg.isFieldValueValid(eLapAvgRightTorqueEffectiveness))
            {
                lap.rightTorqueEff = mesg.getFieldValue(eLapAvgRightTorqueEffectiveness).toUInt(); // uint8, percent
                qDebug() << "eLapAvgRightTorqueEffectiveness=" << lap.rightTorqueEff;
            }
            if(mesg.isFieldValueValid(eLapTotalWork))
            {
                lap.work = mesg.getFieldValue(eLapTotalWork).toUInt(); // uint32, joule
                qDebug() << "eLapTotalWork=" << lap.work;
            }
            if(mesg.isFieldValueValid(eLapTotalCalories))
            {
                lap.energy = mesg.getFieldValue(eLapTotalCalories).toUInt(); // uint16, kcalorie
                qDebug() << "eLapTotalCalories=" << lap.energy;
            }
            getFitData().setLap(lap);
        }
        else if(mesg.getGlobalMesgNr() == eMesgNumSession)
        {
            qDebug() << "\n";

            CFitData::lap_t session;

//            qDebug() << "Start pt.lon=" << toDegree(mesg.getFieldValue(3).toInt());
//            qDebug() << "Start pt.lat=" << toDegree(mesg.getFieldValue(4).toInt());

            session.type = CFitData::eTypeSession;

            if(mesg.isFieldValueValid(eSessionNumLaps))
            {
                session.no = mesg.getFieldValue(eSessionNumLaps).toUInt(); // uint16
                qDebug() << "eSessionNumLaps=" << session.no;
            }
            if(mesg.isFieldValueValid(eSessionTotalElapsedTime))
            {
                session.elapsedTime = mesg.getFieldValue(eSessionTotalElapsedTime).toUInt(); // uint32, second
                qDebug() << "eSessionTotalElapsedTime=" << session.elapsedTime;
            }
            if(mesg.isFieldValueValid(eSessionTotalTimerTime))
            {
                session.timerTime = mesg.getFieldValue(eSessionTotalTimerTime).toUInt(); // uint32, second
                qDebug() << "eSessionTotalTimerTime=" << session.timerTime;
            }
            if(mesg.isFieldValueValid(eSessionTotalDistance))
            {
                session.distance = mesg.getFieldValue(eSessionTotalDistance).toUInt(); // uint32, meter
                qDebug() << "eSessionTotalDistance=" << session.distance;
            }
            if(mesg.isFieldValueValid(eSessionAvgSpeed))
            {
                session.avgSpeed = mesg.getFieldValue(eSessionAvgSpeed).toUInt(); // uint16, meter/second
                qDebug() << "eSessionAvgSpeed=" << session.avgSpeed;
            }
            if(mesg.isFieldValueValid(eSessionMaxSpeed))
            {
                session.maxSpeed = mesg.getFieldValue(eSessionMaxSpeed).toUInt(); // uint16, meter/second
                qDebug() << "eSessionMaxSpeed=" << session.maxSpeed;
            }
            if(mesg.isFieldValueValid(eSessionAvgHeartRate))
            {
                session.avgHr = mesg.getFieldValue(eSessionAvgHeartRate).toUInt(); // uint8, beep/minute
                qDebug() << "eSessionAvgHeartRate=" << session.avgHr;
            }
            if(mesg.isFieldValueValid(eSessionMaxHeartRate))
            {
                session.maxHr = mesg.getFieldValue(eSessionMaxHeartRate).toUInt(); // uint8, beep/minute
                qDebug() << "eSessionMaxHeartRate=" << session.maxHr;
            }
            if(mesg.isFieldValueValid(eSessionAvgCadence))
            {
                session.avgCad = mesg.getFieldValue(eSessionAvgCadence).toUInt(); // uint8, revolution/minute
                qDebug() << "eSessionAvgCadence=" << session.avgCad;
            }
            if(mesg.isFieldValueValid(eSessionMaxCadence))
            {
                session.maxCad = mesg.getFieldValue(eSessionMaxCadence).toUInt(); // uint8, revolution/minute
                qDebug() << "eSessionMaxCadence=" << session.maxCad;
            }
            if(mesg.isFieldValueValid(eSessionTotalAscent))
            {
                session.ascent = mesg.getFieldValue(eSessionTotalAscent).toUInt(); // uint16, meter
                qDebug() << "eSessionTotalAscent=" << session.ascent;
            }
            if(mesg.isFieldValueValid(eSessionTotalDescent))
            {
                session.descent = mesg.getFieldValue(eSessionTotalDescent).toUInt(); // uint16, meter
                qDebug() << "eSessionTotalDescent=" << session.descent;
            }
            if(mesg.isFieldValueValid(eSessionAvgPower))
            {
                session.avgPower = mesg.getFieldValue(eSessionAvgPower).toUInt(); // uint16, watt
                qDebug() << "eSessionAvgPower=" << session.avgPower;
            }
            if(mesg.isFieldValueValid(eSessionMaxPower))
            {
                session.maxPower = mesg.getFieldValue(eSessionMaxPower).toUInt(); // uint16, watt
                qDebug() << "eSessionMaxPower=" << session.maxPower;
            }
            if(mesg.isFieldValueValid(eSessionNormalizedPower))
            {
                session.normPower = mesg.getFieldValue(eSessionNormalizedPower).toUInt(); // uint16, watt
                qDebug() << "eSessionNormalizedPower=" << session.normPower;
            }
            if(mesg.isFieldValueValid(eSessionLeftRightBalance)) // uint16, bitmask
            {
                session.rightBalance = (mesg.getFieldValue(eSessionLeftRightBalance).toUInt() & 0x3FFF) / 100.;
                session.leftBalance = 100 - session.rightBalance;
                qDebug() << "leftBalance=" << session.leftBalance;
                qDebug() << "rightBalance=" << session.rightBalance;
            }
            if(mesg.isFieldValueValid(eSessionAvgLeftPedalSmoothness))
            {
                session.leftPedalSmooth = mesg.getFieldValue(eSessionAvgLeftPedalSmoothness).toUInt(); // uint8, percent
                qDebug() << "eSessionAvgLeftPedalSmoothness=" << session.leftPedalSmooth;
            }
            if(mesg.isFieldValueValid(eSessionAvgRightPedalSmoothness))
            {
                session.rightPedalSmooth = mesg.getFieldValue(eSessionAvgRightPedalSmoothness).toUInt(); // uint8, percent
                qDebug() << "eSessionAvgRightPedalSmoothness=" << session.rightPedalSmooth;
            }
            if(mesg.isFieldValueValid(eSessionAvgLeftTorqueEffectiveness))
            {
                session.leftTorqueEff = mesg.getFieldValue(eSessionAvgLeftTorqueEffectiveness).toUInt(); // uint8, percent
                qDebug() << "eSessionAvgLeftTorqueEffectiveness=" << session.leftTorqueEff;
            }
            if(mesg.isFieldValueValid(eSessionAvgRightTorqueEffectiveness))
            {
                session.rightTorqueEff = mesg.getFieldValue(eSessionAvgRightTorqueEffectiveness).toUInt(); // uint8, percent
                qDebug() << "eSessionAvgRightTorqueEffectiveness=" << session.rightTorqueEff;
            }
            if(mesg.isFieldValueValid(eSessionTrainingStressScore))
            {
                session.trainStress = mesg.getFieldValue(eSessionTrainingStressScore).toDouble(); // uint16
                qDebug() << "eSessionTrainingStressScore=" << session.trainStress;
            }
            if(mesg.isFieldValueValid(eSessionIntensityFactor))
            {
                session.intensity = mesg.getFieldValue(eSessionIntensityFactor).toDouble(); // uint16
                qDebug() << "eSessionIntensityFactor=" << session.intensity;
            }
            if(mesg.isFieldValueValid(eSessionTotalWork))
            {
                session.work = mesg.getFieldValue(eSessionTotalWork).toUInt(); // uint32, joule
                qDebug() << "eSessionTotalWork=" << session.work;
            }
            if(mesg.isFieldValueValid(eSessionTotalCalories))
            {
                session.energy = mesg.getFieldValue(eSessionTotalCalories).toUInt(); // uint16, kcalorie
                qDebug() << "eSessionTotalCalories=" << session.energy;
            }
            getFitData().setLap(session);
        }
    }
    while (stream.hasMoreMesg());

    // append last segment if it is not empty.
    // navigation course files do not have to have start / stop event, so add the segment now.
    // and some records do not have a stop event
    if(!seg.pts.isEmpty())
    {
        trk.segs.append(seg);
    }

    if(trk.segs.isEmpty())
    {
        throw tr("FIT file %1 contains no GPS data.").arg(stream.getFileName());
    }
}


void CGisItemWpt::readWptFromFit(CFitStream& stream)
{
    const CFitMessage& mesg = stream.lastMesg();
    if (mesg.getGlobalMesgNr() == eMesgNumLocation)
    {
        readFitLocation(mesg, wpt);
    }
    else
    {
        readFitCoursePoint(mesg, wpt);
    }
}


void CGisItemRte::readRteFromFit(CFitStream& stream)
{
    // a course file could be considered as a route...
    rte.name = evaluateTrkName(stream);
    stream.reset();
    do
    {
        const CFitMessage& mesg = stream.nextMesg();
        if(mesg.getGlobalMesgNr() == eMesgNumRecord)
        {
            rtept_t pt;
            if(readFitRecord(mesg, pt))
            {
                rte.pts.append(std::move(pt));
            }
        }
    }
    while (stream.hasMoreMesg());
}
