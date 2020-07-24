// endpointlist.cpp
//
// Input/Output labels for xpointpanel(1)
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

#include <stdio.h>
#include <unistd.h>

#include <QApplication>
#include <QClipboard>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

#include "endpointlist.h"

EndpointList::EndpointList(Qt::Orientation orient,QWidget *parent)
  : QWidget(parent)
{
  list_orientation=orient;
  list_show_gpio=false;
  list_position=0;
  list_width=0;
  list_mouse_endpoint=-1;
  list_move_endpoint=-1;
  list_router=0;

  setMouseTracking(true);

  //
  // Dialogs
  //
  switch(orient) {
  case Qt::Horizontal:
    list_gpio_type=EndPointMap::Input;
    break;

  case Qt::Vertical:
    list_gpio_type=EndPointMap::Output;
    break;
  }

  list_mouse_menu=new QMenu(this);
  list_state_dialog_action=list_mouse_menu->
    addAction(tr("Set GPIO State"),this,SLOT(showStateDialogData()));
  list_mouse_menu->addSeparator();
  list_connect_via_http_action=list_mouse_menu->
    addAction("",this,SLOT(connectViaHttpData()));
  list_connect_via_lwrp_action=list_mouse_menu->
    addAction("",this,SLOT(connectViaLwrpData()));
  list_mouse_menu->addSeparator();
  list_copy_source_number_action=list_mouse_menu->
    addAction(tr("Copy Source Number"),this,SLOT(copySourceNumberData()));
  list_copy_source_address_action=list_mouse_menu->
    addAction(tr("Copy Source Stream Address"),
	      this,SLOT(copySourceStreamAddressData()));
  list_copy_host_address_action=list_mouse_menu->
    addAction(tr("Copy Node Address"),this,SLOT(copyNodeAddressData()));
  list_copy_slot_number_action=list_mouse_menu->
    addAction(tr("Copy Slot Number"),this,SLOT(copySlotNumberData()));  
  connect(list_mouse_menu,SIGNAL(aboutToShow()),
	  this,SLOT(aboutToShowMenuData()));
}


EndpointList::~EndpointList()
{
}


QSize EndpointList::sizeHint() const
{
  if(list_show_gpio) {
    return QSize(15+list_width+70,ENDPOINTLIST_ITEM_HEIGHT*list_labels.size());
  }
  return QSize(15+list_width,ENDPOINTLIST_ITEM_HEIGHT*list_labels.size());
}


QSizePolicy EndpointList::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
}


int EndpointList::router() const
{
  return list_router;
}


void EndpointList::setRouter(int router)
{
  list_router=router;
}


void EndpointList::setParser(SaParser *psr)
{
  list_parser=psr;
}


bool EndpointList::showGpio() const
{
  return list_show_gpio;
}


void EndpointList::setShowGpio(bool state)
{
  list_show_gpio=state;
}


int EndpointList::endpoint(int slot) const
{
  return (list_labels.begin()+slot-1).key();
}


QList<int> EndpointList::endpoints() const
{
  QList<int> ret;

  for(QMap<int,QString>::const_iterator it=list_labels.begin();
      it!=list_labels.end();it++) {
    ret.push_back(it.key());
  }
  return ret;
}


int EndpointList::slot(int endpt) const
{
  int ret=0;

  for(QMap<int,QString>::const_iterator it=list_labels.begin();
      it!=list_labels.end();it++) {
    if((it.key()+1)==endpt) {
      return ret;
    }
    ret++;
  }
  return -1;
}


void EndpointList::addEndpoint(int router,int endpt,const QString &name)
{
  list_endpoints.push_back(endpt);
  list_labels[endpt]=name;
  list_gpio_widgets[endpt]=
    new MultiStateWidget(router,endpt,list_orientation,this);
  list_gpio_widgets.value(endpt)->setVisible(list_show_gpio);

  QFontMetrics fm(font());
  for(QMap<int,QString>::const_iterator it=list_labels.begin();
      it!=list_labels.end();it++) {
    if(fm.width(it.value())>list_width) {
      list_width=fm.width(it.value());
    }
  }

  update();
}


