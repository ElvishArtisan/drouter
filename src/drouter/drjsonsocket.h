// drjsonsocket.h
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

#ifndef DRJSONSOCKET_H
#define DRJSONSOCKET_H

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTcpSocket>

class DRJsonSocket : public QTcpSocket
{
 Q_OBJECT
 public:
  DRJsonSocket(QObject *parent);

 signals:
  void documentReceived(const QJsonDocument &jdoc);
  void parseError(const QByteArray &json,const QJsonParseError &jerr);
  
 private slots:
  void readyReadData();

 private:
  QByteArray d_accum;
  int d_bracket_count;
  bool d_in_string;
};


#endif  // DRJSONSOCKET_H
