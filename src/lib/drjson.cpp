// drjson.cpp
//
// JSON Formatting Functions
//
//   (C) Copyright 2018-2024 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include "drjson.h"

QString DRJsonPadding(int padding)
{
  QString ret="";

  for(int i=0;i<padding;i++) {
    ret+=" ";
  }
  return ret;
}


QString DRJsonEscape(const QString &str)
{
  QString ret;

  for(int i=0;i<str.length();i++) {
    QChar c=str.at(i);
    switch(c.category()) {
    case QChar::Other_Control:
      ret+=QString::asprintf("\\u%04X",c.unicode());
      break;

    default:
      switch(c.unicode()) {
      case 0x22:   // Quote
	ret+="\\\"";
	break;

      case 0x5C:   // Backslash
	ret+="\\\\";
	break;

      default:
	ret+=c;
	break;
      }
      break;
    }
  }

  return ret;
}


QString DRJsonNullField(const QString &name,int padding,bool final)
{
  QString comma=",";

  if(final) {
    comma="";
  }

  return DRJsonPadding(padding)+"\""+name+"\": null"+comma+"\r\n";
}


QString DRJsonField(const QString &name,bool value,int padding,bool final)
{
  QString comma=",";

  if(final) {
    comma="";
  }

  if(value) {
    return DRJsonPadding(padding)+"\""+name+"\": true"+comma+"\r\n";
  }
  return DRJsonPadding(padding)+"\""+name+"\": false"+comma+"\r\n";
}


QString DRJsonField(const QString &name,int value,int padding,bool final)
{
  QString comma=",";

  if(final) {
    comma="";
  }

  return DRJsonPadding(padding)+"\""+name+"\": "+QString::asprintf("%d",value)+
    comma+"\r\n";
}


QString DRJsonField(const QString &name,unsigned value,int padding,bool final)
{
  QString comma=",";

  if(final) {
    comma="";
  }

  return DRJsonPadding(padding)+"\""+name+"\": "+QString::asprintf("%u",value)+
    comma+"\r\n";
}


QString DRJsonField(const QString &name,const QString &value,int padding,
		    bool final)
{
  QString ret;
  QString comma=",";

  if(final) {
    comma="";
  }

  ret=DRJsonEscape(value);

  return DRJsonPadding(padding)+"\""+name+"\": \""+ret+"\""+comma+"\r\n";
}


QString DRJsonField(const QString &name,const QDateTime &value,int padding,
		    bool final)
{
  QString comma=",";

  if(final) {
    comma="";
  }

  if(!value.isValid()) {
    return DRJsonNullField(name,padding,final);
  }
  return DRJsonPadding(padding)+"\""+name+"\": \""+
    DRWriteXmlDateTime(value)+"\""+
    comma+"\r\n";
}


QString DRJsonField(const QString &name,const QTime &value,
			     int utc_offset,int padding,bool final)
{
  QString comma=",";

  if(final) {
    comma="";
  }

  if(!value.isValid()) {
    return DRJsonNullField(name,padding,final);
  }
  return DRJsonPadding(padding)+"\""+name+"\": \""+
    DRWriteXmlTime(value,utc_offset)+"\""+
    comma+"\r\n";
}


QString DRJsonField(const QString &name,const QVariant &value,int padding,
		    bool final)
{
  if(value.canConvert<QDateTime>()) {
    return DRJsonField(name,value.toDateTime(),padding,final);
  }
  if(value.canConvert<QTime>()) {
    // WARNING: We assume UTC here!
    return DRJsonField(name,value.toTime(),0,padding,final);
  }
  if(value.canConvert<unsigned>()) {
    return DRJsonField(name,value.toInt(),padding,final);
  }
  if(value.canConvert<int>()) {
    return DRJsonField(name,value.toInt(),padding,final);
  }
  if(value.canConvert<bool>()) {
    return DRJsonField(name,value.toInt(),padding,final);
  }
  if(value.canConvert<QString>()) {
    return DRJsonField(name,value.toString(),padding,final);
  }
  return QString();
}


QString DRJsonCloseBlock(bool final)
{
  if(final) {
    return QString("}\r\n");
  }
  return QString("},\r\n");
}



//
// XML xs:date format
//
QDate DRParseXmlDate(const QString &str,bool *ok)
{
  QDate ret=QDate::fromString(str,"yyyy-MM-dd");
  if(ok!=NULL) {
    *ok=ret.isValid();
  }
  return ret;
}


QString DRWriteXmlDate(const QDate &date)
{
  return date.toString("yyyy-MM-dd");
}