void EndpointList::addEndpoints(int router,const QMap<int,QString> &endpts)
{
  QFontMetrics fm(font());
  for(QMap<int,QString>::const_iterator it=endpts.begin();it!=endpts.end();
      it++) {
    list_endpoints.push_back(it.key());
    list_labels[it.key()]=it.value();
    list_gpio_widgets[it.key()]=
      new MultiStateWidget(router,it.key(),list_orientation,this);
    list_gpio_widgets.value(it.key())->setVisible(list_show_gpio);
    if(fm.width(it.value())>list_width) {
      list_width=fm.width(it.value());
    }
  }
  update();
}


void EndpointList::clearEndpoints()
{
  list_labels.clear();
  list_width=0;

  for(QMap<int,MultiStateWidget *>::const_iterator it=list_gpio_widgets.begin();
      it!=list_gpio_widgets.end();it++) {
    delete it.value();
  }
  list_gpio_widgets.clear();

  for(QMap<int,StateDialog *>::const_iterator it=list_state_dialogs.begin();
      it!=list_state_dialogs.end();it++) {
    delete it.value();
  }
  list_state_dialogs.clear();

  update();
}


int EndpointList::endpointQuantity() const
{
  return list_labels.size();
}


void EndpointList::setPosition(int pixels)
{
  list_position=pixels;
  repaint();
}


void EndpointList::setGpioState(int router,int linenum,const QString &code)
{
  for(QMap<int,MultiStateWidget *>::const_iterator it=list_gpio_widgets.begin();
      it!=list_gpio_widgets.end();it++) {
    it.value()->setState(router,linenum,code);
  }
}


void EndpointList::aboutToShowMenuData()
{
  list_state_dialog_action->setEnabled(list_show_gpio);
  switch(list_orientation) {
  case Qt::Horizontal:
    list_connect_via_http_action->
      setText(tr("Connect to")+" "+
	      list_parser->inputNodeName(list_router,1+list_mouse_endpoint)+" "+
	      tr("via HTTP"));
    list_connect_via_lwrp_action->
      setText(tr("Connect to")+" "+
	      list_parser->inputNodeName(list_router,1+list_mouse_endpoint)+" "+
	      tr("via LWRP"));
    break;

  case Qt::Vertical:
    list_connect_via_http_action->
      setText(tr("Connect to")+" "+
	      list_parser->outputNodeName(list_router,1+list_mouse_endpoint)+" "+
	      tr("via HTTP"));
    list_connect_via_lwrp_action->
      setText(tr("Connect to")+" "+
	      list_parser->outputNodeName(list_router,1+list_mouse_endpoint)+" "+
	      tr("via LWRP"));
    break;
  }
  list_connect_via_http_action->setEnabled(true);
  list_connect_via_lwrp_action->setEnabled(true);
  list_copy_source_number_action->setEnabled(list_orientation==Qt::Horizontal);
  list_copy_source_address_action->setEnabled(list_orientation==Qt::Horizontal);
  list_copy_host_address_action->setEnabled(true);
  list_copy_slot_number_action->setEnabled(true);
}


void EndpointList::showStateDialogData()
{
  QRect geo;

  if(list_state_dialogs.value(list_mouse_endpoint)==NULL) {
    list_state_dialogs[list_mouse_endpoint]=
      new StateDialog(list_router,list_mouse_endpoint,list_gpio_type,
		      list_parser,this);
    geo=QRect(list_mouse_position.x(),list_mouse_position.y(),
	   list_state_dialogs.value(list_mouse_endpoint)->sizeHint().width(),
	   list_state_dialogs.value(list_mouse_endpoint)->sizeHint().height());
  }
  else {
    geo=list_state_dialogs.value(list_mouse_endpoint)->geometry();
  }
  list_state_dialogs.value(list_mouse_endpoint)->show();
  if(!geo.isNull()) {
    list_state_dialogs.value(list_mouse_endpoint)->setGeometry(geo);
  }
}


void EndpointList::connectViaHttpData()
{
  char c_str[256];

  switch(list_orientation) {
  case Qt::Horizontal:
    strncpy(c_str,list_parser->inputNodeAddress(list_router,
						list_mouse_endpoint).
	    toString().toUtf8().constData(),255);
    if(fork()==0) {
      execlp("firefox","firefox",c_str,(char *)NULL);
      exit(0);
    }
    break;

  case Qt::Vertical:
    strncpy(c_str,list_parser->outputNodeAddress(list_router,
						 list_mouse_endpoint).
	    toString().toUtf8().constData(),255);
    if(fork()==0) {
      execlp("firefox","firefox",c_str,(char *)NULL);
      exit(0);
    }
    break;
  }
}


