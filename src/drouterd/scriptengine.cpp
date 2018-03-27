// scriptengine.cpp
//
// Run state scripts.
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

#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <QDir>

#include "scriptengine.h"

ScriptEngine::ScriptEngine(QObject *parent)
{
  script_scan_timer=new QTimer(this);
  connect(script_scan_timer,SIGNAL(timeout()),this,SLOT(scanData()));
}


void ScriptEngine::start()
{
  pid_t pid=0;
  QDir dir(SCRIPTENGINE_SCRIPTS_DIRECTORY);
  QStringList filters;
  filters.push_back(SCRIPTENGINE_SCRIPTS_FILTER);
  QStringList scripts=dir.entryList(filters,QDir::Files|QDir::Executable);

  for(int i=0;i<scripts.size();i++) {
    syslog(LOG_INFO,"starting script \"%s\"",
	   (const char *)Pathname(scripts.at(i)).toUtf8());
    if((pid=fork())==0) {
      execl(Pathname(scripts.at(i)).toUtf8(),scripts.at(i).toUtf8(),
	    (char *)NULL);
      syslog(LOG_WARNING,"failed to start script \"%s\" [%s]",
	     (const char *)Pathname(scripts.at(i)).toUtf8(),strerror(errno));
      exit(1);
    }
    script_scripts[pid]=scripts.at(i);
  }
  script_scan_timer->start(1000);
}


void ScriptEngine::stop()
{
  script_scan_timer->stop();
  for(QMap<pid_t,QString>::const_iterator it=script_scripts.begin();
      it!=script_scripts.end();it++) {
    kill(it.key(),SIGKILL);
  }
}


void ScriptEngine::scanData()
{
  pid_t pid=0;
  int status=0;

  while((pid=waitpid(-1,&status,WNOHANG))>0) {
    QString script(script_scripts[pid]);
    int exit_code=-1;
    if(WIFEXITED(status)) {
      exit_code=WEXITSTATUS(status);
    }
    if(exit_code<0) {
      syslog(LOG_WARNING,"script \"%s\" crashed, restarting",
	     (const char *)Pathname(script).toUtf8());
    }
    else {
      syslog(LOG_WARNING,"script \"%s\" exited with code %d, restarting",
	     (const char *)Pathname(script).toUtf8(),exit_code);
    }
    script_scripts.remove(pid);
    if((pid=fork())==0) {
      execl(Pathname(script).toUtf8(),script.toUtf8(),(char *)NULL);
      syslog(LOG_WARNING,"failed to restart script \"%s\" [%s]",
	     (const char *)Pathname(script).toUtf8(),strerror(errno));
      exit(1);
    }
    script_scripts[pid]=script;
  }
}


QString ScriptEngine::Pathname(const QString &script) const
{
  return SCRIPTENGINE_SCRIPTS_DIRECTORY+"/"+script;
}
