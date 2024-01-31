// shotpanel.cpp
//
// An applet for activating a snapshot
//
//   (C) Copyright 2017-2022 Fred Gleason <fredg@paravelsystems.com>
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
#include <QIcon>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QSettings>

#include <sy5/sycmdswitch.h>
#include <sy5/symcastsocket.h>

#include "shotpanel.h"

//
// Icons
//
#include "../../icons/drouter-16x16.xpm"


MainWidget::MainWidget(QWidget *parent)
  :QWidget(parent)
{
  QString config_filename;
  bool prompt=false;
  bool ok=false;

  panel_initial_connected=false;
  panel_initial_router=-1;

  //
  // Initialize Variables
  //
  panel_hostname="";
  panel_username="shotpanel";
  panel_password="";

  //
  // Read Command Options
  //
  SyCmdSwitch *cmd=new SyCmdSwitch("shotpanel",VERSION,SHOTPANEL_USAGE);
  for(int i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--hostname") {
      panel_hostname=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--initial-router") {
      panel_initial_router=cmd->value(i).toInt(&ok);
      if(!ok) {
	QMessageBox::warning(this,"ShotPanel - "+tr("Error"),
			     tr("Invalid --initial-router value")+
			     " \""+cmd->value(i)+"\".");
	exit(1);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--no-creds") {
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--prompt") {
      prompt=true;
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
    if(!cmd->processed(i)) {
      QMessageBox::warning(this,"ShotPanel - "+tr("Error"),
			   tr("Unknown argument")+" \""+cmd->key(i)+"\".");
      exit(1);
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
  // Fonts
  //
  QFont label_font("helvetica",12,QFont::Bold);
  label_font.setPixelSize(12);
  QFont button_font("helvetica",14,QFont::Bold);
  button_font.setPixelSize(14);

  //
  // Dialogs
  //
  panel_login_dialog=new DRLoginDialog("ShotPanel",this);

  //
  // Router Control
  //
  panel_router_label=new QLabel(tr("Router")+":",this);
  panel_router_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  panel_router_label->setFont(button_font);
  panel_router_box=new QComboBox(this);
  connect(panel_router_box,SIGNAL(activated(int)),
	  this,SLOT(routerBoxActivatedData(int)));

  //
  // Snapshot Control
  //
  panel_snapshot_label=new QLabel(tr("Snapshot"),this);
  panel_snapshot_label->setFont(button_font);
  panel_snapshot_label->setDisabled(true);
  panel_snapshot_box=new QComboBox(this);
  panel_snapshot_box->setDisabled(true);

  //
  // Activate Button
  //
  panel_activate_button=new QPushButton(tr("Activate"),this);
  panel_activate_button->setFont(button_font);
  connect(panel_activate_button,SIGNAL(clicked()),this,SLOT(activateData()));

  //
  // Fix the Window Size
  //
  setMinimumWidth(sizeHint().width());
  setMaximumWidth(sizeHint().width());
  setMinimumHeight(sizeHint().height());
  setMaximumHeight(sizeHint().height());

  //
  // The SA Connection
  //
  panel_parser=new DRJParser(this);
  connect(panel_parser,SIGNAL(connected(bool,DRJParser::ConnectionState)),
	  this,SLOT(connectedData(bool,DRJParser::ConnectionState)));
  connect(panel_parser,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));

  setWindowTitle(QString("Drouter - ShotPanel [")+VERSION+"]");

  if(prompt) {
    if(!panel_login_dialog->exec(&panel_username,&panel_password)) {
      exit(1);
    }
  }
  panel_parser->
    connectToHost(panel_hostname,9600,panel_username,panel_password);
}


MainWidget::~MainWidget()
{
}


QSize MainWidget::sizeHint() const
{
  return QSize(400,90);
}


QSizePolicy MainWidget::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
}


void MainWidget::routerBoxActivatedData(int n)
{
  int router=SelectedRouter();
  DRSnapshotListModel *smodel=panel_parser->snapshotModel(router);
  panel_snapshot_box->setModel(smodel);
  panel_snapshot_label->setEnabled(smodel->rowCount()>0);
  panel_snapshot_box->setEnabled(smodel->rowCount()>0);
  panel_activate_button->setEnabled(smodel->rowCount()>0);
}

void MainWidget::activateData()
{
  int router=SelectedRouter();
  panel_parser->activateSnapshot(router,panel_snapshot_box->currentText());
}

void MainWidget::connectedData(bool state,DRJParser::ConnectionState cstate)
{
  if(state) {
    panel_router_box->setModel(panel_parser->routerModel());
    routerBoxActivatedData(panel_router_box->currentIndex());
    panel_initial_connected=true;
  }
  else {
    if(cstate!=DRJParser::WatchdogActive) {
      QMessageBox::warning(this,"ShotPanel - "+tr("Error"),
			   tr("Login error")+": "+
			   DRJParser::connectionStateString(cstate));
      exit(1);
    }
  }
  panel_router_label->setEnabled(state);
  panel_router_box->setEnabled(state);
  panel_snapshot_label->setEnabled(state);
  panel_snapshot_box->setEnabled(state);
}


void MainWidget::errorData(QAbstractSocket::SocketError err)
{
  if(!panel_initial_connected) {
    QMessageBox::warning(this,"ShotPanel - "+tr("Error"),
			 tr("Network Error")+": "+
			 SyMcastSocket::socketErrorText(err));
    exit(1);
  }
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  panel_router_label->setGeometry(15,8,60,20);
  panel_router_box->setGeometry(80,8,200,20);

  panel_snapshot_label->setGeometry(15,38,100,20);
  panel_snapshot_box->setGeometry(10,60,size().width()-20,20);

  panel_activate_button->setGeometry(size().width()-90,10,80,40);
}


int MainWidget::SelectedRouter() const
{
  return panel_parser->routerModel()->
    routerNumber(panel_router_box->currentIndex());
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);

  //
  // Start Event Loop
  //
  MainWidget *w=new MainWidget(NULL);
  w->setGeometry(w->geometry().x(),w->geometry().y(),w->sizeHint().width(),w->sizeHint().height());
  w->show();
  return a.exec();
}
