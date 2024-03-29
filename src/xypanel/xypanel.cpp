// xypanel.cpp
//
// An applet for controling an LWPath output
//
//   (C) Copyright 2002-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include "xypanel.h"

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

  panel_clock_state=false;
  panel_current_input=0;
  panel_initial_connected=false;
  panel_initial_router=-1;

  //
  // Initialize Variables
  //
  panel_hostname="";
  panel_username="xypanel";
  panel_password="";

  //
  // Read Command Options
  //
  SyCmdSwitch *cmd=new SyCmdSwitch("xypanel",VERSION,XYPANEL_USAGE);
  for(int i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--hostname") {
      panel_hostname=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--initial-router") {
      panel_initial_router=cmd->value(i).toInt(&ok);
      if(!ok) {
	QMessageBox::warning(this,"XYPanel - "+tr("Error"),
			     tr("Invalid --initial-router value")+
			     " \""+cmd->value(i)+"\".");
	exit(1);
      }
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
    if(cmd->key(i)=="--no-creds") {
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--prompt") {
      prompt=true;
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      QMessageBox::warning(this,"XYPanel - "+tr("Error"),
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
  // Clock Timer
  //
  panel_clock_timer=new QTimer(this);
  panel_clock_timer->setSingleShot(false);
  connect(panel_clock_timer,SIGNAL(timeout()),this,SLOT(clockData()));

  //
  // Dialogs
  //
  panel_login_dialog=new LoginDialog("XYPanel",this);

  //
  // Router Control
  //
  panel_router_label=new QLabel(tr("Router")+":",this);
  panel_router_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  panel_router_label->setFont(button_font);
  panel_router_label->setDisabled(true);
  panel_router_box=new ComboBox(this);
  panel_router_box->setDisabled(true);
  connect(panel_router_box,SIGNAL(activated(int)),
	  this,SLOT(routerBoxActivatedData(int)));

  //
  // Output Control
  //
  panel_output_label=new QLabel(tr("Output (Destination)"),this);
  panel_output_label->setFont(button_font);
  panel_output_label->setDisabled(true);
  panel_output_box=new ComboBox(this);
  panel_output_box->setDisabled(true);
  connect(panel_output_box,SIGNAL(activated(int)),
	  this,SLOT(outputBoxActivatedData(int)));

  //
  // Input Control
  //
  panel_input_label=new QLabel(tr("Input (Source)"),this);
  panel_input_label->setFont(button_font);
  panel_input_label->setDisabled(true);
  panel_input_box=new ComboBox(this);
  panel_input_box->setDisabled(true);
  connect(panel_input_box,SIGNAL(activated(int)),
	  this,SLOT(inputBoxActivatedData(int)));

  //
  // Take Button
  //
  panel_take_button=new QPushButton(tr("Take"),this);
  panel_take_button->setFont(button_font);
  connect(panel_take_button,SIGNAL(clicked()),this,SLOT(takeData()));

  //
  // Cancel Button
  //
  panel_cancel_button=new QPushButton(tr("Cancel"),this);
  panel_cancel_button->setFont(button_font);
  connect(panel_cancel_button,SIGNAL(clicked()),this,SLOT(cancelData()));

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
  panel_parser=new SaParser(this);
  connect(panel_parser,SIGNAL(connected(bool,SaParser::ConnectionState)),
	  this,SLOT(connectedData(bool,SaParser::ConnectionState)));
  connect(panel_parser,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
  connect(panel_parser,SIGNAL(outputCrosspointChanged(int,int,int)),
	  this,SLOT(outputCrosspointChangedData(int,int,int)));

  setWindowTitle(QString("Drouter - XYPanel [")+VERSION+"]");

  if(prompt) {
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
  return QSize(800,90);
}


QSizePolicy MainWidget::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
}


void MainWidget::routerBoxActivatedData(int n)
{
  QString name;
  int count;
  int endpt;
  int router=panel_router_box->itemData(n).toInt();

  panel_output_box->clear();
  count=0;
  endpt=0;
  while(count<panel_parser->outputQuantity(router)) {
    name=panel_parser->outputLongName(router,endpt+1);
    if(!name.isEmpty()) {
      panel_output_box->
	insertItem(endpt,QString::asprintf("%u - ",endpt+1)+name,endpt+1);
      count++;
    }
    endpt++;
  }
  panel_input_box->clear();
  endpt=0;
  for(int i=0;i<panel_parser->inputQuantity(router);i++) {
    if(panel_parser->inputIsReal(router,i+1)) {
      name=panel_parser->inputLongName(router,i+1);
      panel_input_box->
	insertItem(panel_input_box->count(),QString::asprintf("%u - ",i+1)+name,i+1);
      count++;
    }
  }
  panel_input_box->
    insertItem(panel_parser->inputQuantity(router),tr("--- OFF ---"),0);
  int output=panel_output_box->currentItemData().toInt();
  outputCrosspointChangedData(router,output,
			      panel_parser->outputCrosspoint(router,output));
  SetArmedState(false);
}


void MainWidget::outputBoxActivatedData(int n)
{
  int router=panel_router_box->currentItemData().toInt();
  int output=panel_output_box->currentItemData().toInt();

  outputCrosspointChangedData(router,output,
			      panel_parser->outputCrosspoint(router,output));
}


void MainWidget::inputBoxActivatedData(int n)
{
  int current=panel_input_box->currentItemData().toInt();

  if(current>=0) {
    SetArmedState(panel_current_input!=(current));
  }
}


void MainWidget::takeData()
{
  int router=panel_router_box->currentItemData().toInt();
  panel_parser->
    setOutputCrosspoint(router,
			panel_output_box->currentItemData().toInt(),
			panel_input_box->currentItemData().toInt());
}


void MainWidget::cancelData()
{
  panel_input_box->setCurrentItemData(panel_current_input);
  SetArmedState(false);
}


void MainWidget::connectedData(bool state,SaParser::ConnectionState cstate)
{
  if(state) {
    QMap<int,QString> routers=panel_parser->routers();
    panel_router_box->clear();
    for(QMap<int,QString>::const_iterator it=routers.begin();it!=routers.end();
	it++) {
      panel_router_box->
	insertItem(panel_router_box->count(),
		   QString::asprintf("%d - ",it.key())+it.value(),it.key());
      if(it.key()==panel_initial_router) {
	panel_router_box->setCurrentIndex(panel_router_box->count()-1);
      }
    }
    if(panel_initial_router<0) {
      panel_router_box->setCurrentIndex(0);
    }
    routerBoxActivatedData(panel_router_box->currentIndex());
    panel_initial_connected=true;
  }
  else {
    if(cstate!=SaParser::WatchdogActive) {
      QMessageBox::warning(this,"XYPanel - "+tr("Error"),
			   tr("Login error")+": "+
			   SaParser::connectionStateString(cstate));
      exit(1);
    }
    panel_router_label->setDisabled(true);
    panel_router_box->setDisabled(true);
    panel_output_label->setDisabled(true);
    panel_output_box->setDisabled(true);
    panel_input_label->setDisabled(true);
    panel_input_box->setDisabled(true);
  }
}


void MainWidget::errorData(QAbstractSocket::SocketError err)
{
  if(!panel_initial_connected) {
    QMessageBox::warning(this,"XYPanel - "+tr("Error"),
			 tr("Network Error")+": "+
			 SyMcastSocket::socketErrorText(err));
    exit(1);
  }
}


void MainWidget::outputCrosspointChangedData(int router,int output,int input)
{
  if(router==panel_router_box->currentItemData().toInt()) {
    if(output==panel_output_box->currentItemData()) {
      if(!panel_input_box->setCurrentItemData(input)) {
	panel_input_box->
	  insertItem(panel_input_box->count(),tr("** UNKNOWN **"),-3);
	panel_input_box->setCurrentItemData(-3);
      }
      panel_current_input=input;
      SetArmedState(false);
      panel_router_label->setEnabled(true);
      panel_router_box->setEnabled(true);
      panel_output_label->setEnabled(true);
      panel_output_box->setEnabled(true);
      panel_input_label->setEnabled(true);
      panel_input_box->setEnabled(true);
    }
    if(input>=0) {
      for(int i=0;i<panel_input_box->count();i++) {
	if(panel_input_box->itemData(i).toInt()<0) {
	  panel_input_box->removeItem(i);
	}
      }
    }
  }
}


void MainWidget::clockData()
{
  if(panel_current_input!=panel_input_box->currentItemData()) {
    if(panel_clock_state) {
      panel_take_button->
	setStyleSheet("color: #FFFFFF;background-color: #FF0000;");
      panel_cancel_button->setStyleSheet("");
    }
    else {
      panel_take_button->setStyleSheet("");
      panel_cancel_button->
	setStyleSheet("color: #FFFFFF;background-color: #0000FF;");
    }
    panel_clock_state=!panel_clock_state;
  }
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  panel_router_label->setGeometry(15,8,60,20);
  panel_router_box->setGeometry(80,8,200,20);

  panel_output_label->setGeometry(15,38,size().width()/2-25,20);
  panel_output_box->setGeometry(10,60,size().width()/2-15,20);

  panel_input_label->setGeometry(size().width()/2+10,38,size().width()/4,20);
  panel_input_box->setGeometry(size().width()/2+5,60,size().width()/2-15,20);

  panel_take_button->
    setGeometry(6*size().width()/8+20,10,size().width()/8-30,40);
  panel_cancel_button->
    setGeometry(7*size().width()/8+20,10,size().width()/8-30,40);
}


void MainWidget::SetArmedState(bool state)
{
  panel_take_button->setEnabled(state);
  panel_cancel_button->setEnabled(state);
  if(state) {
    panel_clock_timer->start(300);
  }
  else {
    panel_clock_timer->stop();
    panel_take_button->setStyleSheet("");
    panel_cancel_button->setStyleSheet("");
  }
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
