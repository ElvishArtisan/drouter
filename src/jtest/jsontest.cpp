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

#include <unistd.h>

#include <QProcess>

#include "jsontest.h"

JsonTest::JsonTest(const QString &hostname,uint16_t portnum,
		   ConnectionType conn_type,QObject *parent)
  : QObject(parent)
{
  d_hostname=hostname;
  d_port_number=portnum;
  d_connection_type=conn_type;
  d_json_socket=NULL;
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
  if(d_json_socket!=NULL) {
    d_json_socket->disconnect();
    delete d_json_socket;
  }
  if(d_web_socket!=NULL) {
    d_web_socket->disconnect();
    delete d_web_socket;
  }
}


void JsonTest::runTest(int testnum,const QString &testname,
		       const QString &send,
		       const QStringList &recv,int recv_start_linenum)
{
  QString err_msg;

  d_test_number=testnum;
  d_test_name=testname;
  d_recv_start_linenum=recv_start_linenum;
  d_send_json=send;
  d_exemplar_docs.clear();
  d_objects_remaining=recv.size();

  QJsonParseError jerr;
  for(int i=0;i<recv.size();i++) {
    d_exemplar_docs.
      push_back(QJsonDocument::fromJson(recv.at(i).toUtf8(),&jerr));
    if(jerr.error!=QJsonParseError::NoError) {
      d_exemplar_error_string=
	QString::asprintf("error in test %d exemplar JSON \"%s\": %s",
			  d_test_number,
			  recv.at(i).toUtf8().constData(),
			  err_msg.toUtf8().constData());
      d_exemplar_error_timer->start(0);
    }
  }

  //
  // ProtocolJ Connection
  //
  switch(d_connection_type) {
  case JsonTest::ConnectionTcp:
    d_json_socket=new DRJsonSocket(this);
    connect(d_json_socket,SIGNAL(connected()),this,SLOT(connectedData()));
    connect(d_json_socket,SIGNAL(disconnected()),this,SLOT(disconnectedData()));
    connect(d_json_socket,SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
	    this,SLOT(errorOccurredData(QAbstractSocket::SocketError)));
    connect(d_json_socket,SIGNAL(documentReceived(const QJsonDocument &)),
	    this,SLOT(documentReceivedData(const QJsonDocument &)));
    connect(d_json_socket,
	    SIGNAL(parseError(const QByteArray &,const QJsonParseError &)),
	    this,
	    SLOT(parseErrorData(const QByteArray &,const QJsonParseError &)));
    d_processing=true;
    d_json_socket->connectToHost(d_hostname,d_port_number);
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
    *err_msg=QString::asprintf("parse error at 1:%d: %s, JSON: %s",jerr.offset,
			       jerr.errorString().toUtf8().constData(),
			       json.constData());
    return false;
  }
  *err_msg="OK";

  return true;
}


QString JsonTest::makeDiff(const QJsonDocument &jdoc) const
{
  QString ret="";
  char tempdir[PATH_MAX]={"/tmp/jtestXXXXXX"};

  //
  // Create Temporary Directory
  //
  if(mkdtemp(tempdir)==NULL) {
    fprintf(stderr,"jtest: unable to create temporary directory [%s]\n",
	    strerror(errno));
    exit(1);
  }

  //
  // Generate JSON Files
  //
  QString orig_filepath=tempdir+QString("/exemplar.json");
  writeJson(d_exemplar_docs.at(0),orig_filepath);

  QString alt_filepath=tempdir+QString("/received.json");
  writeJson(jdoc,alt_filepath);

  //
  // Generate Diff Data
  //
  QProcess *proc=new QProcess();
  QStringList args;
  args.push_back("-u");
  args.push_back(orig_filepath);
  args.push_back(alt_filepath);
  proc->start("diff",args);
  proc->waitForFinished();
  if(proc->exitStatus()!=QProcess::NormalExit) {
    fprintf(stderr,"jtest: diff process crashed\n");
    exit(1);
  }
  if(proc->exitCode()>1) {
    fprintf(stderr,"jtest: diff process exited abnormally [%s]\n",
	    proc->readAllStandardError().constData());
    exit(1);
  }
  ret=QString::fromUtf8(proc->readAllStandardOutput());
  delete proc;

  //
  // Clean Up
  //
  unlink(orig_filepath.toUtf8());
  unlink(alt_filepath.toUtf8());
  if(rmdir(tempdir)!=0) {
    fprintf(stderr,"jtest: failed to delete temporary directory [%s]\n",
	    strerror(errno));
  }

  return ret;
}


