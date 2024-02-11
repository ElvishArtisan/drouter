// xpointpanel.cpp
//
// Full graphical crosspoint panel for SA devices.
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
#include <QDesktopWidget>
#include <QGraphicsProxyWidget>
#include <QGraphicsTextItem>
#include <QIcon>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QScreen>
#include <QScrollBar>
#include <QSettings>

#include <sy5/sycmdswitch.h>
#include <sy5/symcastsocket.h>
#include <sy5/synode.h>

#include <drendpointlistmodel.h>

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
  bool prompt=false;
  bool ok=false;
  panel_initial_connected=false;
  panel_initial_router=-1;
  panel_size_hint=QSize(400,400);

  //
  // Initialize Variables
  //
  panel_hostname="";
  panel_username="xpointpanel";
  panel_password="";

  //
  // Read Command Options
  //
  SyCmdSwitch *cmd=new SyCmdSwitch("xpointpanel",VERSION,XPOINTPANEL_USAGE);
  for(int i=0;i<cmd->keys();i++) {
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
    if(cmd->key(i)=="--prompt") {
      prompt=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--no-creds") {
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
  setWindowIcon(QPixmap(drouter_16x16_xpm));
  panel_greenx_map=new QPixmap(greenx_xpm);

  //
  // Fonts
  //
  QFont label_font("helvetica",12,QFont::Bold);
  label_font.setPixelSize(12);
  QFont button_font("helvetica",14,QFont::Bold);
  button_font.setPixelSize(14);
  QFont endpoint_label_font("helvetica",18,QFont::Bold);
  endpoint_label_font.setPixelSize(18);

  //
  // Dialogs
  //
  panel_login_dialog=new DRLoginDialog("XPointPanel",this);

  //
  // Router Control
  //
  panel_router_label=new QLabel(tr("Router"),this);
  panel_router_label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  panel_router_label->setFont(button_font);
  panel_router_label->setDisabled(true);
  panel_router_box=new QComboBox(this);

  panel_router_box->setDisabled(true);
  connect(panel_router_box,SIGNAL(activated(int)),
	  this,SLOT(routerBoxActivatedData(int)));

  panel_inputs_label=new QLabel(tr("Inputs (Sources)"),this);
  panel_inputs_label->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);
  panel_inputs_label->setFont(endpoint_label_font);

  panel_outputs_label=new SideLabel(tr("Outputs (Destinations)"),this);
  panel_outputs_label->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);
  panel_outputs_label->setFont(endpoint_label_font);

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
  connect(panel_view,SIGNAL(crosspointSelected(int,int)),
	  panel_input_list,SLOT(selectCrosspoint(int,int)));
  connect(panel_view,SIGNAL(crosspointSelected(int,int)),
	  this,SLOT(crosspointSelectedData(int,int)));
  connect(panel_view,SIGNAL(crosspointSelected(int,int)),
  	  panel_output_list,SLOT(selectCrosspoint(int,int)));
  connect(panel_view,SIGNAL(crosspointDoubleClicked(int,int)),
	  this,SLOT(xpointDoubleClickedData(int,int)));
  connect(panel_view->verticalScrollBar(),SIGNAL(valueChanged(int)),
	  panel_input_list,SLOT(setPosition(int)));
  connect(panel_view->horizontalScrollBar(),SIGNAL(valueChanged(int)),
	  panel_output_list,SLOT(setPosition(int)));

  //
  // The Protocol J Connection
  //
  panel_parser=new DRJParser(true,this);
  connect(panel_parser,SIGNAL(connected(bool,DRJParser::ConnectionState)),
	  this,SLOT(connectedData(bool,DRJParser::ConnectionState)));
  connect(panel_parser,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
  connect(panel_parser,SIGNAL(outputCrosspointChanged(int,int,int)),
	  this,SLOT(outputCrosspointChangedData(int,int,int)));
  connect(panel_parser,SIGNAL(gpiStateChanged(int,int,const QString &)),
	  panel_input_list,SLOT(setGpioState(int,int,const QString &)));
  connect(panel_parser,SIGNAL(gpoStateChanged(int,int,const QString &)),
	  panel_output_list,SLOT(setGpioState(int,int,const QString &)));

  //
  // The ProtocolD Connection
  //
  panel_dparser=new DRDParser(this);
  connect(panel_dparser,SIGNAL(connected(bool)),
	  this,SLOT(protocolDConnected(bool)));

  setWindowTitle(QString("Drouter - XPointPanel [")+VERSION+"]");

  if(prompt) {
    if(!panel_login_dialog->exec(&panel_username,&panel_password)) {
      exit(1);
    }
  }
  panel_parser->
    connectToHost(panel_hostname,9600,panel_username,panel_password);
  panel_dparser->connectToHost(panel_hostname,23883);
}


