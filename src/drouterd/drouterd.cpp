// drouterd.cpp
//
// Dynamic router service for Livewire networks
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include <QCoreApplication>

#include <sy/sycmdswitch.h>

#ifdef LIBSYSTEMD
#include <systemd/sd-daemon.h>
#endif  // LIBSYSTEMD

#include "drouterd.h"

bool global_reload=false;
bool global_exiting=false;

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  main_no_scripts=false;
  main_protocol_socks[0]=-1;
  main_protocol_socks[1]=-1;
  int n;
  bool no_protocols=false;
  QString err_msg;
  int log_options=0;
  SyCmdSwitch *cmd=
    new SyCmdSwitch(qApp->argc(),qApp->argv(),"drouterd",VERSION,DROUTERD_USAGE);
  for(unsigned i=0;i<(cmd->keys());i++) {
    if(cmd->key(i)=="-d") {
      log_options=log_options|LOG_PERROR;;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--no-scripts") {
      main_no_scripts=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--no-protocols") {
      no_protocols=true;
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"drouterd: unknown option\n");
      exit(256);
    }
  }

  //
  // Configuration
  //
  main_config=new Config();
  main_config->load();

  //
  // Open Syslog
  //
  openlog("drouterd",log_options,LOG_DAEMON);


#ifdef LIBSYSTEMD
  //
  // Get sockets from SystemD
  //
  n=sd_listen_fds(0);
  if(n>0) {
    if(n==2) {
      main_protocol_socks[0]=SD_LISTEN_FDS_START+0;
      main_protocol_socks[1]=SD_LISTEN_FDS_START+1;
    }
    else {
      syslog(LOG_ERR,"error receiving sockets from Systemd, aborting");
      exit(1);
    }
  }
#endif  // LIBSYSTEMD

  //
  // Exit Notifier
  //
  main_exit_notifier=new ExitNotifier(this);
  connect(main_exit_notifier,SIGNAL(aboutToExit()),this,SLOT(exitData()));

  //
  // State Scripts
  //
  main_script_engine=new ScriptEngine();

  main_scripts_timer=new QTimer(this);
  main_scripts_timer->setSingleShot(true);
  connect(main_scripts_timer,SIGNAL(timeout()),this,SLOT(scriptsData()));

  //
  // Tethering
  //
  main_tether=new Tether(this);
  connect(main_tether,SIGNAL(instanceStateChanged(bool)),
	  this,SLOT(instanceStateChangedData(bool)));
  connect(main_exit_notifier,SIGNAL(aboutToExit()),main_tether,SLOT(cleanup()));

  //
  // Start Router Process
  //
  main_drouter=new DRouter(main_protocol_socks,this);
  connect(main_tether,SIGNAL(instanceStateChanged(bool)),
	  main_drouter,SLOT(setWriteable(bool)));
  if(!main_drouter->start(&err_msg)) {
    syslog(LOG_ERR,"core router startup error: %s, aborting",
	   (const char *)err_msg.toUtf8());
    exit(1);
  }

  //
  // Protocol Start Timer
  //
  main_protocol_timer=new QTimer(this);
  main_protocol_timer->setSingleShot(true);
  connect(main_protocol_timer,SIGNAL(timeout()),this,SLOT(protocolData()));
  if(!no_protocols) {
    main_protocol_timer->start(DROUTERD_PROTOCOL_START_INTERVAL);
  }
}


void MainObject::protocolData()
{
  QString err_msg;
  pid_t pid=0;

  if((pid=fork())==0) {
    if(main_protocol_socks[0]<0) {
      execl("/usr/sbin/dprotod","dprotod","--protocol-d",(char *)NULL);
    }
    else {
      execl("/usr/sbin/dprotod","dprotod","--protocol-d","--systemd",
	    (char *)NULL);
    }
  }
  main_protocol_pids.push_back(pid);
  syslog(LOG_INFO,"started Protocol D protocol");

  if((pid=fork())==0) {
    if(main_protocol_socks[1]<0) {
      execl("/usr/sbin/dprotod","dprotod","--protocol-sa",(char *)NULL);
    }
    else {
      execl("/usr/sbin/dprotod","dprotod","--protocol-sa","--systemd",
	    (char *)NULL);
    }
  }
  main_protocol_pids.push_back(pid);
  syslog(LOG_INFO,"started Software Authority protocol");
  if(!main_no_scripts) {
    main_scripts_timer->start(5000);
  }
  if(!main_tether->start(main_config,&err_msg)) {
    fprintf(stderr,"drouterd: tethering system failed to start [%s]\n",
	    (const char *)err_msg.toUtf8());
    exit(1);
  }
}


void MainObject::scriptsData()
{
  main_script_engine->start();
}


void MainObject::instanceStateChangedData(bool state)
{
  if(state) {
    syslog(LOG_INFO,"we are now the active instance");
  }
}


void MainObject::exitData()
{
  main_drouter->disconnect();
  main_drouter->setWriteable(false);
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();
  return a.exec();
}