void EndpointList::connectViaLwrpData()
{
  char c_str[256];

  switch(list_orientation) {
  case Qt::Horizontal:
    strncpy(c_str,list_parser->inputNodeAddress(list_router,
						list_mouse_endpoint+1).
	    toString().toUtf8().constData(),255);
    if(fork()==0) {
      execlp("lwmon","lwmon","--mode=lwrp",c_str,(char *)NULL);
      exit(0);
    }
    break;

  case Qt::Vertical:
    strncpy(c_str,list_parser->outputNodeAddress(list_router,
						 list_mouse_endpoint+1).
	    toString().toUtf8().constData(),255);
    if(fork()==0) {
      execlp("lwmon","lwmon","--mode=lwrp",c_str,(char *)NULL);
      exit(0);
    }
    break;
  }
}


void EndpointList::copySourceNumberData()
{
  QClipboard *cb=QApplication::clipboard();
  cb->setText(QString().sprintf("%d",list_parser->
				inputSourceNumber(list_router,
						  1+list_mouse_endpoint)));
}


void EndpointList::copySourceStreamAddressData()
{
  QClipboard *cb=QApplication::clipboard();
  cb->setText(list_parser->
	      inputStreamAddress(list_router,1+list_mouse_endpoint).toString());
}


void EndpointList::copyNodeAddressData()
{
  QClipboard *cb=QApplication::clipboard();
  switch(list_orientation) {
  case Qt::Horizontal:
    cb->setText(list_parser->
		inputNodeAddress(list_router,1+list_mouse_endpoint).toString());
    break;

  case Qt::Vertical:
    cb->setText(list_parser->
		outputNodeAddress(list_router,1+list_mouse_endpoint).
		toString());
    break;
  }
}


void EndpointList::copySlotNumberData()
{
  QClipboard *cb=QApplication::clipboard();
  switch(list_orientation) {
  case Qt::Horizontal:
    cb->setText(QString().sprintf("%d",1+list_parser->
				  inputNodeSlotNumber(list_router,
						      1+list_mouse_endpoint)));
    break;

  case Qt::Vertical:
    cb->setText(QString().sprintf("%d",1+list_parser->
				  outputNodeSlotNumber(list_router,
						       1+list_mouse_endpoint)));
    break;
  }
}


void EndpointList::mousePressEvent(QMouseEvent *e)
{
  if((list_mouse_endpoint=LocalEndpoint(e))>=0) {
    list_mouse_position=e->globalPos();
    list_mouse_menu->popup(e->globalPos());
  }
  QWidget::mousePressEvent(e);
}


void EndpointList::mouseMoveEvent(QMouseEvent *e)
{
  //
  // Get the Endpoint
  //
  int listpos=(e->pos().x()+list_position)/ENDPOINTLIST_ITEM_HEIGHT;
  QPoint pos(e->globalPos().x(),e->globalPos().y()-ENDPOINTLIST_ITEM_HEIGHT-13);

  if(list_orientation==Qt::Horizontal) {
    listpos=(e->pos().y()+list_position)/ENDPOINTLIST_ITEM_HEIGHT;
  }

  //
  // Get Endpoint
  //
  bool ok=false;
  if(listpos>=list_endpoints.size()) {
    return;
  }
  QString label=list_labels.value(list_endpoints.at(listpos));
  QStringList f0=label.split("-",QString::SkipEmptyParts);
  if(f0.size()<1) {
    return;
  }
  int endpt=f0.first().toInt(&ok);
  if(!ok) {
    endpt=-1;
  }

  if(endpt!=list_move_endpoint) {
    list_move_endpoint=endpt;
    emit hoveredEndpointChanged(list_router,endpt);
  }
}


void EndpointList::leaveEvent(QEvent *event)
{
  if(list_move_endpoint>=0) {
    list_move_endpoint=-1;
    emit hoveredEndpointChanged(list_router,list_move_endpoint);
  }
}


