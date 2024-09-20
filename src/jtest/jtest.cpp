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
#include <QSignalMapper>

#include <sy5/sycmdswitch.h>
#include <sy5/syconfig.h>
#include <sy5/syinterfaces.h>
#include <sy5/syprofile.h>

#include "jtest.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  QString hostname="localhost";
  unsigned portnum=9600;
  QString tests;
  FILE *testsfile=NULL;
  bool ok=false;
  JsonTest::ConnectionType conntype=JsonTest::ConnectionTcp;
  d_linenum=1;
  d_passed=0;
  d_failed=0;
  
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
  // Open Test Stream
  //
  if((testsfile=fopen(tests.toUtf8(),"r"))==NULL) {
    fprintf(stderr,"jtest: %s\n",strerror(errno));
    exit(1);
  }
  d_tests_stream=new QTextStream(testsfile,QIODevice::ReadOnly);
  
  //
  // Create Test Jig
  //
  d_json_test=new JsonTest(hostname,portnum,conntype,this);
  connect(d_json_test,
	  SIGNAL(testComplete(int,const QString &,bool,const QString&)),
	  this,
	  SLOT(testCompleteData(int,const QString &,bool,const QString &)));

  //
  // Kick It Off
  //
  testCompleteData(0,"KICKOFF!",true,"KICKOFF!");
}


void MainObject::testCompleteData(int testnum,const QString &testname,
				  bool passed,const QString &err_msg)
{
  /*
  printf("testCompleteData(%d,%s,%d,%s)\n",
	 testnum,testname.toUtf8().constData(),passed,
	 err_msg.toUtf8().constData());
  */
  int next_testnum=0;
  QString next_testname;
  bool ok=false;
  bool receiving=false;
  QStringList send;
  QStringList recv;
  int next_recv_start_linenum=0;

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
    }
  }

  if(d_tests_stream->atEnd()) {  // We're done!
    Finish();
  }
  while(!d_tests_stream->atEnd()) {
    QString line=d_tests_stream->readLine();
    if(line.left(7)=="=======") {
      QStringList f0=line.split(":",Qt::KeepEmptyParts);
      if(f0.size()!=3) {
	fprintf(stderr,"jtest: malformed test header at line %d\n",1+d_linenum);
	exit(1);
      }
      next_testnum=f0.at(1).toInt(&ok);
      if(!ok) {
	fprintf(stderr,"jtest: malformed test header at line %d\n",1+d_linenum);
	exit(1);
      }
      next_testname=f0.at(2);
      next_recv_start_linenum=1+d_linenum;
      receiving=true;
    }
    else {
      if(line.left(7)=="*******") {
	QStringList f0=line.split(":",Qt::KeepEmptyParts);
	if(f0.size()!=2) {
	  fprintf(stderr,"jtest: malformed test footer at line %d\n",
		  1+d_linenum);
	  exit(1);
	}
	int end_next_testnum=f0.at(1).toInt(&ok);
	if((!ok)||(next_testnum!=end_next_testnum)) {
	  fprintf(stderr,"jtest: mismatched test footer number at line %d\n",
		  1+d_linenum);
	  exit(1);
	}
	receiving=false;
	d_json_test->runTest(next_testnum,next_testname,send,recv,
			     next_recv_start_linenum);
	send.clear();
	recv.clear();
	d_linenum++;
	return;
      }
      else {
	if(receiving) {
	  recv.push_back(line);
	}
	else {
	  send.push_back(line);
	}
      }
    }
    d_linenum++;
  }
  Finish();
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
