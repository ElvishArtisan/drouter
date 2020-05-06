// xpointpanel.cpp
//
// Full graphical crosspoint panel for SA devices.
//
//   (C) Copyright 2017-2020 Fred Gleason <fredg@paravelsystems.com>
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
#include <QDesktopWidget>
#include <QGraphicsProxyWidget>
#include <QGraphicsTextItem>
#include <QIcon>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QScrollBar>
#include <QSettings>

#include <sy/sycmdswitch.h>
#include <sy/symcastsocket.h>

#include "xpointpanel.h"

//
// Icons
//
#include "../../icons/drouter-16x16.xpm"
#include "../../icons/greenx.xpm"

MainWidget::MainWidget(QWidget *parent)
  :QWidget(parent)
{
  QString config_filename;
  bool no_creds=false;
  bool ok=false;
  panel_initial_connected=false;
  panel_initial_router=-1;

  //
  // Initialize Variables
  //
  panel_hostname="";
  panel_username="admin";
  panel_password="";

  //
  // Read Command Options
  //
  SyCmdSwitch *cmd=
    new SyCmdSwitch(qApp->argc(),qApp->argv(),"xpointpanel",VERSION,
		    XPOINTPANEL_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--initial-router") {
      panel_initial_router=cmd->value(i).toInt(&ok);
      if(!ok) {
	QMessageBox::warning(this,"XPointPanel - "+tr("Error"),
			     tr("Invalid --initial-router value")+
			     " \""+cmd->value(i)+"\".");
	exit(1);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--hostname") {
      panel_hostname=cmd->value(i);
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
      no_creds=true;
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      QMessageBox::warning(this,"XPointPanel - "+tr("Error"),
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
  // Create And Set Icons
  //
  setWindowIcon(QIcon(drouter_16x16_xpm));
  panel_greenx_map=new QPixmap(greenx_xpm);

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
  panel_login_dialog=new LoginDialog("XPointPanel",this);

  //
  // Router Control
  //
  panel_router_label=new QLabel(tr("Router"),this);
  panel_router_label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  panel_router_label->setFont(button_font);
  panel_router_label->setDisabled(true);
  panel_router_box=new ComboBox(this);

  panel_router_box->setDisabled(true);
  connect(panel_router_box,SIGNAL(activated(int)),
	  this,SLOT(routerBoxActivatedData(int)));
  
  //
  // Endpoint Lists
  //
  panel_description_name_label=new QLabel(this);
  panel_description_name_label->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);
  panel_description_name_label->
    setFont(QFont(font().family(),font().pointSize()-1,QFont::Normal));

  panel_description_text_label=new QLabel(this);
  panel_description_text_label->setAlignment(Qt::AlignLeft|Qt::AlignTop);
  panel_description_text_label->
    setFont(QFont(font().family(),font().pointSize()-2,QFont::Normal));

  panel_input_list=new EndpointList(Qt::Horizontal,this);
  connect(panel_input_list,SIGNAL(hoveredEndpointChanged(int,int)),
  	  this,SLOT(inputHoveredEndpointChangedData(int,int)));
  panel_output_list=new EndpointList(Qt::Vertical,this);
  connect(panel_output_list,SIGNAL(hoveredEndpointChanged(int,int)),
  	  this,SLOT(outputHoveredEndpointChangedData(int,int)));

  //
  // Scroll Area
  //
  panel_scene=new QGraphicsScene(this);
  panel_scene->setBackgroundBrush(Qt::blue);
  panel_view=new XPointView(panel_scene,this);
  panel_view->setAlignment(Qt::AlignLeft|Qt::AlignTop);
  connect(panel_view,SIGNAL(doubleClicked(int,int)),
	  this,SLOT(xpointDoubleClickedData(int,int)));
  connect(panel_view->verticalScrollBar(),SIGNAL(valueChanged(int)),
	  panel_input_list,SLOT(setPosition(int)));
  connect(panel_view->horizontalScrollBar(),SIGNAL(valueChanged(int)),
	  panel_output_list,SLOT(setPosition(int)));

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
  connect(panel_parser,SIGNAL(gpiStateChanged(int,int,const QString &)),
	  panel_input_list,SLOT(setGpioState(int,int,const QString &)));
  connect(panel_parser,SIGNAL(gpoStateChanged(int,int,const QString &)),
	  panel_output_list,SLOT(setGpioState(int,int,const QString &)));

  setWindowTitle("XPointPanel ["+tr("Server")+": "+panel_hostname+"]");

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
  return QSize(440,400);
}


QSizePolicy MainWidget::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
}


void MainWidget::routerBoxActivatedData(int n)
{
  QString name;
  int count=0;
  int endpt=0;
  int router=panel_router_box->itemData(n).toInt();

  //
  // Clear Previous Crosspoints
  //
  QGraphicsScene *scene=new QGraphicsScene(this);
  panel_view->setScene(scene);
  delete panel_scene;
  panel_scene=scene;
  panel_output_list->clearEndpoints();
  panel_input_list->clearEndpoints();
  panel_input_list->setRouter(router);
  panel_output_list->setRouter(router);
  
  //
  // Populate Inputs
  //
  panel_input_list->setShowGpio(panel_parser->gpioSupported(router));
  QMap<int,QString> endpts;
  while(count<panel_parser->inputQuantity(router)) {
    name=panel_parser->inputLongName(router,endpt+1);
    if(!name.isEmpty()) {
      panel_input_list->addEndpoint(router,endpt,
				    QString().sprintf("%d - ",endpt+1)+
				  panel_parser->inputLongName(router,endpt+1));
      panel_input_list->
	setGpioState(router,endpt,panel_parser->gpiState(router,endpt));
      count++;
    }
    endpt++;
  }

  //
  // Populate Outputs
  //
  count=0;
  endpt=0;
  endpts.clear();
  panel_output_list->setShowGpio(panel_parser->gpioSupported(router));
  while(count<panel_parser->outputQuantity(router)) {
    name=panel_parser->outputLongName(router,endpt+1);
    if(!name.isEmpty()) {
      endpts[endpt]=QString().sprintf("%d - ",endpt+1)+
	panel_parser->outputLongName(router,endpt+1);
      count++;
    }
    endpt++;
  }
  panel_output_list->addEndpoints(router,endpts);
  for(int i=0;i<panel_output_list->endpointQuantity();i++) {
    int endpt=panel_output_list->endpoint(i);
    panel_output_list->
      setGpioState(router,endpt,panel_parser->gpoState(router,endpt));
  }

  //
  // Populate Crosspoints
  //
  for(int i=0;i<(panel_input_list->endpointQuantity()+1);i++) {
    panel_scene->
      addLine(0,26*i-1,26*panel_output_list->endpointQuantity(),26*i-1);
  }
  for(int i=0;i<(panel_output_list->endpointQuantity()+1);i++) {
    panel_scene->
      addLine(26*i-1,0,26*i-1,26*panel_input_list->endpointQuantity());
  }
  QList<int> input_endpts=panel_input_list->endpoints();
  QList<int> output_endpts=panel_output_list->endpoints();
  for(int i=0;i<output_endpts.size();i++) {
    for(int j=0;j<input_endpts.size();j++) {
      if(panel_parser->outputCrosspoint(panel_router_box->currentItemData().
					toInt(),output_endpts.at(i)+1)==
	 (input_endpts.at(j)+1)) {
	QGraphicsPixmapItem *item=panel_scene->addPixmap(*panel_greenx_map);
	item->setPos(26*i+5,26*j+5);
      }
    }
  }
  QRect screen=QApplication::desktop()->availableGeometry();
  QSize panel(15+panel_view->sizeHint().width()+
	      panel_input_list->sizeHint().width(),
	      15+panel_view->sizeHint().height()+
	      panel_output_list->sizeHint().width());
  if(panel.width()>screen.width()) {
    panel.setWidth(screen.width()-10);
  }
  if(panel.height()>screen.height()) {
    panel.setHeight(screen.height());
  }
  resize(panel);
  show();
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
		   QString().sprintf("%d - ",it.key())+it.value(),it.key());
      if(it.key()==panel_initial_router) {
	panel_router_box->setCurrentIndex(panel_router_box->count()-1);
      }
    }
    panel_input_list->setParser(panel_parser);
    panel_output_list->setParser(panel_parser);
    panel_router_label->setEnabled(true);
    panel_router_box->setEnabled(true);
    routerBoxActivatedData(panel_router_box->currentIndex());
    panel_initial_connected=true;
  }
  else {
    if(cstate!=SaParser::WatchdogActive) {
      QMessageBox::warning(this,"XPointPanel - "+tr("Error"),
			   tr("Login error")+": "+
			   SaParser::connectionStateString(cstate));
      exit(1);
    }
    panel_router_label->setDisabled(true);
    panel_router_box->setDisabled(true);
  }
}


