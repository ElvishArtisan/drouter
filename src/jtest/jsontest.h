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

#include <QJsonDocument>
#include <QStringList>
#include <QTcpSocket>
#include <QTimer>

class JsonTest : public QObject
{
 Q_OBJECT;
 public:
 JsonTest(const QString &hostname,QObject *parent=0);
 ~JsonTest();
 
 signals:
  void testComplete(int testnum,const QString &testname,
		    bool passed,const QString &err_msg);
 
 public slots:
  void runTest(int testnum,const QString &testname,
	       const QStringList &send_json,
	       const QStringList &recv_json,int recv_start_linenum);

 public:
  static bool parseCheck(QString *err_msg,const QJsonParseError &jerr,
			 const QByteArray &json,int start_linenum);

 private slots:
  void exemplarErrorData();
  void connectedData();
  void disconnectedData();
  void errorOccurredData(QAbstractSocket::SocketError err);
  void readyReadData();

 private:
  int d_test_number;
  QString d_test_name;
  int d_recv_start_linenum;
  QStringList d_send_list;
  QJsonDocument d_exemplar_doc;
  QString d_hostname;
  QTcpSocket *d_socket; 
  QString d_exemplar_error_string;
  QTimer *d_exemplar_error_timer;
  bool d_processing;
};


#endif  // JTEST_H