//
// XML xs:time format
//
QTime DRParseXmlTime(const QString &str,bool *ok,int *day_offset)
{
  QTime ret;
  QStringList f0;
  QStringList f1;
  QStringList f2;
  int tz=0;
  QTime time;
  QTime tztime;

  if(ok!=NULL) {
    *ok=false;
  }
  if(day_offset!=NULL) {
    *day_offset=0;
  }
  f0=str.trimmed().split(" ");
  if(f0.size()!=1) {
    if(ok!=NULL) {
      *ok=false;
    }
    return ret;
  }

  if(f0[0].right(1).toLower()=="z") {  // GMT
    tz=-DRTimeZoneOffset();
    f0[0]=f0[0].left(f0[0].length()-1);
    f2=f0[0].split(":");
  }
  else {
    f1=f0[0].split("+");
    if(f1.size()==2) {   // GMT+
      f2=f1[1].split(":");
      if(f2.size()==2) {
	tztime=QTime(f2[0].toInt(),f2[1].toInt(),0);
	if(tztime.isValid()) {
	  tz=-DRTimeZoneOffset()-QTime(0,0,0).secsTo(tztime);
	}
      }
      else {
	if(ok!=NULL) {
	  *ok=false;
	}
	return QTime();
      }
    }
    else {
      f1=f0[0].split("-");
      if(f1.size()==2) {   // GMT-
	f2=f1[1].split(":");
	if(f2.size()==2) {
	  tztime=QTime(f2[0].toInt(),f2[1].toInt(),0);
	  if(tztime.isValid()) {
	    tz=-DRTimeZoneOffset()+QTime(0,0,0).secsTo(tztime);
	  }
	}
	else {
	  if(ok!=NULL) {
	    *ok=false;
	  }
	  return QTime();
	}
      }
    }
    f2=f1[0].split(":");
  }
  if(f2.size()==3) {
    QStringList f3=f2[2].split(".");
    time=QTime(f2[0].toInt(),f2[1].toInt(),f2[2].toInt());
    if(time.isValid()) {
      ret=time.addSecs(tz);
      if(day_offset!=NULL) {
	if((tz<0)&&((3600*time.hour()+60*time.minute()+time.second())<(-tz))) {
	  *day_offset=-1;
	}
	if((tz>0)&&(86400-((3600*time.hour()+60*time.minute()+time.second()))<tz)) {
	  *day_offset=1;
	}
      }
      if(ok!=NULL) {
	*ok=true;
      }
    }
  }
  return ret;
}


QString DRWriteXmlTime(const QTime &time,int utc_offset)
{
  if(utc_offset==0) {  // Prefer the 'Z' format for UTC
    return time.toString("hh:mm:ss")+"Z";
  }

  QString tz_str="-";
  if(utc_offset<0) {
    tz_str="+";
  }
  tz_str+=QString::asprintf("%02d:%02d",utc_offset/3600,
			    (utc_offset-3600*(utc_offset/3600))/60);

  return time.toString("hh:mm:ss")+tz_str;
}


//
// XML xs:dateTime format
//
QDateTime DRParseXmlDateTime(const QString &str,bool *ok)
{
  QDateTime ret;
  QStringList list;
  QStringList f0;
  QStringList f1;
  QStringList f2;
  int day;
  int month;
  int year;
  QTime time;
  bool lok=false;
  int day_offset=0;

  if(ok!=NULL) {
    *ok=false;
  }

  f0=str.trimmed().split(" ");
  if(f0.size()!=1) {
    if(ok!=NULL) {
      *ok=false;
    }
    return ret;
  }
  f1=f0[0].split("T");
  if(f1.size()<=2) {
    f2=f1[0].split("-");
    if(f2.size()==3) {
      year=f2[0].toInt(&lok);
      if(lok&&(year>0)) {
	month=f2[1].toInt(&lok);
	if(lok&&(month>=1)&&(month<=12)) {
	  day=f2[2].toInt(&lok);
	  if(lok&&(day>=1)&&(day<=31)) {
	    if(f1.size()==2) {
	      time=DRParseXmlTime(f1[1],&lok,&day_offset);
	      if(lok) {
		ret=QDateTime(QDate(year,month,day),time).addDays(day_offset);
		if(ok!=NULL) {
		  *ok=true;
		}
	      }
	    }
	  }
	}
      }
    }
  }
  return ret;
}


QString DRWriteXmlDateTime(const QDateTime &dt)
{
  return DRWriteXmlDate(dt.date())+"T"+DRWriteXmlTime(dt.time(),dt.offsetFromUtc());
}


int DRTimeZoneOffset()
{
  time_t t=time(&t);
  struct tm *tm=localtime(&t);
  time_t local_time=3600*tm->tm_hour+60*tm->tm_min+tm->tm_sec;
  tm=gmtime(&t);
  time_t gmt_time=3600*tm->tm_hour+60*tm->tm_min+tm->tm_sec;

  int offset=gmt_time-local_time;
  if(offset>43200) {
    offset=offset-86400;
  }
  if(offset<-43200) {
    offset=offset+86400;
  }

  return offset;
}
