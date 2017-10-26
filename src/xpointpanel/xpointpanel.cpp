// xpointpanel.cpp
//
// Full graphical crosspoint panel for SA devices.
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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
  panel_initial_connected=false;

  //
  // Initialize Variables
  //
  panel_hostname="localhost";
  panel_username="admin";
  panel_password="";

  //
  // Read Command Options
  //
  SyCmdSwitch *cmd=
    new SyCmdSwitch(qApp->argc(),qApp->argv(),"xpointpanel",VERSION,
		    XPOINTPANEL_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
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
    if(!cmd->processed(i)) {
      QMessageBox::warning(this,"XPointPanel - "+tr("Error"),
			   tr("Unknown argument")+" \""+cmd->key(i)+"\".");
      exit(1);
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
  panel_router_label=new QLabel(tr("Router")+":",this);
  panel_router_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  panel_router_label->setFont(button_font);
  panel_router_label->setDisabled(true);
  panel_router_box=new ComboBox(this);
  panel_router_box->setDisabled(true);
  connect(panel_router_box,SIGNAL(activated(int)),
	  this,SLOT(routerBoxActivatedData(int)));

  //
  // Endpoint Lists
  //
  panel_input_list=new EndpointList(EndpointList::Horizontal,this);
  panel_output_list=new EndpointList(EndpointList::Vertical,this);

  //
  // Scroll Area
  //
  panel_scene=new QGraphicsScene(this);
  panel_view=new XPointView(panel_scene,this);
  panel_view->setAlignment(Qt::AlignLeft|Qt::AlignTop);
  connect(panel_view,SIGNAL(doubleClicked(int,int)),
	  this,SLOT(xpointDoubleClickedData(int,int)));
  connect(panel_view->verticalScrollBar(),SIGNAL(valueChanged(int)),
	  panel_input_list,SLOT(setPosition(int)));
  connect(panel_view->horizontalScrollBar(),SIGNAL(valueChanged(int)),
	  panel_output_list,SLOT(setPosition(int)));

  //
  // Fix the Window Size
  //
  //  setMinimumSize(sizeHint());

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

  setWindowTitle("XPointPanel ["+tr("Server")+": "+panel_hostname+"]");

  if(panel_password.isEmpty()) {
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
  return QSize(800,600);
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
  
  //
  // Populate Inputs
  //
  QMap<int,QString> endpts;
  while(count<panel_parser->inputQuantity(router)) {
    name=panel_parser->inputLongName(router,endpt+1);
    if(!name.isEmpty()) {
      panel_input_list->addEndpoint(endpt,QString().sprintf("%d - ",endpt+1)+
				  panel_parser->inputLongName(router,endpt+1));
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
  while(count<panel_parser->outputQuantity(router)) {
    name=panel_parser->outputLongName(router,endpt+1);
    if(!name.isEmpty()) {
      endpts[endpt]=QString().sprintf("%d - ",endpt+1)+
	panel_parser->outputLongName(router,endpt+1);
      count++;
    }
    endpt++;
  }
  panel_output_list->addEndpoints(endpts);

  //
  // Populate Crosspoints
  //
  for(int i=1;i<panel_input_list->endpointQuantity();i++) {
    panel_scene->
      addLine(0,26*i-1,26*panel_output_list->endpointQuantity(),26*i-1);
  }
  for(int i=1;i<panel_output_list->endpointQuantity();i++) {
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
  resizeEvent(NULL);
}


void MainWidget::connectedData(bool state,SaParser::ConnectionState cstate)
{
  if(state) {
    QMap<int,QString> routers=panel_parser->routers();
    panel_router_box->clear();
    for(QMap<int,QString>::const_iterator it=routers.begin();it!=routers.end();
	it++) {
      panel_router_box->
	insertItem(panel_router_box->count(),it.value(),it.key());
    }
    panel_router_box->setEnabled(true);
    panel_router_box->setCurrentIndex(0);
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
    //
    // Clear Previous XPoint
    //
    QList<QGraphicsItem *>items=panel_scene->items(1+(output-1)*25,0,(output*25)-1,panel_scene->height(),Qt::ContainsItemShape,Qt::AscendingOrder);
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
      item->setPos(26*(output-1)+5,26*(input-1)+5);
    }
  }
}


void MainWidget::xpointDoubleClickedData(int output,int input)
{
  //  printf("output: %d  input: %d\n",output,input);
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


void MainWidget::resizeEvent(QResizeEvent *e)
{
  panel_router_label->setGeometry(15,8,60,20);
  panel_router_box->setGeometry(80,8,200,20);
  panel_input_list->setGeometry(10,panel_output_list->sizeHint().width()+35,panel_input_list->sizeHint().width()+1,size().height()-(panel_output_list->sizeHint().width()-45));
  panel_output_list->setGeometry(panel_input_list->sizeHint().width()+10,35,size().width()-10,panel_output_list->sizeHint().width()+1);
  
  int view_width=2+26*panel_output_list->endpointQuantity();
  int bar_width=0;
  int view_height=2+26*panel_input_list->endpointQuantity();
  int bar_height=0;
  if(view_width>(size().width()-(panel_input_list->sizeHint().width()+10))) {
      view_width=size().width()-(panel_input_list->sizeHint().width()+10);
    bar_height=15;
  }
  if(view_height>(size().height()-(panel_output_list->sizeHint().width()+35))) {
    view_height=size().height()-(panel_output_list->sizeHint().width()+35);
    bar_width=15;
  }
  if((bar_width!=0)&&(bar_height!=0)) {
    bar_width=0;
    bar_height=0;
  }
  panel_view->setGeometry(panel_input_list->sizeHint().width()+10,panel_output_list->sizeHint().width()+35,view_width+bar_width,view_height+bar_height);
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
