// matrix_lwrp.cpp
//
// LWRP matrix implementation
//
// (C) 2023-2024 Fred Gleason <fredg@paravelsystems.com>
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

#include "matrix_lwrp.h"

MatrixLwrp::MatrixLwrp(unsigned id,Config *conf,QObject *parent)
  : Matrix(Config::LwrpMatrix,id,conf,parent)
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


MatrixLwrp::~MatrixLwrp()
{
  delete d_lwrp_client;
}


bool MatrixLwrp::isConnected() const
{
  return d_lwrp_client->isConnected();
}


QHostAddress MatrixLwrp::hostAddress() const
{
  return d_lwrp_client->hostAddress();
}


QString MatrixLwrp::hostName() const
{
  return d_lwrp_client->hostName();
}


QString MatrixLwrp::deviceName() const
{
  return d_lwrp_client->deviceName();
}


QString MatrixLwrp::description() const
{
  return d_description;
}


unsigned MatrixLwrp::dstSlots() const
{
  return d_lwrp_client->dstSlots();
}


unsigned MatrixLwrp::srcSlots() const
{
  return d_lwrp_client->srcSlots();
}


SySource *MatrixLwrp::src(int slot) const
{
  return d_lwrp_client->src(slot);
}


SyDestination *MatrixLwrp::dst(int slot) const
{
  return d_lwrp_client->dst(slot);
}


unsigned MatrixLwrp::gpis() const
{
  return d_lwrp_client->gpis();
}


unsigned MatrixLwrp::gpos() const
{
  return d_lwrp_client->gpos();
}


int MatrixLwrp::srcNumber(int slot) const
{
  return d_lwrp_client->srcNumber(slot);
}


QHostAddress MatrixLwrp::srcAddress(int slot) const
{
  return d_lwrp_client->srcAddress(slot);
}


QString MatrixLwrp::srcName(int slot) const
{
  return d_lwrp_client->srcName(slot);
}


bool MatrixLwrp::srcEnabled(int slot) const
{
  return d_lwrp_client->srcEnabled(slot);
}


unsigned MatrixLwrp::srcChannels(int slot) const
{
  return d_lwrp_client->srcChannels(slot);
}


unsigned MatrixLwrp::srcPacketSize(int slot)
{
  return d_lwrp_client->srcPacketSize(slot);
}


QHostAddress MatrixLwrp::dstAddress(int slot) const
{
  return d_lwrp_client->dstAddress(slot);
}


void MatrixLwrp::setDstAddress(int slot,const QHostAddress &addr)
{
  d_lwrp_client->setDstAddress(slot,addr);
}


void MatrixLwrp::setDstAddress(int slot,const QString &addr)
{
  d_lwrp_client->setDstAddress(slot,addr);
}


QString MatrixLwrp::dstName(int slot) const
{
  return d_lwrp_client->dstName(slot);
}


unsigned MatrixLwrp::dstChannels(int slot) const
{
  return d_lwrp_client->dstChannels(slot);
}


SyGpioBundle *MatrixLwrp::gpiBundle(int slot) const
{
  return d_lwrp_client->gpiBundle(slot);
}


void MatrixLwrp::setGpiCode(int slot,const QString &code,int duration)
{
  d_lwrp_client->setGpiCode(slot,code,duration);
}


SyGpo *MatrixLwrp::gpo(int slot) const
{
  return d_lwrp_client->gpo(slot);
}


void MatrixLwrp::setGpoCode(int slot,const QString &code,int duration)
{
  d_lwrp_client->setGpoCode(slot,code,duration);
}


void MatrixLwrp::setGpoSourceAddress(int slot,const QHostAddress &s_addr,
				     int s_slot)
{
  d_lwrp_client->setGpoSourceAddress(slot,s_addr,s_slot);
}


bool MatrixLwrp::clipAlarmActive(int slot,SyLwrpClient::MeterType type,
				 int chan) const
{
  return d_lwrp_client->clipAlarmActive(slot,type,chan);
}


bool MatrixLwrp::silenceAlarmActive(int slot,SyLwrpClient::MeterType type,
				    int chan) const
{
  return d_lwrp_client->silenceAlarmActive(slot,type,chan);
}


void MatrixLwrp::setClipMonitor(int slot,SyLwrpClient::MeterType type,int lvl,
				int msec)
{
  d_lwrp_client->setClipMonitor(slot,type,lvl,msec);
}


void MatrixLwrp::setSilenceMonitor(int slot,SyLwrpClient::MeterType type,
				   int lvl,int msec)
{
  d_lwrp_client->setSilenceMonitor(slot,type,lvl,msec);
}


void MatrixLwrp::connectToHost(const QHostAddress &addr,uint16_t port,
			       const QString &pwd,bool persistent)
{
  d_lwrp_client->connectToHost(addr,port,pwd,persistent);
}


void MatrixLwrp::sendRawLwrp(const QString &cmd)
{
  return d_lwrp_client->sendRawLwrp(cmd);
}


void MatrixLwrp::nodeConnectedData(unsigned id,bool state)
{
  d_description=SyNode::productName(deviceName(),gpis(),gpos());

  emit connected(id,state);
}


void MatrixLwrp::sourceChangedData(unsigned id,int slotnum,const SyNode &node,
				   const SySource &src)
{
  emit sourceChanged(id,slotnum,node,src);
}


void MatrixLwrp::destinationChangedData(unsigned id,int slotnum,
					const SyNode &node,
					const SyDestination &dst)
{
  emit destinationChanged(id,slotnum,node,dst);
}


void MatrixLwrp::gpiChangedData(unsigned id,int slotnum,const SyNode &node,
				const SyGpioBundle &gpi)
{
  emit gpiChanged(id,slotnum,node,gpi);
}


void MatrixLwrp::gpoChangedData(unsigned id,int slotnum,const SyNode &node,
				const SyGpo &gpo)
{
  emit gpoChanged(id,slotnum,node,gpo);
}


void MatrixLwrp::audioClipAlarmData(unsigned id,SyLwrpClient::MeterType type,
				    unsigned slotnum,int chan,bool state)
{
  emit audioClipAlarm(id,type,slotnum,chan,state);
}


void MatrixLwrp::audioSilenceAlarmData(unsigned id,SyLwrpClient::MeterType type,
				       unsigned slotnum,int chan,bool state)
{
  emit audioSilenceAlarm(id,type,slotnum,chan,state);
}
