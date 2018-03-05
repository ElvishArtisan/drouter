// scripthost.cpp
//
// Run a state script.
//
//   (C) Copyright 2018 Fred Gleason <fredg@paravelsystems.com>
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

#include <syslog.h>

#include "scripthost.h"

ScriptHost::ScriptHost(const QString &exec,QObject *parent)
  : QObject(parent)
{
  script_arguments.push_back(exec);

  script_process=NULL;
  script_restart=false;

  script_restart_timer=new QTimer(this);
  script_restart_timer->setSingleShot(true);
  connect(script_restart_timer,SIGNAL(timeout()),this,SLOT(start()));
}


ScriptHost::~ScriptHost()
{
  delete script_restart_timer;
  if(script_process!=NULL) {
    delete script_process;
  }
}


void ScriptHost::start()
{
  script_restart=true;
  script_process=new QProcess(this);
  connect(script_process,SIGNAL(finished(int,QProcess::ExitStatus)),
	  this,SLOT(finishedData(int,QProcess::ExitStatus)));
  connect(script_process,SIGNAL(error(QProcess::ProcessError)),
	  this,SLOT(errorData(QProcess::ProcessError)));
  script_process->start("/usr/bin/python",script_arguments);
  syslog(LOG_DEBUG,"starting state script \"%s\"",
	 (const char *)script_arguments.at(0).toUtf8());
}


void ScriptHost::terminate()
{
  script_restart=false;
  script_process->terminate();
}


void ScriptHost::kill()
{
  script_restart=false;
  script_process->kill();
}


void ScriptHost::finishedData(int exit_code,QProcess::ExitStatus status)
{
  if(status==QProcess::CrashExit) {
    if(script_restart) {
      syslog(LOG_WARNING,"script \"%s\" crashed!",
	     (const char *)script_arguments.at(0).toUtf8());
      script_restart_timer->start(SCRIPTHOST_RESTART_INTERVAL);
    }
    else {
      syslog(LOG_DEBUG,"script \"%s\" terminated",
	     (const char *)script_arguments.at(0).toUtf8());
    }
  }
  else {
    if(exit_code==0) {
      syslog(LOG_INFO,"script \"%s\" exited normally",
	     (const char *)script_arguments.at(0).toUtf8());
    }
    else {
      syslog(LOG_INFO,"script \"%s\" exited with code %d",
	     (const char *)script_arguments.at(0).toUtf8(),exit_code);
      syslog(LOG_INFO,"script output: %s\n",
	     (const char *)script_process->readAllStandardError());
      if(script_restart) {
	script_restart_timer->start(SCRIPTHOST_RESTART_INTERVAL);
      }
    }
  }
  script_process->deleteLater();
  script_process=NULL;
}


void ScriptHost::errorData(QProcess::ProcessError err)
{
}
