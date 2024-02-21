// drjson.h
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

#ifndef DRJSON_J_H
#define DRJSON_J_H

#include <QDateTime>
#include <QString>
#include <QVariant>

QString DRJsonPadding(int padding);
QString DRJsonEscape(const QString &str);
QString DRJsonNullField(const QString &name,int padding=0,
			bool final=false);
QString DRJsonField(const QString &name,bool value,int padding=0,
		    bool final=false);
QString DRJsonField(const QString &name,int value,int padding=0,
		    bool final=false);
QString DRJsonField(const QString &name,unsigned value,int padding=0,
		    bool final=false);
QString DRJsonField(const QString &name,const QString &value,
		    int padding=0,bool final=false);
QString DRJsonField(const QString &name,const QDateTime &value,
		    int padding=0,bool final=false);
QString DRJsonField(const QString &name,const QTime &value,int utc_offset,
		    int padding=0,bool final=false);
QString DRJsonField(const QString &name,const QVariant &value,int padding=0,
		    bool final=false);
QString DRJsonCloseBlock(bool final);

//
// XML xs:date format
//
QDate DRParseXmlDate(const QString &str,bool *ok);
QString DRWriteXmlDate(const QDate &date);

//
// XML xs:time format
//
QTime DRParseXmlTime(const QString &str,bool *ok,int *day_offset=NULL);
QString DRWriteXmlTime(const QTime &time,int utc_offset);

//
// XML xs:dateTime format
//
QDateTime DRParseXmlDateTime(const QString &str,bool *ok);
QString DRWriteXmlDateTime(const QDateTime &dt);

int DRTimeZoneOffset();


#endif  // DRJSON_H
