// jsontest.h
//
// Class for running a ProtocolJ test rule
//
//   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef JSONTEST_H
#define JSONTEST_H

#include <stdint.h>

#include <QJsonDocument>
#include <QList>
#include <QStringList>
#include <QTimer>
#include <QWebSocket>

#include <drouter/drjsonsocket.h>

class JsonTest : public QObject
{
 Q_OBJECT;
 public:
  enum ConnectionType {ConnectionTcp=0,ConnectionWebSocket=1,ConnectionLast=2};
  JsonTest(const QString &hostname,uint16_t portnum,
	   ConnectionType conn_type,QObject *parent=0);
 ~JsonTest();
 
 signals:
  void testComplete(int testnum,const QString &testname,
		    bool passed,const QString &err_msg,
		    const QString &diff);
 
 public slots:
  void runTest(int testnum,const QString &testname,const QString &send_json,
	       const QStringList &recv_jsons,int recv_start_linenum);

 public:
  static bool parseCheck(QString *err_msg,const QJsonParseError &jerr,
			 const QByteArray &json,int start_linenum);

 protected:
  QString makeDiff(const QJsonDocument &jdoc) const;
  void writeJson(const QJsonDocument &json,const QString &filepath) const; 
  void completeTest(bool passed,const QString &err_msg,const QString &diff);
									 
 private slots:
  void exemplarErrorData();
  void connectedData();
  void disconnectedData();
  void errorOccurredData(QAbstractSocket::SocketError err);
  void documentReceivedData(const QJsonDocument &jdoc);
  void parseErrorData(const QByteArray &json,const QJsonParseError &jerr);
  void binaryMessageReceivedData(const QByteArray &);

private:
  // Persistent variables
  QString d_hostname;
  uint16_t d_port_number;
  ConnectionType d_connection_type;
  DRJsonSocket *d_json_socket;
  QWebSocket *d_web_socket;

  // Per-test variables
  int d_test_number;
  QString d_test_name;
  int d_recv_start_linenum;
  QString d_send_json;
  QList<QJsonDocument> d_exemplar_docs;
  int d_objects_remaining;
  QString d_exemplar_error_string;
  QTimer *d_exemplar_error_timer;
  bool d_processing;
};


#endif  // JTEST_H