MainWidget::~MainWidget()
{
}


QSize MainWidget::sizeHint() const
{
  return panel_size_hint;
}


QSizePolicy MainWidget::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
}


void MainWidget::routerBoxActivatedData(int n)
{
  QString name;
  int endpt=0;
  int router=SelectedRouter();
  DREndPointListModel *imodel=panel_parser->inputModel(router);
  DREndPointListModel *omodel=panel_parser->outputModel(router);
  QMap<QString,QVariant> mdata;

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
  for(int i=0;i<imodel->rowCount();i++) {
    mdata=imodel->rowMetadata(i);
    panel_input_list->addEndpoint(router,endpt,
				  QString::asprintf("%d - ",endpt+1)+
				  mdata.value("name").toString());
    panel_input_list->
      setGpioState(router,endpt,panel_parser->gpiState(router,i));
    endpt++;
  }

  //
  // Populate Outputs
  //
  endpt=0;
  panel_output_list->setShowGpio(panel_parser->gpioSupported(router));
  endpts.clear();
  int count=0;
  while(count<omodel->rowCount()) {
    mdata=omodel->rowMetadata(count);
    panel_output_list->addEndpoint(router,endpt,
				   QString::asprintf("%d - ",endpt+1)+
				   mdata.value("name").toString());
    panel_output_list->
      setGpioState(router,endpt,panel_parser->gpoState(router,endpt));
    count++;
    endpt++;
  }

  //
  // Populate Crosspoints
  //
  QPen line_pen(palette().color(QPalette::WindowText));
  for(int i=0;i<(panel_input_list->endpointQuantity()+1);i++) {
    panel_scene->
      addLine(0,26*i-1,26*panel_output_list->endpointQuantity()-1,26*i-1,
	      line_pen);
  }
  for(int i=0;i<(panel_output_list->endpointQuantity()+1);i++) {
    panel_scene->
      addLine(26*i-1,0,26*i-1,26*panel_input_list->endpointQuantity()-1,
	      line_pen);
  }
  for(int i=0;i<imodel->rowCount();i++) {
    for(int j=0;j<omodel->rowCount();j++) {
      if(panel_parser->outputCrosspoint(SelectedRouter(),
					omodel->endPointNumber(j))==
	 imodel->endPointNumber(i)) {
	QGraphicsPixmapItem *item=panel_scene->addPixmap(*panel_greenx_map);
	item->setPos(26*i+5,26*j+5);
      }
    }
  }

  panel_view->setXSlotQuantity(omodel->rowCount());
  panel_view->setYSlotQuantity(imodel->rowCount());
  QRect screen=QApplication::desktop()->availableGeometry(this);
  screen.setHeight(screen.height()-30); // Hack to compensate for window titlebar

  int info_width=15+panel_input_list->sizeHint().width();
  if(panel_output_list->sizeHint().width()>info_width) {
    info_width=15+panel_output_list->sizeHint().width();
  }
  QSize panel(24+panel_output_list->sizeHint().height()+info_width,
	      15+panel_view->sizeHint().height()+
  	      panel_output_list->sizeHint().width());
  if(panel.width()>screen.width()) {
    panel.setWidth(screen.width()-10);
    panel.setHeight(panel.height()+10);
  }
  if(panel.height()>screen.height()) {
    panel.setHeight(screen.height()-10);
    panel.setWidth(panel.width()+10);
  }
  panel_size_hint=panel;
  if(panel_initial_connected) {
    resize(panel);
  }
  else {
    setGeometry((screen.width()-panel.width())/2,
		(screen.height()-panel.height())/2,
		panel.width(),
		panel.height());
  }
  show();
}


void MainWidget::connectedData(bool state,DRJParser::ConnectionState cstate)
{
  if(state) {
    panel_router_box->setModel(panel_parser->routerModel());
    panel_input_list->setParser(panel_parser);
    panel_output_list->setParser(panel_parser);
    panel_router_label->setEnabled(true);
    panel_router_box->setEnabled(true);
    if(panel_initial_router>0) {
      panel_router_box->setCurrentIndex(panel_parser->routerModel()->
					rowNumber(panel_initial_router));
    }
    routerBoxActivatedData(panel_router_box->currentIndex());
    panel_initial_connected=true;
  }
  else {
    if(cstate!=DRJParser::WatchdogActive) {
      QMessageBox::warning(this,"XPointPanel - "+tr("Error"),
			   tr("Login error")+": "+
			   DRJParser::connectionStateString(cstate));
      exit(1);
    }
    panel_router_label->setDisabled(true);
    panel_router_box->setDisabled(true);
  }
}


