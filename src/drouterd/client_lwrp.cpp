// client_lwrp.cpp
//
// LWRP client implementation
//
// (C) 2023 Fred Gleason <fredg@paravelsystems.com>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of version 2.1 of the GNU Lesser General Public
//    License as published by the Free Software Foundation;
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, 
//    Boston, MA  02111-1307  USA
//

#include "client_lwrp.h"

ClientLwrp::ClientLwrp(unsigned id,QObject *parent)
  : Client(parent)
{
  d_lwrp_client=new SyLwrpClient(id,this);
  connect(d_lwrp_client,SIGNAL(connected(unsigned,bool)),
	  this,SLOT(nodeConnectedData(unsigned,bool)));
  connect(d_lwrp_client,
	  SIGNAL(sourceChanged(unsigned,int,const SyNode,const SySource &)),
	  this,SLOT(sourceChangedData(unsigned,int,const SyNode,
				      const SySource &)));
  connect(d_lwrp_client,SIGNAL(destinationChanged(unsigned,int,const SyNode &,
						  const SyDestination &)),
	  this,SLOT(destinationChangedData(unsigned,int,const SyNode &,
					   const SyDestination &)));
  connect(d_lwrp_client,SIGNAL(gpiChanged(unsigned,int,const SyNode &,
					  const SyGpioBundle &)),
	  this,SLOT(gpiChangedData(unsigned,int,const SyNode &,
				   const SyGpioBundle &)));
  connect(d_lwrp_client,
	  SIGNAL(gpoChanged(unsigned,int,const SyNode &,const SyGpo &)),
	  this,
	  SLOT(gpoChangedData(unsigned,int,const SyNode &,const SyGpo &)));
  connect(d_lwrp_client,SIGNAL(audioClipAlarm(unsigned,SyLwrpClient::MeterType,
					      unsigned,int,bool)),
	  this,SLOT(audioClipAlarmData(unsigned,SyLwrpClient::MeterType,
				       unsigned,int,bool)));
  connect(d_lwrp_client,
	  SIGNAL(audioSilenceAlarm(unsigned,SyLwrpClient::MeterType,
				   unsigned,int,bool)),
	  this,SLOT(audioSilenceAlarmData(unsigned,SyLwrpClient::MeterType,
					  unsigned,int,bool)));
}


ClientLwrp::~ClientLwrp()
{
  delete d_lwrp_client;
}


bool ClientLwrp::isConnected() const
{
  return d_lwrp_client->isConnected();
}


QHostAddress ClientLwrp::hostAddress() const
{
  return d_lwrp_client->hostAddress();
}


QString ClientLwrp::deviceName() const
{
  return d_lwrp_client->deviceName();
}


unsigned ClientLwrp::dstSlots() const
{
  return d_lwrp_client->dstSlots();
}


unsigned ClientLwrp::srcSlots() const
{
  return d_lwrp_client->srcSlots();
}


SySource *ClientLwrp::src(int slot) const
{
  return d_lwrp_client->src(slot);
}


SyDestination *ClientLwrp::dst(int slot) const
{
  return d_lwrp_client->dst(slot);
}


unsigned ClientLwrp::gpis() const
{
  return d_lwrp_client->gpis();
}


unsigned ClientLwrp::gpos() const
{
  return d_lwrp_client->gpos();
}


QString ClientLwrp::hostName() const
{
  return d_lwrp_client->hostName();
}


int ClientLwrp::srcNumber(int slot) const
{
  return d_lwrp_client->srcNumber(slot);
}


QHostAddress ClientLwrp::srcAddress(int slot) const
{
  return d_lwrp_client->srcAddress(slot);
}


QString ClientLwrp::srcName(int slot) const
{
  return d_lwrp_client->srcName(slot);
}


bool ClientLwrp::srcEnabled(int slot) const
{
  return d_lwrp_client->srcEnabled(slot);
}


unsigned ClientLwrp::srcChannels(int slot) const
{
  return d_lwrp_client->srcChannels(slot);
}


