// sendmailtest.cpp
//
// Test the email sending routines.
//
//   (C) Copyright 2021-2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <QApplication>

#include <sy5/sycmdswitch.h>

#include <drsendmail.h>

#include "sendmailtest.h"

MainObject::MainObject(QObject *parent)
  :QObject(parent)
{
  QString err_msg;
  QString from_addr;
  QString to_addrs;
  QString cc_addrs;
  QString bcc_addrs;
  QString subject;
  QString body;
  QString body_file;
  bool dry_run=false;
  FILE *f=NULL;
  QByteArray raw;
  char data[1024];
  size_t n;

  //
  // Read Command Options
  //
  SyCmdSwitch *cmd=new SyCmdSwitch("sendmailtest",VERSION,SENDMAILTEST_USAGE);
  for(int i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--from-addr") {
      from_addr=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--to-addrs") {
      to_addrs=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--cc-addrs") {
      cc_addrs=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--bcc-addrs") {
      bcc_addrs=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--subject") {
      subject=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--body") {
      body=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--body-file") {
      body_file=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--dry-run") {
      dry_run=true;
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"sendmailtest: unknown option \"%s\"\n",
	      cmd->key(i).toUtf8().constData());
      exit(1);
    }
  }

  //
  // Sanity Checks
  //
  if((!body.isEmpty())&&(!body_file.isEmpty())) {
    fprintf(stderr,
	    "sendmailtest: --body and --body-file are mutually exclusive\n");
    exit(1);
  }

  //
  // Load Message Body
  //
  if(!body_file.isEmpty()) {
    if((f=fopen(body_file.toUtf8(),"r"))==NULL) {
      perror("sendmailtest");
      exit(256);
    }
    while((n=fread(data,1,1024,f))>0) {
      raw+=QByteArray(data,n);
    }
    fclose(f);
    body=QString::fromUtf8(raw);
  }

  if(!DRSendMail(&err_msg,subject,body,
	       from_addr,to_addrs,cc_addrs,bcc_addrs,dry_run)) {
    fprintf(stderr,"%s\n",err_msg.toUtf8().constData());
    exit(256);
  }

  exit(1);
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv,false);
  new MainObject();
  return a.exec();
}
