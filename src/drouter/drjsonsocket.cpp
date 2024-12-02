// drjsonsocket.cpp
//
// Qt-style socket class for receiving concaternated JSON
//
//   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
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

#include "drjsonsocket.h"

DRJsonSocket::DRJsonSocket(QObject *parent)
  : QTcpSocket(parent)
{
  d_bracket_count=0;
  d_in_string=false;
  
  connect(this,SIGNAL(readyRead()),this,SLOT(readyReadData()));
}


void DRJsonSocket::readyReadData()
{
  QJsonParseError jerr;
  QByteArray data=readAll();

  for(int i=0;i<data.size();i++) {
    switch(data.at(i)) {
    case '"':
      d_in_string=!d_in_string;
      d_accum+=data.at(i);
      break;

    case '{':
      if(!d_in_string) {
	d_bracket_count++;
      }
      d_accum+=data.at(i);
      break;

    case '}':
      if(!d_in_string) {
	d_bracket_count--;
	d_accum+=data.at(i);
	if(d_bracket_count==0) {
	  QJsonDocument jdoc=QJsonDocument::fromJson(d_accum,&jerr);
	  if(jdoc.isNull()) {
	    emit parseError(d_accum,jerr);
	  }
	  else {
	    emit documentReceived(jdoc);
	  }
	  d_accum.clear();
	}
	if(d_bracket_count<0) {
	  QJsonParseError jerr;
	  jerr.error=QJsonParseError::MissingObject;
	  jerr.offset=d_accum.size()-1;
	  emit parseError(d_accum,jerr);
	  d_accum.clear();
	  d_bracket_count=0;
	}
      }
      break;

    default:
      d_accum+=data.at(i);
      break;
    }
  }
}