unsigned ClientLwrp::srcPacketSize(int slot)
{
  return d_lwrp_client->srcPacketSize(slot);
}


QHostAddress ClientLwrp::dstAddress(int slot) const
{
  return d_lwrp_client->dstAddress(slot);
}


void ClientLwrp::setDstAddress(int slot,const QHostAddress &addr)
{
  d_lwrp_client->setDstAddress(slot,addr);
}


void ClientLwrp::setDstAddress(int slot,const QString &addr)
{
  d_lwrp_client->setDstAddress(slot,addr);
}


QString ClientLwrp::dstName(int slot) const
{
  return d_lwrp_client->dstName(slot);
}


unsigned ClientLwrp::dstChannels(int slot) const
{
  return d_lwrp_client->dstChannels(slot);
}


SyGpioBundle *ClientLwrp::gpiBundle(int slot) const
{
  return d_lwrp_client->gpiBundle(slot);
}


void ClientLwrp::setGpiCode(int slot,const QString &code)
{
  d_lwrp_client->setGpiCode(slot,code);
}


SyGpo *ClientLwrp::gpo(int slot) const
{
  return d_lwrp_client->gpo(slot);
}


void ClientLwrp::setGpoCode(int slot,const QString &code)
{
  d_lwrp_client->setGpoCode(slot,code);
}


void ClientLwrp::setGpoSourceAddress(int slot,const QHostAddress &s_addr,
				     int s_slot)
{
  d_lwrp_client->setGpoSourceAddress(slot,s_addr,s_slot);
}


bool ClientLwrp::clipAlarmActive(int slot,SyLwrpClient::MeterType type,
				 int chan) const
{
  return d_lwrp_client->clipAlarmActive(slot,type,chan);
}


bool ClientLwrp::silenceAlarmActive(int slot,SyLwrpClient::MeterType type,
				    int chan) const
{
  return d_lwrp_client->silenceAlarmActive(slot,type,chan);
}


void ClientLwrp::setClipMonitor(int slot,SyLwrpClient::MeterType type,int lvl,
				int msec)
{
  d_lwrp_client->setClipMonitor(slot,type,lvl,msec);
}


void ClientLwrp::setSilenceMonitor(int slot,SyLwrpClient::MeterType type,
				   int lvl,int msec)
{
  d_lwrp_client->setSilenceMonitor(slot,type,lvl,msec);
}


void ClientLwrp::connectToHost(const QHostAddress &addr,uint16_t port,
			       const QString &pwd,bool persistent)
{
  d_lwrp_client->connectToHost(addr,port,pwd,persistent);
}


void ClientLwrp::sendRawLwrp(const QString &cmd)
{
  return d_lwrp_client->sendRawLwrp(cmd);
}


void ClientLwrp::nodeConnectedData(unsigned id,bool state)
{
  emit connected(id,state);
}


void ClientLwrp::sourceChangedData(unsigned id,int slotnum,const SyNode &node,
				   const SySource &src)
{
  emit sourceChanged(id,slotnum,node,src);
}


void ClientLwrp::destinationChangedData(unsigned id,int slotnum,
					const SyNode &node,
					const SyDestination &dst)
{
  emit destinationChanged(id,slotnum,node,dst);
}


void ClientLwrp::gpiChangedData(unsigned id,int slotnum,const SyNode &node,
				const SyGpioBundle &gpi)
{
  emit gpiChanged(id,slotnum,node,gpi);
}


void ClientLwrp::gpoChangedData(unsigned id,int slotnum,const SyNode &node,
				const SyGpo &gpo)
{
  emit gpoChanged(id,slotnum,node,gpo);
}


void ClientLwrp::audioClipAlarmData(unsigned id,SyLwrpClient::MeterType type,
				    unsigned slotnum,int chan,bool state)
{
  emit audioClipAlarm(id,type,slotnum,chan,state);
}


void ClientLwrp::audioSilenceAlarmData(unsigned id,SyLwrpClient::MeterType type,
				       unsigned slotnum,int chan,bool state)
{
  emit audioSilenceAlarm(id,type,slotnum,chan,state);
}