void EndpointList::paintEvent(QPaintEvent *e)
{
  QFontMetrics fm(font());
  int w=size().width();
  int text_y=(ENDPOINTLIST_ITEM_HEIGHT-fm.height())/2+fm.height();
  QPainter *p=new QPainter(this);
  int gpio_offset=0;

  if(list_show_gpio)  {
    gpio_offset=70;
  }
  p->setFont(font());
  p->setPen(Qt::black);
  p->setBrush(Qt::black);
  
  if(list_orientation==Qt::Vertical) {
    //
    // Vertical Orientation (Destinations, Outputs)
    //
    p->translate(w-(list_width+15+10),0);
    p->rotate(90.0);

    QMap<int,QString>::const_iterator it=list_labels.begin();
    for(int i=0;i<(ENDPOINTLIST_ITEM_HEIGHT*endpointQuantity());i+=ENDPOINTLIST_ITEM_HEIGHT) {
      if(it!=list_labels.end()) {
	p->drawLine(0,w-(ENDPOINTLIST_ITEM_HEIGHT+i)+list_position-(list_width+15+10),
		    0,w-i+list_position-(list_width+15+10));
	p->drawLine(0,w-i+list_position-(list_width+15+10),
		    list_width+15+gpio_offset,w-i+list_position-(list_width+15+10));
	p->drawText(((list_width+15-5)-fm.width(it.value())),w-(text_y+i+list_width+15)+list_position,
		    it.value());
	it++;
      }
    }
    p->drawLine(0,w-ENDPOINTLIST_ITEM_HEIGHT*endpointQuantity()+list_position-(list_width+15+10),
		list_width+15+gpio_offset,w-ENDPOINTLIST_ITEM_HEIGHT*endpointQuantity()+list_position-(list_width+15+10));
  }
  else {
    //
    // Horizontal Orientation (Sources, Inputs)
    //
    QMap<int,QString>::const_iterator it=list_labels.begin();
    for(int i=0;i<(ENDPOINTLIST_ITEM_HEIGHT*endpointQuantity());i+=ENDPOINTLIST_ITEM_HEIGHT) {
      if(it!=list_labels.end()) {
	p->drawLine(0,
		    ENDPOINTLIST_ITEM_HEIGHT+i-list_position,
		    0,
		    i-list_position);
	p->drawLine(0,
		    i-list_position,
		    w+gpio_offset,
		    i-list_position);
	p->drawText(w-fm.width(it.value())-gpio_offset-5,
		    text_y+i-list_position,
		    it.value());
	it++;
      }
    }
    p->drawLine(0,
		ENDPOINTLIST_ITEM_HEIGHT*endpointQuantity()-list_position,
		w,
		ENDPOINTLIST_ITEM_HEIGHT*endpointQuantity()-list_position);
  }
  delete p;

  resizeEvent(NULL);
}


void EndpointList::resizeEvent(QResizeEvent *e)
{
  int w=size().width();
  int h=size().height();
  int ypos=-list_position;

  if(list_orientation==Qt::Horizontal) {
    for(QMap<int,MultiStateWidget *>::const_iterator it=
	  list_gpio_widgets.begin();it!=list_gpio_widgets.end();it++) {
      it.value()->setGeometry(w-65,
			      ypos+4,
			      60,
			      18);
      ypos+=ENDPOINTLIST_ITEM_HEIGHT;
    }
  }

  if(list_orientation==Qt::Vertical) {
    for(QMap<int,MultiStateWidget *>::const_iterator it=
	  list_gpio_widgets.begin();it!=list_gpio_widgets.end();it++) {
      it.value()->setGeometry(ypos+4,
			      h-65,
			      18,
			      60);
      ypos+=ENDPOINTLIST_ITEM_HEIGHT;
    }
  }
}


int EndpointList::LocalEndpoint(QMouseEvent *e) const
{
  int slot=-1;
  int endpt=-1;

  if(e->button()==Qt::RightButton) {
    switch(list_orientation) {
    case Qt::Horizontal:
      slot=(e->pos().y()+list_position)/ENDPOINTLIST_ITEM_HEIGHT;
      if(slot>=list_endpoints.size()) {
	return -1;
      }
      endpt=list_endpoints.at(slot);
      break;

    case Qt::Vertical:
      slot=(e->pos().x()+list_position)/ENDPOINTLIST_ITEM_HEIGHT;
      endpt=list_endpoints.at(slot);
      break;
    }
    if((slot<0)||(slot>=list_labels.size())) {
      endpt=-1;
    }
  }
  else {
    endpt=-1;
  }

  return endpt;
}