void MainWidget::protocolDConnected(bool state)
{
  if(!state) {
    fprintf(stderr,"unable to connect to protocolD server\n");
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

  if(router==SelectedRouter()) {
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
  DREndPointListModel *imodel=panel_parser->inputModel(SelectedRouter());
  DREndPointListModel *omodel=panel_parser->outputModel(SelectedRouter());

  int input=imodel->endPointNumber(y_slot);
  int output=omodel->endPointNumber(x_slot);
  if(panel_parser->outputCrosspoint(SelectedRouter(),output)==input) {  // Mute
    panel_parser->setOutputCrosspoint(SelectedRouter(),output,0);
  }
  else {
    panel_parser->setOutputCrosspoint(SelectedRouter(),output,input);
  }
}


void MainWidget::inputHoveredEndpointChangedData(int router,int rownum)
{
  QString tt;
  DRRouterListModel *rmodel=panel_parser->routerModel();
  DREndPointListModel *imodel=panel_parser->inputModel(router);
  QMap<QString,QVariant> mdata;

  if(rownum<0) {
    panel_description_name_label->clear();
    panel_description_text_label->clear();
    return;
  }

  //
  // Set Description Title
  //
  panel_description_name_label->setText(InputDescriptionTitle(router,rownum));
  
  //
  // Set Description Text
  //
  mdata=imodel->rowMetadata(rownum);
  tt="";
  if(mdata.contains("hostDescription")) {
    tt+="Device: <strong>"+
      mdata.value("hostDescription").toString()+
      "</strong><br>";
  }
  if(mdata.contains("sourceNumber")) {
    tt+=QString::asprintf("Source Number: <strong>%d</strong><br>",
			  mdata.value("sourceNumber").toInt());
  }
  switch(rmodel->routerType(rmodel->rowNumber(router))) {
  case DREndPointMap::AudioRouter:
    if(mdata.contains("hostAddress")&&mdata.contains("slot")) {
      tt+="Host Address: <strong>"+
	mdata.value("hostAddress").toString()+
	QString::asprintf("/%d</strong>",mdata.value("slot").toInt());
    }
    if(mdata.contains("streamAddress")) {
      tt+="Stream Address: <strong>"+
	mdata.value("streamAddress").toString()+"</strong>";
    }
    break;

  case DREndPointMap::GpioRouter:
    if(mdata.contains("gpioAddress")) {
      tt+="GPIO Address: <strong>"+
	mdata.value("gpioAddress").toString()+"</strong>";
    }
    break;

  case DREndPointMap::LastRouter:
    break;
  }
  panel_description_text_label->setText(tt);
}


void MainWidget::outputHoveredEndpointChangedData(int router,int rownum)
{
  QString tt;
  DREndPointListModel *omodel=panel_parser->outputModel(router);
  QMap<QString,QVariant> mdata;

  if(rownum<0) {
    panel_description_name_label->clear();
    panel_description_text_label->clear();
    return;
  }

  //
  // Set Description Title
  //
  panel_description_name_label->setText(OutputDescriptionTitle(router,rownum));

  //
  // Set Description Text
  //
  mdata=omodel->rowMetadata(rownum);
  tt="";
  if(mdata.contains("hostDescription")) {
    tt+="Device: <strong>"+
      mdata.value("hostDescription").toString()+
      "</strong><br>";
  }
  if(mdata.contains("hostAddress")&&mdata.contains("slot")) {
    tt+="Node Address/Slot: <strong>"+mdata.value("hostAddress").toString();
    tt+=QString::asprintf("/%d</strong>",mdata.value("slot").toInt());
  }
  panel_description_text_label->setText(tt);
}


void MainWidget::crosspointSelectedData(int slot_x,int slot_y)
{
  DREndPointListModel *imodel=panel_parser->inputModel(SelectedRouter());
  DREndPointListModel *omodel=panel_parser->outputModel(SelectedRouter());

  if((slot_x<0)||(slot_x>=omodel->rowCount())||
     (slot_y<0)||(slot_y>=imodel->rowCount())) {
    panel_description_name_label->clear();
    panel_description_text_label->clear();
  }
  else {
    panel_description_name_label->
      setText("<strong>"+tr("Selected Crosspoint")+"</strong>");
    panel_description_text_label->
      setText("<strong>"+tr("Output (Destination)")+":"+"</strong><br>"+
	      "&nbsp;&nbsp;&nbsp;"+
	      OutputDescriptionTitle(SelectedRouter(),slot_x)+
	      "<br><br>"+
	      "<strong>"+tr("Input (Source)")+":"+"</strong><br>"+
	      "&nbsp;&nbsp;&nbsp;"+
	      InputDescriptionTitle(SelectedRouter(),slot_y));
  }
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  int info_width=panel_input_list->sizeHint().width()+20;
  if(panel_output_list->sizeHint().width()>info_width) {
    info_width=panel_output_list->sizeHint().width()+20;
  }

  panel_router_label->
    setGeometry(15,10,info_width-10,20);
  panel_router_box->setGeometry(10,32,info_width-30,20);


  panel_description_name_label->
    setGeometry(10,64,
		info_width-ENDPOINTLIST_ITEM_HEIGHT,20);
  panel_description_text_label->
    setGeometry(15,90,
		info_width-ENDPOINTLIST_ITEM_HEIGHT-5,
		panel_output_list->sizeHint().width()-106);

  panel_inputs_label->
    setGeometry(10,
		panel_output_list->sizeHint().width()-15,
		info_width-23,
		26);
  panel_input_list->
    setGeometry(10,
		panel_output_list->sizeHint().width()+10,
		info_width,
		e->size().height()-(panel_output_list->sizeHint().width()-45));

  panel_outputs_label->
    setGeometry(info_width-14,
		10,
		26,
		panel_output_list->sizeHint().width()-24);
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

  //
  // Horrible Hack to deal with the idiotic "disappearing" scroll bars in
  // Adwaita styles.
  //
  if(QString(QApplication::style()->metaObject()->className()).toLower().
     contains("adwaita")) {
    bar_width+=2;
    bar_height+=2;
  }

  panel_view->setGeometry(info_width+bar_x+9,
			  panel_output_list->sizeHint().width()+bar_y+9,
			  view_width+bar_width,
			  view_height+bar_height);

  setMinimumWidth(10+panel_input_list->width());
  setMinimumHeight(10+panel_output_list->height());
}


void MainWidget::paintEvent(QPaintEvent *e)
{
  QPainter *p=new QPainter(this);
  int info_width=panel_input_list->sizeHint().width()+20;
  if(panel_output_list->sizeHint().width()>info_width) {
    info_width=panel_output_list->sizeHint().width()+20;
  }
  p->setPen(palette().color(QPalette::WindowText));
  p->setBrush(palette().color(QPalette::WindowText));

  p->drawLine(10,
	      panel_output_list->sizeHint().width()+10,
	      10,
	      panel_output_list->sizeHint().width()-16);
  p->drawLine(10,
	      panel_output_list->sizeHint().width()-16,
	      info_width-16,
	      panel_output_list->sizeHint().width()-16);


  p->drawLine(info_width+10,
	      10,
	      info_width-16,
	      10);
  p->drawLine(info_width-16,
	      10,
	      info_width-16,
	      panel_output_list->sizeHint().width()-16);	      

  p->drawLine(info_width-16,
	      panel_output_list->sizeHint().width()-16,
	      info_width+10,
	      panel_output_list->sizeHint().width()+10);	      
  delete p;
}


QString MainWidget::InputDescriptionTitle(int router,int rownum) const
{
  DREndPointListModel *imodel=panel_parser->inputModel(router);
  QMap<QString,QVariant> mdata=imodel->rowMetadata(rownum);

  QString ret="";
  ret+="<strong>"+QString::asprintf("%d - ",mdata.value("number").toInt());
  ret+=mdata.value("name").toString()+"</strong>";

  return ret;
}


QString MainWidget::OutputDescriptionTitle(int router,int rownum) const
{
  DREndPointListModel *omodel=panel_parser->outputModel(router);
  QMap<QString,QVariant> mdata=omodel->rowMetadata(rownum);

  QString ret="";
  ret+="<strong>"+QString::asprintf("%d - ",mdata.value("number").toInt());
  ret+=mdata.value("name").toString()+"</strong>";

  return ret;
}


int MainWidget::SelectedRouter() const
{
  return panel_parser->
    routerModel()->routerNumber(panel_router_box->currentIndex());
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);
  new MainWidget(NULL);
  return a.exec();
}
