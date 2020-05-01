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
	processError(tr("Invalid --columns value specified!"));
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
    if(cmd->key(i)=="--gpio") {
      panel_arg_types.push_back(EndPointMap::GpioRouter);
      QStringList types;
      QList<QChar> dirs;
      QList<int> routers;
      QList<int> endpts;
      QStringList masks;
      QStringList f0=cmd->value(i).split("/",QString::SkipEmptyParts);
      for(int j=0;j<f0.size();j++) {
	QStringList f1=f0.at(j).split(":",QString::SkipEmptyParts);
	if(f1.size()!=5) {
	  processError(tr("Invalid --gpio argument")+" \""+f0.at(j)+"\".");
	}
	types.push_back(f1.at(0).toLower());

	if((f1.at(1).length()!=1)||
	   ((f1.at(1).toLower().at(0)!=QChar('i'))&&
	    (f1.at(1).toLower().at(0)!=QChar('o')))) {
	  processError(tr("Invalid --gpio argument direction")+
		       " \""+f1.at(1)+"\".");
	}
	dirs.push_back(f1.at(1).toLower().at(0));

	unsigned router=f1.at(2).toUInt(&ok);
	if(!ok) {
	  processError(tr("Invalid --gpio argument router")+
		       " \""+f1.at(2)+"\".");
	}
	routers.push_back(router);

	unsigned endpt=f1.at(3).toUInt(&ok);
	if(!ok) {
	  processError(tr("Invalid --gpio argument endpt")+
		       " \""+f1.at(3)+"\".");
	}
	endpts.push_back(endpt);
	
	QString mask=f1.at(4).toLower();
	if(mask.length()!=5) {
	  processError(tr("Invalid --gpio argument mask")+" \""+f1.at(4)+"\".");
	}
	masks.push_back(mask);
      }
      panel_arg_gpio_types.push_back(types);
      panel_arg_gpio_dirs.push_back(dirs);
      panel_arg_gpio_routers.push_back(routers);
      panel_arg_gpio_endpts.push_back(endpts);
      panel_arg_gpio_masks.push_back(masks);
      cmd->setProcessed(i,true);
    }

    if(cmd->key(i)=="--output") {
      panel_arg_types.push_back(EndPointMap::AudioRouter);
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
  // The SA Connection
  //
  panel_parser=new SaParser(this);
  int audionum=0;
  int gpionum=0;
  for(int i=0;i<panel_arg_types.size();i++) {
    if(panel_arg_types[i]==EndPointMap::AudioRouter) {
      panel_widgets.
	push_back(new ButtonWidget(panel_arg_audio_routers.at(audionum),
				   panel_arg_audio_outputs.at(audionum),
				   panel_columns,panel_parser,
				   panel_arm_button,this));
      audionum++;
    }
    if(panel_arg_types[i]==EndPointMap::GpioRouter) {
      GpioWidget *w=NULL;
      w=new GpioWidget(panel_arg_gpio_types.at(gpionum),
		       panel_arg_gpio_dirs.at(gpionum),
		       panel_arg_gpio_routers.at(gpionum),
		       panel_arg_gpio_endpts.at(gpionum),
		       panel_arg_gpio_masks.at(gpionum),
		       panel_parser,this);
      panel_widgets.push_back(w);
      gpionum++;
    }
  }
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
  QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),err_msg);;
  exit(1);
}


void MainWidget::changeConnectionState(bool state,
				       SaParser::ConnectionState cstate)
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
  QSize sz=sizeHint();

  //  setGeometry(geometry().x(),geometry().y(),sz.width(),sz.height());
  setMinimumSize(sizeHint());
  setMaximumSize(sizeHint());
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  int ypos=0;

  for(int i=0;i<panel_widgets.size();i++) {
    QWidget *w=panel_widgets.at(i);
    w->setGeometry(5,ypos,w->sizeHint().width(),w->sizeHint().height());
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