void MainWidget::errorData(QAbstractSocket::SocketError err)
{
  if(!panel_initial_connected) {
    QMessageBox::warning(this,"XPointPanel - "+tr("Error"),
			 tr("Network Error")+": "+
			 SyMcastSocket::socketErrorText(err));
    exit(1);
  }
}


void MainWidget::outputCrosspointChangedData(int router,int output,int input)
{
  QGraphicsItem *item=NULL;

  if(router==panel_router_box->currentItemData().toInt()) {
    int x_slot=1+panel_output_list->slot(output);
    int y_slot=1+panel_input_list->slot(input);

    //
    // Clear Previous XPoint
    //
    QList<QGraphicsItem *>items=
      panel_scene->items((x_slot-1)*26,0,25,panel_scene->height(),
			 Qt::ContainsItemShape,Qt::AscendingOrder);
    for(int i=0;i<items.size();i++) {
      item=items.at(i);
      panel_scene->removeItem(item);
      delete item;
    }

    //
    // Add New XPoint
    //
    if(input>0) {
      item=panel_scene->addPixmap(*panel_greenx_map);
      item->setPos(26*(x_slot-1)+5,26*(y_slot-1)+5);
    }
  }
}


void MainWidget::xpointDoubleClickedData(int x_slot,int y_slot)
{
  /*
  printf("x_slot: %d  y_slot: %d\n",x_slot,y_slot);
  printf("output: %d  input: %d\n",panel_output_list->endpoint(x_slot),
	 panel_input_list->endpoint(y_slot));
  */
  int input=panel_input_list->endpoint(y_slot)+1;
  int output=panel_output_list->endpoint(x_slot)+1;
  if(panel_parser->outputCrosspoint(panel_router_box->currentItemData().toInt(),
				    output)==input) {  // Mute
    panel_parser->
      setOutputCrosspoint(panel_router_box->currentItemData().toInt(),output,0);
  }
  else {
    panel_parser->
      setOutputCrosspoint(panel_router_box->currentItemData().toInt(),
			  output,input);
  }
}