void JsonTest::writeJson(const QJsonDocument &json,const QString &filepath)
  const
{
  FILE *f=NULL;

  if((f=fopen(filepath.toUtf8(),"w"))==NULL) {
    fprintf(stderr,"jtest: unable to create temporary file \"%s\" [%s]\n",
	    filepath.toUtf8().constData(),strerror(errno));
    exit(1);
  }
  fprintf(f,"%s",json.toJson().constData());
  fclose(f);
}


void JsonTest::completeTest(bool passed,const QString &err_msg,
			    const QString &diff)
{
  emit testComplete(d_test_number,d_test_name,passed,err_msg,diff);
  d_json_socket->deleteLater();
  d_json_socket=NULL;
  d_processing=false;
}


void JsonTest::exemplarErrorData()
{
  completeTest(false,d_exemplar_error_string,"");
}


void JsonTest::connectedData()
{
  switch(d_connection_type) {
  case JsonTest::ConnectionTcp:
    d_json_socket->write(d_send_json.toUtf8());
    break;

  case JsonTest::ConnectionWebSocket:
    d_web_socket->sendBinaryMessage(d_send_json.toUtf8());
    break;

  case JsonTest::ConnectionLast:
    break;
  }
}


void JsonTest::disconnectedData()
{
  if(d_processing) {
    switch(d_connection_type) {
    case JsonTest::ConnectionTcp:
      d_json_socket->deleteLater();
      d_json_socket=NULL;
      break;

    case JsonTest::ConnectionWebSocket:
      d_web_socket->deleteLater();
      d_web_socket=NULL;
      break;

    case JsonTest::ConnectionLast:
      break;
    }
    completeTest(false,"far end disconnected","");
  }
}


void JsonTest::errorOccurredData(QAbstractSocket::SocketError err)
{
  if(d_processing) {
    switch(d_connection_type) {
    case JsonTest::ConnectionTcp:
      d_json_socket->disconnect();
      d_json_socket->deleteLater();
      d_json_socket=NULL;
      break;

    case JsonTest::ConnectionWebSocket:
      d_web_socket->disconnect();
      d_web_socket->deleteLater();
      d_web_socket=NULL;
      break;

    case JsonTest::ConnectionLast:
      break;
    }
    completeTest(false,QString::asprintf("received socket error %d",err),"");
  }
}


void JsonTest::documentReceivedData(const QJsonDocument &jdoc)
{
  //  printf("*******************************************\n");
  //  printf("TEST: %d\n",d_test_number);
  //  printf("%s\n",jdoc.toJson().constData());
  //  printf("*******************************************\n");

  if(!d_exemplar_docs.contains(jdoc)) {
    completeTest(false,"returned json does not match exemplar(s)",
		 makeDiff(jdoc));
    return;
  }
  d_exemplar_docs.removeOne(jdoc);
  if(--d_objects_remaining==0) {
    completeTest(true,"OK","");
  }
}


void JsonTest::parseErrorData(const QByteArray &json,
			      const QJsonParseError &jerr)
{
  completeTest(false,
	     QString::asprintf("parse error at %d in returned json \"%s\": %s",
			       jerr.offset,json.constData(),
			       jerr.errorString().toUtf8().constData()),"");
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

  if(!JsonTest::parseCheck(&err_msg,jerr,json,d_recv_start_linenum)) {
    completeTest(false,QString::asprintf("parse error in returned json: %s",
					 err_msg.toUtf8().constData()),"");
  }
  else {
    if(!d_exemplar_docs.contains(recv_doc)) {
      completeTest(false,"returned json does not match exemplar(s)",
		   makeDiff(recv_doc));
    }
    else {
      d_exemplar_docs.removeOne(recv_doc);
      if(d_exemplar_docs.size()==0) {
	completeTest(true,"OK","");
      }
    }
  }
}
