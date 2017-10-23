// protocol.cpp
//
// Abstract base class for drouter client protocols
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#include "protocol.h"

Protocol::Protocol(DRouter *router,Protocol::Type type,QObject *parent)
  : QObject(parent)
{
  protocol_type=type;
  protocol_drouter=router;

  connect(router,SIGNAL(nodeAdded(const SyNode &)),
	  this,SLOT(nodeAddedData(const SyNode &)));
  connect(router,SIGNAL(nodeAboutToBeRemoved(const SyNode &)),
	  this,SLOT(nodeAboutToBeRemovedData(const SyNode &)));
  connect(router,SIGNAL(srcChanged(const SyNode &,int,const SySource &)),
	  this,SLOT(srcChangedData(const SyNode &,int,const SySource &)));
  connect(router,SIGNAL(dstChanged(const SyNode &,int,const SyDestination &)),
	  this,SLOT(dstChangedData(const SyNode &,int,const SyDestination &)));
  connect(router,SIGNAL(gpiChanged(const SyNode &,int,const SyGpioBundle &)),
	  this,SLOT(gpiChangedData(const SyNode &,int,const SyGpioBundle &)));
  connect(router,SIGNAL(gpoChanged(const SyNode &,int,const SyGpo &)),
	  this,SLOT(gpoChangedData(const SyNode &,int,const SyGpo &)));
  connect(router,SIGNAL(clipAlarmChanged(const SyNode &,int,
					 SyLwrpClient::MeterType,int,bool)),
	  this,SLOT(clipAlarmChangedData(const SyNode &,int,
					 SyLwrpClient::MeterType,int,bool)));
  connect(router,SIGNAL(silenceAlarmChanged(const SyNode &,int,
					    SyLwrpClient::MeterType,int,bool)),
	  this,SLOT(silenceAlarmChangedData(const SyNode &,int,
					    SyLwrpClient::MeterType,int,bool)));
}


Protocol::Type Protocol::type() const
{
  return protocol_type;
}


void Protocol::reload()
{
}


void Protocol::nodeAddedData(const SyNode &node)
{
  processAddedNode(node);
}


void Protocol::nodeAboutToBeRemovedData(const SyNode &node)
{
  processAboutToBeRemovedNode(node);
}


void Protocol::srcChangedData(const SyNode &node,int slot,const SySource &src)
{
  processChangedSource(node,slot,src);
}


void Protocol::dstChangedData(const SyNode &node,int slot,
			      const SyDestination &dst)
{
  processChangedDestination(node,slot,dst);
}


void Protocol::gpiChangedData(const SyNode &node,int slot,
			      const SyGpioBundle &gpi)
{
  processChangedGpi(node,slot,gpi);
}


void Protocol::gpoChangedData(const SyNode &node,int slot,const SyGpo &gpo)
{
  processChangedGpo(node,slot,gpo);
}


void Protocol::clipAlarmChangedData(const SyNode &node,int slot,
				    SyLwrpClient::MeterType type,int chan,
				    bool state)
{
  processClipAlarm(node,slot,type,chan,state);
}


void Protocol::silenceAlarmChangedData(const SyNode &node,int slot,
				       SyLwrpClient::MeterType type,int chan,
				       bool state)
{
  processSilenceAlarm(node,slot,type,chan,state);
}


void Protocol::processAddedNode(const SyNode &node)
{
}


void Protocol::processAboutToBeRemovedNode(const SyNode &node)
{
}


void Protocol::processChangedSource(const SyNode &node,int slot,
				    const SySource &src)
{
}


void Protocol::processChangedDestination(const SyNode &node,int slot,
					 const SyDestination &dst)
{
}


void Protocol::processClipAlarm(const SyNode &node,
				int slot,SyLwrpClient::MeterType type,
				int chan,bool state)
{
  /*
  printf("processClipAlarm(%s,%d,%d,%d,%d)\n",
	 (const char *)node.hostAddress().toString().toUtf8(),
	 slot,type,chan,state);
  */
}


void Protocol::processSilenceAlarm(const SyNode &node,
				   int slot,SyLwrpClient::MeterType type,
				   int chan,bool state)
{
  /*
  printf("processSilenceAlarm(%s,%d,%d,%d,%d)\n",
	 (const char *)node.hostAddress().toString().toUtf8(),
	 slot,type,chan,state);
  */
}


void Protocol::processChangedGpi(const SyNode &node,int slot,
				 const SyGpioBundle &gpi)
{
}


void Protocol::processChangedGpo(const SyNode &node,int slot,const SyGpo &gpo)
{
}


DRouter *Protocol::router() const
{
  return protocol_drouter;
}
