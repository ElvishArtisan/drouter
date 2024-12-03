// jtest.cpp
//
// jtest() Testing harness for ProtocolJ
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <QCoreApplication>
#include <QJsonObject>
#include <QSignalMapper>

#include <sy5/sycmdswitch.h>
#include <sy5/syconfig.h>
#include <sy5/syinterfaces.h>
#include <sy5/symcastsocket.h>
#include <sy5/syprofile.h>

#include "jtest.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  QString hostname="localhost";
  unsigned portnum=9600;
  QString tests;
  bool ok=false;
  JsonTest::ConnectionType conntype=JsonTest::ConnectionTcp;
  QString err_msg;
  d_test_number=0;
  d_linenum=1;
  d_passed=0;
  d_failed=0;
  d_generate_diffs=false;

  SyCmdSwitch *cmd=new SyCmdSwitch("jtest",VERSION,JTEST_USAGE);
  for(int i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--connection-type") {
      if(cmd->value(i).toLower()=="tcp") {
	conntype=JsonTest::ConnectionTcp;
      }
      else {
	if(cmd->value(i).toLower()=="websocket") {
	  conntype=JsonTest::ConnectionWebSocket;
	}
	else {
	  fprintf(stderr,"jtest: unknown connection type \"%s\"\n",
		  cmd->value(i).toUtf8().constData());
	  exit(1);
	}
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--generate-diffs") {
      d_generate_diffs=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--hostname") {
      QStringList f0=cmd->value(i).split(":",Qt::KeepEmptyParts);
      if((f0.size()==0)||(f0.size()>2)) {
	fprintf(stderr,"invalid \"--hostname\" argument\n");
	exit(1);
      }
      if(f0.size()==2) {
	portnum=f0.at(1).toUInt(&ok);
	if((!ok)||(portnum>0xFFFF)) {
	  fprintf(stderr,"invalid port value in \"--hostname\" argument\n");
	  exit(1);
	}
      }
      hostname=f0.at(0);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--tests") {
      tests=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"jtest: unknown option\n");
      exit(256);
    }
  }
  if(tests.isEmpty()) {
    fprintf(stderr,"jtest: \"--tests\" must be specified\n");
    exit(1);
  }

  //
  // Open Test Cases
  //
  d_test_profile=new Profile();
  if(!d_test_profile->loadFile(tests,&err_msg)) {
    fprintf(stderr,"jtest: %s\n",err_msg.toUtf8().constData());
    exit(1);
  }
  
  //
  // Create Test Jig
  //
  d_json_test=new JsonTest(hostname,portnum,conntype,this);
  connect(d_json_test,
	  SIGNAL(testComplete(int,const QString &,bool,const QString &,
			      const QString &)),
	  this,
	  SLOT(testCompleteData(int,const QString &,bool,const QString &,
				const QString &)));

  //
  // Run Prologue Commands
  //
  d_prologue_commands=d_test_profile->stringValues("Prologue","Command");
  if(d_prologue_commands.size()>0) {
    d_prologue_timer=new QTimer(this);
    d_prologue_timer->setSingleShot(true);
    connect(d_prologue_timer,SIGNAL(timeout()),
	    this,SLOT(prologueTimeoutData()));
    d_prologue_socket=new DRJsonSocket(this);
    connect(d_prologue_socket,SIGNAL(connected()),
	    this,SLOT(prologueConnectedData()));
    connect(d_prologue_socket,SIGNAL(documentReceived(const QJsonDocument &)),
	    this,SLOT(prologueDocumentReceivedData(const QJsonDocument &)));
    connect(d_prologue_socket,
	    SIGNAL(parseError(const QByteArray &,const QJsonParseError &)),
	    this,
	    SLOT(prologueParseErrorData(const QByteArray &,
					const QJsonParseError &)));
    connect(d_prologue_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	    this,SLOT(prologueErrorData(QAbstractSocket::SocketError)));
    d_prologue_socket->connectToHost(hostname,portnum);
  }
  else {
    //
    // Kick It Off
    //
    testCompleteData(0,"KICKOFF!",true,"KICKOFF!",QString());
  }
}


void MainObject::testCompleteData(int testnum,const QString &testname,
				  bool passed,const QString &err_msg,
				  const QString &diff)
{
  /*
  printf("testCompleteData(%d,%s,%d,%s)\n",
	 testnum,testname.toUtf8().constData(),passed,
	 err_msg.toUtf8().constData());
  */
  QString next_testname;
  bool ok=false;
  QStringList send;
  QStringList recv;

  //
  // Process Completed Test
  //
  if(testnum>0) {  // Ignore the "kickoff"
    if(passed) {
      d_passed++;
      printf("Test %d [%s]: PASS\n",testnum,testname.toUtf8().constData());
    }
    else {
      d_failed++;
      printf("Test %d [%s]: FAIL (%s)\n",testnum,testname.toUtf8().constData(),
	     err_msg.toUtf8().constData());
      if(d_generate_diffs) {
	printf("================================ Diff Starts ================================\n");
	printf("%s",diff.toUtf8().constData());
	printf("================================= Diff Ends =================================\n");
      }
    }
  }

  //
  // Load Next Test Case
  //
  d_test_number++;
  QString section=QString::asprintf("Test%d",d_test_number);
  QString name=d_test_profile->stringValue(section,"Name","",&ok);
  if(!ok) {  // We're done
    Finish();
  }
  QString input=d_test_profile->stringValue(section,"Input");
  QString output=d_test_profile->stringValue(section,"Output");
  d_json_test->runTest(d_test_number,name,input,output,1);
}


void MainObject::prologueConnectedData()
{
  for(int i=0;i<d_prologue_commands.size();i++) {
    d_prologue_socket->write(d_prologue_commands.at(i).toUtf8());
  }
  d_prologue_socket->write("{\"ping\":null}");
}


void MainObject::prologueDocumentReceivedData(const QJsonDocument &jdoc)
{
  if(jdoc.object().contains("pong")) {
    d_prologue_socket->deleteLater();
    d_prologue_socket=NULL;
    d_prologue_timer->start(1000);
  }
}


void MainObject::prologueParseErrorData(const QByteArray &json,
					const QJsonParseError &jerr)
{
  fprintf(stderr,"jtest: prologue command return \"%s\": %s at %d\n",
	  json.constData(),jerr.errorString().toUtf8().constData(),jerr.offset);
  exit(1);
}


void MainObject::prologueErrorData(QAbstractSocket::SocketError err)
{
  fprintf(stderr,"prologue socket error: %s\n",
	  SyMcastSocket::socketErrorText(err).toUtf8().constData());
  exit(1);
}


void MainObject::prologueTimeoutData()
{
  testCompleteData(0,"KICKOFF!",true,"KICKOFF!",QString());
}


void MainObject::Finish()
{
    printf("\n");
    printf("%d out of %d tests passed.\n",d_passed,d_passed+d_failed);
    if(d_failed==0) {
      exit(0);
    }
    exit(1);
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();
  return a.exec();
}
