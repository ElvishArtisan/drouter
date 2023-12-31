// client_lwrp.h
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

#ifndef CLIENT_LWRP_H
#define CLIENT_LWRP_H

#include <sy5/sylwrp_client.h>

#include "client.h"

class ClientLwrp :public Client
{
  Q_OBJECT;
 public:
  ClientLwrp(unsigned id,QObject *parent=0);
  ~ClientLwrp();
  bool isConnected() const;
  QHostAddress hostAddress() const;
  QString deviceName() const;
  // QString productName() const;
  unsigned dstSlots() const;
  unsigned srcSlots() const;
  SySource *src(int slot) const;
  SyDestination *dst(int slot) const;
  unsigned gpis() const;
  unsigned gpos() const;
  QString hostName() const;
  // QHostAddress hostAddress() const;
  // uint16_t port() const;
  int srcNumber(int slot) const;
  QHostAddress srcAddress(int slot) const;
  // void setSrcAddress(int slot,const QHostAddress &addr);
  // void setSrcAddress(int slot,const QString &addr);
  QString srcName(int slot) const;
  // void setSrcName(int slot,const QString &str);
  // QString srcLabel(int slot) const;
  // void setSrcLabel(int slot,const QString &str);
  bool srcEnabled(int slot) const;
  // void setSrcEnabled(int slot,bool state);
  unsigned srcChannels(int slot) const;
  // void setSrcChannels(int slot,unsigned chans);
  unsigned srcPacketSize(int slot);
  // void setSrcPacketSize(int slot,unsigned size);
  // bool srcShareable(int slot) const;
  // void setSrcShareable(int slot,bool state);
  // int srcMeterLevel(int slot,int chan) const;
  QHostAddress dstAddress(int slot) const;
  void setDstAddress(int slot,const QHostAddress &addr);
  void setDstAddress(int slot,const QString &addr);
  QString dstName(int slot) const;
  // void setDstName(int slot,const QString &str);
  unsigned dstChannels(int slot) const;
  // void setDstChannels(int slot,unsigned chans);
  // int dstMeterLevel(int slot,int chan) const;
  SyGpioBundle *gpiBundle(int slot) const;
  void setGpiCode(int slot,const QString &code);
  SyGpo *gpo(int slot) const;
  void setGpoCode(int slot,const QString &code);
  // void setGpoName(int slot,const QString &str);
  void setGpoSourceAddress(int slot,const QHostAddress &s_addr,
			   int s_slot);
  // void setGpoFollow(int slot,bool state);
  bool clipAlarmActive(int slot,SyLwrpClient::MeterType type,int chan) const;
  bool silenceAlarmActive(int slot,SyLwrpClient::MeterType type,int chan) const;
  void setClipMonitor(int slot,SyLwrpClient::MeterType type,int lvl,int msec);
  void setSilenceMonitor(int slot,SyLwrpClient::MeterType type,int lvl,
			 int msec);
  // void startMeter(SyLwrpClient::MeterType type);
  // void stopMeter(SyLwrpClient::MeterType type);
  // QHostAddress nicAddress() const;
  // void setNicAddress(const QHostAddress &addr);
  void connectToHost(const QHostAddress &addr,uint16_t port,
		     const QString &pwd,bool persistent=false);
  // int timeoutInterval() const;
  // void setTimeoutInterval(int msec);
  void sendRawLwrp(const QString &cmd);
  // void close();

 private slots:
  void nodeConnectedData(unsigned id,bool state);
  void sourceChangedData(unsigned id,int slotnum,const SyNode &node,
			 const SySource &src);
  void destinationChangedData(unsigned id,int slotnum,const SyNode &node,
			      const SyDestination &dst);
  void gpiChangedData(unsigned id,int slotnum,const SyNode &node,
		      const SyGpioBundle &gpi);
  void gpoChangedData(unsigned id,int slotnum,const SyNode &node,
		      const SyGpo &gpo);
  void audioClipAlarmData(unsigned id,SyLwrpClient::MeterType type,
			  unsigned slotnum,int chan,bool state);
  void audioSilenceAlarmData(unsigned id,SyLwrpClient::MeterType type,
			     unsigned slotnum,int chan,bool state);

 private:
  SyLwrpClient *d_lwrp_client;
};


#endif  // CLIENT_LWRP_H
