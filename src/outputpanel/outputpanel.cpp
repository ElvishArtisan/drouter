// outputpanel.cpp
//
// An applet for controling an SA output
//
//   (C) Copyright 2002-2017 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
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

#include <stdlib.h>

#include <QApplication>
#include <QMessageBox>
#include <QPainter>
#include <QTimer>
#include <QSettings>

#include <sy/sycmdswitch.h>

#include "outputpanel.h"

//
// Icons
//
#include "../../icons/drouter-16x16.xpm"


MainWidget::MainWidget(QWidget *parent)
  :QWidget(parent)
{
  //
  // Initialize Variables
  //
  panel_hostname="";
  panel_username="admin";
  panel_password="";
  int router;
  int output;
  bool ok;
  unsigned cols=0;
  int ypos=0;
  bool no_creds=false;
  panel_quantity=0;
  panel_columns=0;
  panel_rows=0;
  QList<unsigned> active_outputs;
  setWindowTitle("OutputPanel");

  //
  // Read Command Options
  //
  SyCmdSwitch *cmd=
    new SyCmdSwitch(qApp->argc(),qApp->argv(),"outputpanel",VERSION,OUTPUTPANEL_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--columns") {
      panel_columns=cmd->value(i).toUInt();
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--hostname") {
      panel_hostname=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--no-creds") {
      no_creds=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--username") {
      panel_username=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--password") {
      panel_password=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--output") {  // Defer processing on these
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      QMessageBox::warning(this,"OutputPanel - "+tr("Error"),
			   tr("Unknown option")+": "+cmd->key(i)+"!");
      exit(256);
    }
  }

  //
  // Get the hostname
  //
  if(panel_hostname.isEmpty()) {
    if(getenv("DROUTER_HOSTNAME")!=NULL) {
      panel_hostname=getenv("DROUTER_HOSTNAME");
    }
    else {
      panel_hostname="localhost";
    }
  }

  //
  // Create And Set Icon
  //
  setWindowIcon(QPixmap(drouter_16x16_xpm));

  //
  // The SA Connection
  //
  panel_parser=new SaParser(this);

  //
  // Dialogs
  //
  panel_login_dialog=new LoginDialog("OutputPanel",this);

  //
  // The Button Clock
  //
  clock_state=false;
  QTimer *timer=new QTimer(this);
  timer->start(PANELWIDGET_FLASH_INTERVAL);
  this->connect(timer,SIGNAL(timeout()),this,SLOT(tickClock()));

  //
  // Start Output Panels
  //
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--output") {
      router=1;
      QStringList f0=cmd->value(i).split(":");
      if(f0.size()>2) {
	QMessageBox::warning(this,"OutputPanel - "+tr("Error"),
			     tr("Invalid output")+" \""+cmd->value(i)+
			     "\" specified!");
	exit(1);
      }
      if(f0.size()==2) {
	router=f0.at(0).toInt(&ok);
	if((!ok)||(router<1)) {
	  QMessageBox::warning(this,"OutputPanel - "+tr("Error"),
			       tr("Invalid router specified!"));
	  exit(1);
	}
      }
      output=f0.back().toInt(&ok)-1;
      if((!ok)||(output<0)) {
	QMessageBox::warning(this,"OutputPanel - "+tr("Error"),
			     tr("Invalid output specified!"));
	exit(1);
      }      
      PanelWidget *widget=
	new PanelWidget(panel_parser,router,output,this);
      connect(panel_parser,SIGNAL(connected(bool,SaParser::ConnectionState)),
	      widget,
	      SLOT(changeConnectionState(bool,SaParser::ConnectionState)));
      connect(panel_parser,SIGNAL(inputListChanged()),
	      widget,SLOT(updateInputNames()));
      connect(panel_parser,SIGNAL(outputListChanged()),
	      widget,SLOT(updateOutputNames()));
      connect(panel_parser,
	      SIGNAL(outputCrosspointChanged(int,int,int)),
	      widget,SLOT(changeOutputCrosspoint(int,int,int)));
      connect(this,SIGNAL(clockTicked(bool)),
	      widget,SLOT(tickClock(bool)));
      if(panel_columns==0) {
	widget->
	  setGeometry(10+panel_quantity*(20+widget->sizeHint().width()),0,
		      widget->sizeHint().width(),
		      widget->sizeHint().height());
      }
      else {
	widget->
	  setGeometry(10+cols*(20+widget->sizeHint().width()),ypos,
		      widget->sizeHint().width(),
		      widget->sizeHint().height());
	cols++;
	if(cols==panel_columns) {
	  cols=0;
	  panel_rows++;
	  ypos+=(PANELWIDGET_HEIGHT+10);
	}
      }
      active_outputs.push_back(output);
      panel_quantity++;
    }
  }
  if(panel_quantity==0) {
    QMessageBox::warning(this,"OutputPanel - "+tr("Error"),
			 tr("No output specified!"));
    exit(256);
  }

  if(cols!=0) {
    panel_rows++;
  }

  //
  // Fix the Window Size
  //
  setMinimumWidth(sizeHint().width());
  setMaximumWidth(sizeHint().width());
  setMinimumHeight(sizeHint().height());
  setMaximumHeight(sizeHint().height());

  //
  // Fire up the SAP connection
  //
  if((!no_creds)&&panel_password.isEmpty()) {
    if(!panel_login_dialog->exec(&panel_username,&panel_password)) {
      exit(1);
    }
  }
  panel_parser->
    connectToHost(panel_hostname,9500,panel_username,panel_password);
}


MainWidget::~MainWidget()
{
}


QSize MainWidget::sizeHint() const
{
  if(panel_columns==0) {
    return QSize((PANELWIDGET_WIDTH+20)*panel_quantity,PANELWIDGET_HEIGHT+10);
  }
  else {
    return QSize((PANELWIDGET_WIDTH+20)*panel_columns,
		 (PANELWIDGET_HEIGHT+10)*panel_rows);
  }
}


void MainWidget::tickClock()
{
  emit clockTicked(clock_state);
  clock_state=!clock_state;
}


QSizePolicy MainWidget::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
}


void MainWidget::paintEvent(QPaintEvent *e)
{
  QPainter *p=new QPainter(this);
  p->setPen(Qt::black);
  p->setBrush(Qt::black);
  if(panel_columns==0) {
    for(unsigned i=1;i<panel_quantity;i++) {
      p->drawLine(i*(PANELWIDGET_WIDTH+20),0,
		  i*(PANELWIDGET_WIDTH+20),sizeHint().height());
    }
  }
  else {
    for(unsigned i=1;i<panel_columns;i++) {
      p->drawLine(i*(PANELWIDGET_WIDTH+20),0,
		  i*(PANELWIDGET_WIDTH+20),sizeHint().height());
    }
    for(unsigned i=1;i<panel_rows;i++) {
      p->drawLine(0,i*(PANELWIDGET_HEIGHT+10)-1,
		  sizeHint().width(),i*(PANELWIDGET_HEIGHT+10)-1);
    }
  }
  delete p;
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);
  MainWidget *w=new MainWidget(NULL);
  w->setGeometry(w->geometry().x(),w->geometry().y(),w->sizeHint().width(),w->sizeHint().height());
  w->show();
  return a.exec();
}