void MainWidget::inputHoveredEndpointChangedData(int router,int input)
{
  QString tt;

  if(input<0) {
    panel_description_name_label->clear();
    panel_description_text_label->clear();
    return;
  }

  //
  // Set Description Title
  //
  tt="";
  tt+="<strong>"+QString().sprintf("%d - ",input);
  tt+=panel_parser->inputName(router,input)+"</strong>";
  tt+=" ON ";
  tt+="<strong>"+panel_parser->inputNodeName(router,input)+"</strong>";
  panel_description_name_label->setText(tt);
  
  //
  // Set Description Text
  //
  tt="";
  if(panel_parser->inputSourceNumber(router,input)>0) {
    tt+=QString().sprintf("Source Number: <strong>%d</strong><br>",
			  panel_parser->inputSourceNumber(router,input));
    tt+="Stream Address: <strong>"+
      panel_parser->inputStreamAddress(router,input).toString()+
      "</strong><br>";
  }
  tt+="Node Address/Slot: <strong>"+
    panel_parser->inputNodeAddress(router,input).toString();
  tt+=QString().sprintf("/%d</strong>",1+panel_parser->
			  inputNodeSlotNumber(router,input));
  panel_description_text_label->setText(tt);
}


void MainWidget::outputHoveredEndpointChangedData(int router,int output)
{
  QString tt;

  if(output<0) {
    panel_description_name_label->clear();
    panel_description_text_label->clear();
    return;
  }

  //
  // Set Description Title
  //
  tt="";
  tt+="<strong>"+QString().sprintf("%d - ",output);
  tt+=panel_parser->outputName(router,output)+"</strong>";
  tt+=" ON ";
  tt+="<strong>"+panel_parser->outputNodeName(router,output)+"</strong>";
  panel_description_name_label->setText(tt);

  //
  // Set Description Text
  //
  tt="";
  tt+="Node Address/Slot: <strong>"+
    panel_parser->outputNodeAddress(router,output).toString();
  tt+=QString().sprintf("/%d</strong>",
			1+panel_parser->
			outputNodeSlotNumber(router,output));
  panel_description_text_label->setText(tt);
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  int info_width=panel_input_list->sizeHint().width();
  if(panel_output_list->sizeHint().width()>info_width) {
    info_width=panel_output_list->sizeHint().width();
  }

  panel_router_label->
    setGeometry(15,10,info_width-10,20);
  panel_router_box->
    setGeometry(10,32,info_width-10,20);


  panel_description_name_label->
    setGeometry(0,64,
		info_width+10,20);
  panel_description_text_label->
    setGeometry(10,90,
		info_width,
		panel_output_list->sizeHint().width()-90);



  panel_input_list->
    setGeometry(10,
		panel_output_list->sizeHint().width()+10,
		info_width-0,
		e->size().height()-(panel_output_list->sizeHint().width()-45));
  panel_output_list->
    setGeometry(info_width+10,10,
		e->size().width()-10,panel_output_list->sizeHint().width()+1);
  
  int view_width=3+26*panel_output_list->endpointQuantity();
  int bar_width=0;
  int view_height=3+26*panel_input_list->endpointQuantity();
  int bar_height=0;
  int bar_x=0;
  int bar_y=0;
  if(view_width>(e->size().width()-(info_width+10))) {
      view_width=e->size().width()-(info_width+10);
      bar_x=1;
      bar_height=15;
  }
  if(view_height>(e->size().height()-(panel_output_list->sizeHint().width()+15))) {
    view_height=e->size().height()-(panel_output_list->sizeHint().width()+15);
    bar_y=1;
    bar_width=15;
  }
  if((bar_width!=0)&&(bar_height!=0)) {
    bar_width=0;
    bar_height=0;
  }
  panel_view->setGeometry(info_width+bar_x+9,panel_output_list->sizeHint().width()+bar_y+9,view_width+bar_width,view_height+bar_height);
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);

  //
  // Start Event Loop
  //
  MainWidget *w=new MainWidget(NULL);
  w->setGeometry(w->geometry().x(),w->geometry().y(),w->sizeHint().width(),w->sizeHint().height());
  return a.exec();
}
