// buttonpanel.cpp
//
// Button applet for controlling an lwpath output.
//
//   (C) Copyright 2002-2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <sy5/sycmdswitch.h>
#include <sy5/symcastsocket.h>

#include "buttonpanel.h"
#include "gpiowidget.h"

//
// Icons
//
#include "../../icons/drouter-16x16.xpm"

MainWidget::MainWidget(QWidget *parent)
  : QWidget(parent)
{
  panel_columns=0;
  panel_hostname="";
  panel_username="buttonpanel";
  panel_arm_button=false;
  panel_no_max_size=false;

  bool prompt=false;
  bool ok=false;
  QString err_msg;

  setWindowTitle(QString("Drouter - ButtonPanel [")+VERSION+"]");
  setWindowIcon(QPixmap(drouter_16x16_xpm));

  //
  // Read Command Options
  //
  QStringList colornames;
  colornames.push_back("black");
  colornames.push_back("blue");
  colornames.push_back("cyan");
  colornames.push_back("green");
  colornames.push_back("magenta");
  colornames.push_back("red");
  colornames.push_back("yellow");

  SyCmdSwitch *cmd=
    new SyCmdSwitch("buttonpanel",VERSION,BUTTONPANEL_USAGE);
  for(int i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--arm-button") {
      panel_arm_button=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--columns") {
      panel_columns=cmd->value(i).toUInt(&ok);
      if(!ok) {
	processError(tr("Invalid --columns value specified!"));
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--hostname") {
      panel_hostname=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--no-creds") {
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--no-max-size") {
      panel_no_max_size=true;
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
    if(cmd->key(i)=="--gpio") {
      panel_arg_types.push_back(DREndPointMap::GpioRouter);
      GpioParser *parser=GpioParser::fromString(cmd->value(i),&err_msg);
      if(parser==NULL) {
	processError(err_msg);
      }
      panel_gpio_parsers.push_back(parser);
      cmd->setProcessed(i,true);
    }

    if(cmd->key(i)=="--output") {
      panel_arg_types.push_back(DREndPointMap::AudioRouter);
      panel_arg_audio_routers.push_back(1);
      QStringList f0=cmd->value(i).split(":");
      if(f0.size()>2) {
	processError(tr("Invalid output")+" \""+cmd->value(i)+"\" specified!");
      }
      if(f0.size()==2) {
	panel_arg_audio_routers.back()=f0.at(0).toInt(&ok);
	if((!ok)||(panel_arg_audio_routers.back()<1)) {
	  processError(tr("Invalid router specified!"));
	}
      }
      panel_arg_audio_outputs.push_back(f0.back().toInt(&ok));
      if((!ok)||(panel_arg_audio_outputs.back()<0)) {
	processError(tr("Invalid output specified!"));
      }      
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      processError(tr("Unknown option")+": "+cmd->key(i)+"!");
    }
  }

  //
  // Sanity Checks
  //
  if(panel_arg_types.size()==0) {
    processError(tr("At least one --output or --gpio argment must be specified."));
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
  // The Protocol J Connection
  //
  panel_parser=new DRJParser(false,this);
  int audionum=0;
  int gpionum=0;
  for(int i=0;i<panel_arg_types.size();i++) {
    if(panel_arg_types[i]==DREndPointMap::AudioRouter) {
      panel_widgets.
	push_back(new ButtonWidget(panel_arg_audio_routers.at(audionum),
				   panel_arg_audio_outputs.at(audionum),
				   panel_columns,panel_parser,
				   panel_arm_button,this));
      audionum++;
    }
    if(panel_arg_types[i]==DREndPointMap::GpioRouter) {
      GpioWidget *w=NULL;
      w=new GpioWidget(panel_gpio_parsers.at(gpionum),panel_parser,this);
      panel_widgets.push_back(w);
      gpionum++;
    }
  }
  connect(panel_parser,SIGNAL(connected(bool,DRJParser::ConnectionState)),
	  this,SLOT(changeConnectionState(bool,DRJParser::ConnectionState)));
  connect(panel_parser,SIGNAL(parserError(DRJParser::ErrorType,const QString &)),
	  this,SLOT(parserErrorData(DRJParser::ErrorType,const QString &)));

  panel_resize_timer=new QTimer(this);
  panel_resize_timer->setSingleShot(true);
  connect(panel_resize_timer,SIGNAL(timeout()),this,SLOT(resizeData()));

  //
  // Dialogs
  //
  panel_login_dialog=new DRLoginDialog("ButtonPanel",this);

  //
  // Connecting Label
  //
  panel_connecting_label=new QLabel(tr("Connecting..."),this);
  panel_connecting_label->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);
  panel_connecting_label->
    setFont(QFont(font().family(),font().pixelSize(),QFont::Bold));

  //
  // Fire up the Protocol J connection
  //
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
  if((panel_parser!=NULL)&&panel_parser->isConnected()) {
    int width=0;
    int height=0;

    for(int i=0;i<panel_widgets.size();i++) {
      QWidget *w=panel_widgets.at(i);
      if(w->sizeHint().width()>width) {
	width=w->sizeHint().width();
      }
      height+=10+w->sizeHint().height();
    }
    width+=10;
    height-=10;

    return QSize(width,height);
  }
  return QSize(200,22);
}


void MainWidget::processError(const QString err_msg)
{
  QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),err_msg);
  exit(1);
}


void MainWidget::parserErrorData(DRJParser::ErrorType err,const QString &remarks)
{
  QString str=DRJParser::errorString(err);
  if(!remarks.isEmpty()) {
    str+="\n\n"+remarks;
  }
  QMessageBox::warning(this,"Buttonpanel - "+tr("JSON Parser Error"),str);
}


void MainWidget::changeConnectionState(bool state,
				       DRJParser::ConnectionState cstate)
{
  if(state) {
    panel_resize_timer->start(0);  // So the widgets can create buttons first
    panel_connecting_label->hide();
    for(int i=0;i<panel_widgets.size();i++) {
      panel_widgets.at(i)->show();
    }
  }
  else {
    panel_connecting_label->setText(tr("Reconnecting..."));
    panel_connecting_label->
      setFont(QFont(font().family(),36,QFont::Bold));
    for(int i=0;i<panel_widgets.size();i++) {
      panel_widgets.at(i)->hide();
    }
    panel_connecting_label->show();
  }
}


void MainWidget::resizeData()
{
  setMinimumSize(sizeHint());
  if(!panel_no_max_size) {
    setMaximumSize(sizeHint());
  }
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  int ypos=0;

  for(int i=0;i<panel_widgets.size();i++) {
    QWidget *w=panel_widgets.at(i);
    w->setGeometry(5,ypos,size().width()-5,w->sizeHint().height());
    ypos+=10+w->sizeHint().height();
  }
  panel_connecting_label->setGeometry(0,0,size().width(),size().height());
}


void MainWidget::paintEvent(QPaintEvent *e)
{
  int ypos=0;

  if(panel_parser->isConnected()) {
    QPainter *p=new QPainter(this);

    p->setPen(Qt::black);
    p->setBrush(Qt::black);

    for(int i=0;i<panel_widgets.size()-1;i++) {
      QWidget *w=panel_widgets.at(i);
      ypos+=10+w->sizeHint().height();
      p->drawLine(0,ypos-5,size().width(),ypos-5);
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
