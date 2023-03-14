// vgpionode.cpp
//
// vgpionode(8) Drouter watchdog monitor
//
//   (C) Copyright 2023 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
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

#include <QCoreApplication>

#include "vgpionode.h"

VGpioNode::VGpioNode(int slot_quan,const QHostAddress &iface_addr,
		     const QHostAddress &iface_mask,QObject *parent)
  : QObject(parent)
{
  d_gpi_code="hhhhh";

  d_routing=new SyRouting(0,0,slot_quan,slot_quan);
  d_routing->setNicAddress(iface_addr);
  d_routing->setNicNetmask(iface_mask);
  d_routing->setClkAddress(QHostAddress(SWITCHYARD_CLOCK_ADDRESS));

  d_lwrp_server=new SyLwrpServer(d_routing);
  connect(d_lwrp_server,SIGNAL(gpiStateSet(int,const QString &)),
	  this,SLOT(gpiStateSetData(int,const QString &)));
  connect(d_lwrp_server,SIGNAL(gpoStateSet(int,const QString &)),
	  this,SLOT(gpoStateSetData(int,const QString &)));
  d_gpio_server=new SyGpioServer(d_routing,this);
  d_adv_server=new SyAdvServer(d_routing,false,this);
}


VGpioNode::~VGpioNode()
{
  delete d_adv_server;
  delete d_gpio_server;
  delete d_lwrp_server;
  delete d_routing;
}


void VGpioNode::setGpoMode(int slot,SyRouting::GpoMode gpo_mode)
{
  d_routing->setGpoMode(slot,gpo_mode);
}


void VGpioNode::setGpoAddress(int slot,const QHostAddress &saddr)
{
  d_routing->setGpoAddress(0,saddr);
}


void VGpioNode::setGpoAddress(int slot,int srca)
{
  d_routing->setGpoAddress(0,SyRouting::streamAddress(SyRouting::Stereo,srca));
}


void VGpioNode::gpiStateSetData(int slot,const QString &code)
{
  QString new_code;
  bool changed=false;

  for(int i=0;i<SWITCHYARD_GPIO_BUNDLE_SIZE;i++) {
    if(i<code.length()) {
      QChar c=code.at(i).toLower();
      if(c!=d_gpi_code.at(i)) {
	if(c==QChar('l')) {
	  new_code+='L';
	  d_gpio_server->sendGpi(31000,i,true,false);
	  changed=true;
	}
	else {
	  if(c==QChar('h')) {
	    new_code+='H';
	    d_gpio_server->sendGpi(31000,i,false,false);
	    changed=true;
	  }
	  else {
	    new_code+=d_gpi_code.at(i);
	  }
	}
      }
      else {
	new_code+=d_gpi_code.at(i);
      }
    }
    else {
      new_code+=d_gpi_code.at(i);
    }
  }
  if(changed) {
    d_lwrp_server->sendGpiState(0,new_code);
    d_gpi_code=new_code.toLower();
  }
}


void VGpioNode::gpoStateSetData(int slot,const QString &code)
{
}
