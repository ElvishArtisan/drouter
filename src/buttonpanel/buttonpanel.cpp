// buttonpanel.cpp
//
// Button applet for controlling a Drouter output.
//
//   (C) Copyright 2002-2025 Fred Gleason <fredg@paravelsystems.com>
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
#include <QFile>
#include <QMessageBox>
#include <QPainter>

#include <sy6/sycmdswitch.h>
#include <sy6/symcastsocket.h>

#include "drouter/paths.h"

#include "buttonpanel.h"
#include "gpiostrip.h"

//
// Icons
//
#include "../../icons/drouter-16x16.xpm"

MainWidget::MainWidget(QWidget *parent)
  : QWidget(parent)
{
  panel_columns=0;
  panel_hostname="";
  panel_arm_button=false;
  panel_no_max_size=false;
  panel_summary_alarm_state=false;

  bool list_sound_devices=false;
  int sound_device=-1;
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
    if(cmd->key(i)=="--list-sound-devices") {
      list_sound_devices=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--no-creds") {  // Backwards compatibility
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--no-max-size") {
      panel_no_max_size=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--prompt") {  // Backwards compatibility
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--username") {  // Backwards compatibility
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--password") {  // Backwards compatibility
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--sound-device") {
      sound_device=cmd->value(i).toInt(&ok);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--sound-test-file") {
      panel_sound_test_file=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--sound-alert-file") {
      panel_sound_alert_file=cmd->value(i);
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
  if((panel_arg_types.size()==0)&&(panel_sound_test_file.isEmpty())&&
     (!list_sound_devices)) {
    processError(tr("At least one --output or --gpio argment must be specified."));
  }
  //
  // Verify The Sound File
  //
  QStringList paths;
  paths.push_back("/etc/drouter/sounds");
  paths.push_back(QString(PATH_DATA)+"/sounds");
  if(!panel_sound_alert_file.isEmpty()) {
    bool found=false;
    for(int i=0;i<paths.size();i++) {
      if(QFile::exists(paths.at(i)+"/"+panel_sound_alert_file)) {
	found=true;
      }
    }
    if(!found) {
      QMessageBox::warning(this,"ButtonPanel - "+tr("Warning"),
			   tr("Audio file")+" \""+panel_sound_alert_file+"\" "+
			   tr("not found."));
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
  // Audio Player
  //
  int pa_err=0;
  if((pa_err=Pa_Initialize())!=paNoError) {
    QMessageBox::critical(this,"Drouter - ButtonPanel - "+tr("Error"),
			  tr("PortAudio Error")+":"+Pa_GetErrorText(pa_err));
    exit(1);
  }
  if(sound_device<0) {
    sound_device=Pa_GetDefaultOutputDevice();
  }
  if(sound_device>=Pa_GetDeviceCount()) {
    processError(tr("No such sound device."));
  }
  if(list_sound_devices) {
    QString defcol("  ");
    QString devlist;
    for(int i=0;i<Pa_GetDeviceCount();i++) {
      if(i==Pa_GetDefaultOutputDevice()) {
	defcol="* ";
      }
      devlist+=QString::asprintf("%s%2d: %s\n",defcol.toUtf8().constData(),
				 i,Pa_GetDeviceInfo(i)->name);
    }
    QMessageBox::information(this,"Drouter - ButtonPanel - "+
			     tr("Available Audio Devices"),devlist);
    Pa_Terminate();
    exit(0);
  }
  panel_sound_player=new SoundPlayer(sound_device,paths,this);
  connect(panel_sound_player,SIGNAL(started()),this,SLOT(playerStartedData()));
  connect(panel_sound_player,SIGNAL(stopped()),this,SLOT(playerStoppedData()));
  
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
      GpioStrip *w=NULL;
      w=new GpioStrip(panel_widgets.size(),panel_gpio_parsers.at(gpionum),
		      panel_parser,this);
      connect(w,SIGNAL(summaryAlarmStateChanged(int,bool,bool)),
	      this,SLOT(summaryAlarmStateChangedData(int,bool,bool)));
      connect(w,SIGNAL(acknowledgeRequested()),this,SLOT(acknowledgedData()));
      panel_widgets.push_back(w);
      panel_alarm_states.push_back(false);
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

  if(panel_sound_test_file.isEmpty()) {
    //
    // Fire up the Protocol J connection
    //
    panel_parser->connectToHost(panel_hostname,9600);
  }
  else {
    if(!panel_sound_player->play(panel_sound_test_file,false,&err_msg)) {
      QMessageBox::warning(this,"Drouter - ButtonPanel - "+tr("Error"),
			   tr("Audio play-out failed!")+"\n"+
			   "["+err_msg+"]");
      Pa_Terminate();
      exit(1);
    }
  }
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


void MainWidget::summaryAlarmStateChangedData(int id,bool state,bool new_alarm)
{
  //  printf("summaryAlarmStateChangedData(%d,%d)\n",id,state);
  QString err_msg;

  panel_alarm_states[id]=state;

  if(state) {  // New Alarm
    panel_summary_alarm_state=true;
    if(new_alarm&&(!panel_sound_player->isPlaying())&&
       (!panel_sound_alert_file.isEmpty())) {
      if(!panel_sound_player->play(panel_sound_alert_file,true,&err_msg)) {
	fprintf(stderr,"audio error: %s\n",err_msg.toUtf8().constData());
      }
    }
  }
  else {
    for(int i=0;i<panel_alarm_states.size();i++) {
      if(panel_alarm_states.at(i)) {
	panel_summary_alarm_state=true;
	return;
      }
    }
    panel_summary_alarm_state=false;
    if(panel_sound_player->isPlaying()) {
      panel_sound_player->stop();
    }
  }
}


void MainWidget::acknowledgedData()
{
  //  printf("acknowledgedData()\n");
  if(panel_sound_player->isPlaying()) {
    panel_sound_player->stop();
  }
}


void MainWidget::playerStartedData()
{
}


void MainWidget::playerStoppedData()
{
  if(!panel_sound_test_file.isEmpty()) {
    Pa_Terminate();
    exit(0);
  }
}


void MainWidget::processError(const QString err_msg)
{
  QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),err_msg);
  Pa_Terminate();
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
  //  printf("MainWidget::changeConnectionState(%d,%d)\n",
  //	 state,cstate);

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


void MainWidget::closeEvent(QCloseEvent *e)
{
  Pa_Terminate();
  exit(0);
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);

  MainWidget *w=new MainWidget();
  w->show();
  return a.exec();
}
