// jsontest.cpp
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

#include "jsontest.h"

JsonTest::JsonTest(const QString &hostname,uint16_t portnum,
		   ConnectionType conn_type,QObject *parent)
  : QObject(parent)
{
  d_hostname=hostname;
  d_port_number=portnum;
  d_connection_type=conn_type;
  d_tcp_socket=NULL;
  d_web_socket=NULL;
  d_processing=false;

  //
  // Exemplar Error Handler
  //
  d_exemplar_error_timer=new QTimer(this);
  d_exemplar_error_timer->setSingleShot(true);
  connect(d_exemplar_error_timer,SIGNAL(timeout()),
	  this,SLOT(exemplarErrorData()));
}


JsonTest::~JsonTest()
{
  if(d_tcp_socket!=NULL) {
    d_tcp_socket->disconnect();
    delete d_tcp_socket;
  }
  if(d_web_socket!=NULL) {
    d_web_socket->disconnect();
    delete d_web_socket;
  }
}


void JsonTest::runTest(int testnum,const QString &testname,
		       const QStringList &send,
		       const QStringList &recv,int recv_start_linenum)
{
  QString err_msg;
  /*
  printf("SEND: %s\n",send.join("\n").toUtf8().constData());
  printf("*****************************************************************\n");
  printf("RECV: %s\n",recv.join("\n").toUtf8().constData());
  printf("*****************************************************************\n");
  */
  d_test_number=testnum;
  d_test_name=testname;
  d_recv_start_linenum=recv_start_linenum;
  d_send_list=send;

  QJsonParseError jerr;
  QByteArray recv_json=recv.join("\n").toUtf8();
  d_exemplar_doc=QJsonDocument::fromJson(recv_json,&jerr);
  if(!JsonTest::parseCheck(&err_msg,jerr,recv_json,recv_start_linenum)) {
    d_exemplar_error_string=
      QString::asprintf("error in test %d exemplar JSON: %s",d_test_number,
			err_msg.toUtf8().constData());
    d_exemplar_error_timer->start(0);
  }

  //
  // ProtocolJ Connection
  //
  switch(d_connection_type) {
  case JsonTest::ConnectionTcp:
    d_tcp_socket=new QTcpSocket(this);
    connect(d_tcp_socket,SIGNAL(connected()),this,SLOT(connectedData()));
    connect(d_tcp_socket,SIGNAL(disconnected()),this,SLOT(disconnectedData()));
    connect(d_tcp_socket,SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
	    this,SLOT(errorOccurredData(QAbstractSocket::SocketError)));
    connect(d_tcp_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
    d_processing=true;
    d_tcp_socket->connectToHost(d_hostname,d_port_number);
    break;

  case JsonTest::ConnectionWebSocket:
    d_web_socket=
      new QWebSocket(QString(),QWebSocketProtocol::VersionLatest,this);
    connect(d_web_socket,SIGNAL(connected()),this,SLOT(connectedData()));
    connect(d_web_socket,SIGNAL(disconnected()),this,SLOT(disconnectedData()));
    connect(d_web_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	    this,SLOT(errorOccurredData(QAbstractSocket::SocketError)));
    connect(d_web_socket,SIGNAL(binaryMessageReceived(const QByteArray &)),
	    this,SLOT(binaryMessageReceivedData(const QByteArray &)));
    d_processing=true;
    d_web_socket->open(QString::asprintf("ws://%s:%u/drouter",
					 d_hostname.toUtf8().constData(),
					 0xFFFF&d_port_number));
    break;

  case JsonTest::ConnectionLast:
    break;
  }
}


bool JsonTest::parseCheck(QString *err_msg,const QJsonParseError &jerr,
			  const QByteArray &json,int start_linenum)
{
  if(jerr.error!=QJsonParseError::NoError) {
    int linenum=start_linenum;
    int colnum=0;
    for(int i=0;i<jerr.offset;i++) {
      if(json.at(i)==QChar('\n')) {
	linenum++;
	colnum=0;
      }
      else {
	colnum++;
      }
    }    
    *err_msg=QString::asprintf("parse error at %d:%d: %s",linenum,colnum,
			       jerr.errorString().toUtf8().constData());
    return false;
  }
  *err_msg="OK";

  return true;
}


void JsonTest::exemplarErrorData()
{
  emit testComplete(d_test_number,d_test_name,false,d_exemplar_error_string);
}


void JsonTest::connectedData()
{
  //  printf("connected!\n");
  switch(d_connection_type) {
  case JsonTest::ConnectionTcp:
    d_tcp_socket->write(d_send_list.join("\n").toUtf8());
    break;

  case JsonTest::ConnectionWebSocket:
    d_web_socket->sendBinaryMessage(d_send_list.join("\n").toUtf8());
    break;

  case JsonTest::ConnectionLast:
    break;
  }
}


void JsonTest::disconnectedData()
{
  //  printf("disconnected!\n");
  if(d_processing) {
    switch(d_connection_type) {
    case JsonTest::ConnectionTcp:
      d_tcp_socket->deleteLater();
      d_tcp_socket=NULL;
      break;

    case JsonTest::ConnectionWebSocket:
      d_web_socket->deleteLater();
      d_web_socket=NULL;
      break;

    case JsonTest::ConnectionLast:
      break;
    }
    emit testComplete(d_test_number,d_test_name,false,"far end disconnected");
  }
}


void JsonTest::errorOccurredData(QAbstractSocket::SocketError err)
{
  if(d_processing) {
    switch(d_connection_type) {
    case JsonTest::ConnectionTcp:
      d_tcp_socket->disconnect();
      d_tcp_socket->deleteLater();
      d_tcp_socket=NULL;
      break;

    case JsonTest::ConnectionWebSocket:
      d_web_socket->disconnect();
      d_web_socket->deleteLater();
      d_web_socket=NULL;
      break;

    case JsonTest::ConnectionLast:
      break;
    }
    emit testComplete(d_test_number,d_test_name,false,
		      QString::asprintf("received socket error %d",err));
  }
}


void JsonTest::readyReadData()
{
  QString err_msg;
  QByteArray json=d_tcp_socket->readAll();
  d_processing=false;
  d_tcp_socket->disconnect();
  d_tcp_socket->deleteLater();
  d_tcp_socket=NULL;

  QJsonParseError jerr;
  QJsonDocument recv_doc=QJsonDocument::fromJson(json,&jerr);

//  printf("RETURNED STARTS\n");
//  printf("%s",json.constData());
//  printf("RETURNED ENDS\n");

  if(!JsonTest::parseCheck(&err_msg,jerr,json,d_recv_start_linenum)) {
    emit testComplete(d_test_number,d_test_name,false,
		      QString::asprintf("parse error in returned json: %s",
					err_msg.toUtf8().constData()));
  }
  else {
    if(d_exemplar_doc!=recv_doc) {
      emit testComplete(d_test_number,d_test_name,false,
			"returned json does not match exemplar");
    }
    else {
      emit testComplete(d_test_number,d_test_name,true,"OK");
    }
  }
}


void JsonTest::binaryMessageReceivedData(const QByteArray &json)
{
  QString err_msg;
  d_processing=false;

  d_web_socket->disconnect();
  d_web_socket->deleteLater();
  d_web_socket=NULL;

  QJsonParseError jerr;
  QJsonDocument recv_doc=QJsonDocument::fromJson(json,&jerr);

//  printf("RETURNED STARTS\n");
//  printf("%s",json.constData());
//  printf("RETURNED ENDS\n");

  if(!JsonTest::parseCheck(&err_msg,jerr,json,d_recv_start_linenum)) {
    emit testComplete(d_test_number,d_test_name,false,
		      QString::asprintf("parse error in returned json: %s",
					err_msg.toUtf8().constData()));
  }
  else {
    if(d_exemplar_doc!=recv_doc) {
      emit testComplete(d_test_number,d_test_name,false,
			"returned json does not match exemplar");
    }
    else {
      emit testComplete(d_test_number,d_test_name,true,"OK");
    }
  }
}
