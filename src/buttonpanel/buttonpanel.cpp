// buttonpanel.cpp
//
// Button applet for controlling an lwpath output.
//
//   (C) Copyright 2002-2020 Fred Gleason <fredg@paravelsystems.com>
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

#include <QApplication>
#include <QMessageBox>
#include <QPainter>

#include <sy/sycmdswitch.h>
#include <sy/symcastsocket.h>

#include "buttonpanel.h"

//
// Icons
//
#include "../../icons/drouter-16x16.xpm"

MainWidget::MainWidget(QWidget *parent)
  : QWidget(parent)
{
  panel_columns=0;
  panel_hostname="";
  panel_username="Admin";
  panel_arm_button=false;

  bool no_creds=false;
  bool ok=false;

  setWindowTitle(QString("ButtonPanel v")+VERSION);
  setWindowIcon(QPixmap(drouter_16x16_xpm));

  //
  // Read Command Options
  //
  SyCmdSwitch *cmd=
    new SyCmdSwitch(qApp->argc(),qApp->argv(),"buttonpanel",VERSION,
				   BUTTONPANEL_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--arm-button") {
      panel_arm_button=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--columns") {
      panel_columns=cmd->value(i).toUInt(&ok);
      if(!ok) {
	QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),
			     tr("Invalid --columns value specified!"));
	exit(1);
      }
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
      panel_routers.push_back(1);
      QStringList f0=cmd->value(i).split(":");
      if(f0.size()>2) {
	QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),
			     tr("Invalid output")+" \""+cmd->value(i)+
			     "\" specified!");
	exit(1);
      }
      if(f0.size()==2) {
	panel_routers.back()=f0.at(0).toInt(&ok);
	if((!ok)||(panel_routers.back()<1)) {
	  QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),
			       tr("Invalid router specified!"));
	  exit(1);
	}
      }
      panel_outputs.push_back(f0.back().toInt(&ok));
      if((!ok)||(panel_outputs.back()<0)) {
	QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),
			     tr("Invalid output specified!"));
	exit(1);
      }      
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      QMessageBox::warning(this,"Buttonpanel - "+tr("Error"),
			   tr("Unknown option")+": "+cmd->key(i)+"!");
      exit(256);
    }
  }

  //
  // Sanity Checks
  //
  if(panel_outputs.size()==0) {
    QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),
			 tr("No output specified."));
    exit(1);
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
  // The SA Connection
  //
  panel_parser=new SaParser(this);
  panel_vlayout=new QVBoxLayout();
  for(int i=0;i<panel_routers.size();i++) {
    panel_vlayout->addWidget(new ButtonWidget(panel_routers.at(i),
					      panel_outputs.at(i),
					      panel_columns,panel_parser,
					      panel_arm_button));
  }
  panel_canvas=new QWidget(this);
  panel_canvas->setLayout(panel_vlayout);
  panel_canvas->hide();
  connect(panel_parser,SIGNAL(connected(bool,SaParser::ConnectionState)),
	  this,SLOT(changeConnectionState(bool,SaParser::ConnectionState)));
  panel_resize_timer=new QTimer(this);
  panel_resize_timer->setSingleShot(true);
  connect(panel_resize_timer,SIGNAL(timeout()),this,SLOT(resizeData()));

  //
  // Dialogs
  //
  panel_login_dialog=new LoginDialog("ButtonPanel",this);

  //
  // Connecting Label
  //
  panel_connecting_label=new QLabel(tr("Connecting..."),this);
  panel_connecting_label->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);
  panel_connecting_label->
    setFont(QFont(font().family(),font().pixelSize(),QFont::Bold));

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
  if(panel_vlayout->sizeHint().width()<BUTTONWIDGET_CELL_WIDTH) {
    return QSize(200,30);
  }
  return panel_vlayout->sizeHint();
}


void MainWidget::changeConnectionState(bool state,
				       SaParser::ConnectionState cstate)
{
  //  printf("changeConnectionState(%d)\n",state);
  if(state) {
    panel_resize_timer->start(0);  // So the widgets can create buttons first
    panel_canvas->show();
    panel_connecting_label->hide();
  }
  else {
    panel_connecting_label->setText(tr("Reconnecting..."));
    panel_connecting_label->
      setFont(QFont(font().family(),36,QFont::Bold));
    panel_canvas->hide();
    panel_connecting_label->show();
  }
}


void MainWidget::resizeData()
{
  setMinimumSize(sizeHint());
  setMaximumSize(sizeHint());
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  panel_canvas->setGeometry(0,0,size().width(),size().height());
  panel_connecting_label->setGeometry(0,0,size().width(),size().height());
}


void MainWidget::paintEvent(QPaintEvent *e)
{
  if(panel_parser->isConnected()) {
    QPainter *p=new QPainter(this);

    p->setPen(Qt::black);
    p->setBrush(Qt::black);

    for(int i=0;i<panel_vlayout->count()-1;i++) {
      p->drawLine(0,
		  (i+1)*(5+panel_vlayout->itemAt(i)->sizeHint().height())+1,
		  size().width(),
		  (i+1)*(5+panel_vlayout->itemAt(i)->sizeHint().height())+1);
    }

    delete p;
  }
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);

  MainWidget *w=new MainWidget();
  w->show();
  return a.exec();
}
