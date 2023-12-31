// client.h
//
// Abstract router client implementation
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

#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>

#include <vector>

#include <QHostAddress>
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

#include <sy5/sylwrp_client.h>

class Client :public QObject
{
  Q_OBJECT;
 public:
  enum Type {LwrpClient=0,LastClient=1};
  Client(QObject *parent=0);
  ~Client();
  virtual bool isConnected() const=0;
  virtual QHostAddress hostAddress() const=0;
  virtual QString deviceName() const=0;
  // virtual QString productName() const=0;
  virtual unsigned dstSlots() const=0;
  virtual unsigned srcSlots() const=0;
  virtual SySource *src(int slot) const=0;
  virtual SyDestination *dst(int slot) const=0;
  virtual unsigned gpis() const=0;
  virtual unsigned gpos() const=0;
  virtual QString hostName() const=0;
  // virtual QHostAddress hostAddress() const=0;
  // virtual uint16_t port() const=0;
  virtual int srcNumber(int slot) const=0;
  virtual QHostAddress srcAddress(int slot) const=0;
  // virtual void setSrcAddress(int slot,const QHostAddress &addr)=0;
  // virtual void setSrcAddress(int slot,const QString &addr)=0;
  virtual QString srcName(int slot) const=0;
  // virtual void setSrcName(int slot,const QString &str)=0;
  // virtual QString srcLabel(int slot) const=0;
  // virtual void setSrcLabel(int slot,const QString &str)=0;
  virtual bool srcEnabled(int slot) const=0;
  // virtual void setSrcEnabled(int slot,bool state)=0;
  virtual unsigned srcChannels(int slot) const=0;
  // virtual void setSrcChannels(int slot,unsigned chans)=0;
  virtual unsigned srcPacketSize(int slot)=0;
  // virtual void setSrcPacketSize(int slot,unsigned size)=0;
  // virtual bool srcShareable(int slot) const=0;
  // virtual void setSrcShareable(int slot,bool state)=0;
  // virtual int srcMeterLevel(int slot,int chan) const=0;
  virtual QHostAddress dstAddress(int slot) const=0;
  virtual void setDstAddress(int slot,const QHostAddress &addr)=0;
  virtual void setDstAddress(int slot,const QString &addr)=0;
  virtual QString dstName(int slot) const=0;
  // virtual void setDstName(int slot,const QString &str)=0;
  virtual unsigned dstChannels(int slot) const=0;
  //virtual void setDstChannels(int slot,unsigned chans)=0;
  // virtual int dstMeterLevel(int slot,int chan) const=0;
  virtual SyGpioBundle *gpiBundle(int slot) const=0;
  virtual void setGpiCode(int slot,const QString &code)=0;
  virtual SyGpo *gpo(int slot) const=0;
  virtual void setGpoCode(int slot,const QString &code)=0;
  // virtual void setGpoName(int slot,const QString &str)=0;
  virtual void setGpoSourceAddress(int slot,const QHostAddress &s_addr,
				   int s_slot)=0;
  // virtual void setGpoFollow(int slot,bool state)=0;
  virtual bool clipAlarmActive(int slot,SyLwrpClient::MeterType type,
			       int chan) const=0;
  virtual bool silenceAlarmActive(int slot,SyLwrpClient::MeterType type,
				  int chan) const=0;
  virtual void setClipMonitor(int slot,SyLwrpClient::MeterType type,int lvl,
			      int msec)=0;
  virtual void setSilenceMonitor(int slot,SyLwrpClient::MeterType type,int lvl,
				 int msec)=0;
  // virtual void startMeter(SyLwrpClient::MeterType type)=0;
  // virtual void stopMeter(SyLwrpClient::MeterType type)=0;
  // virtual QHostAddress nicAddress() const=0;
  // virtual void setNicAddress(const QHostAddress &addr)=0;
  virtual void connectToHost(const QHostAddress &addr,uint16_t port,
  			     const QString &pwd,bool persistent=false)=0;
  // virtual int timeoutInterval() const=0;
  // virtual void setTimeoutInterval(int msec)=0;
  virtual void sendRawLwrp(const QString &cmd)=0;
  // virtual void close()=0;
  static QString typeString(Type type);

 signals:
  void connected(unsigned id,bool state);
  void connectionError(unsigned id,QAbstractSocket::SocketError err);
  void sourceChanged(unsigned id,int slotnum,const SyNode &node,
		     const SySource &src);
  void destinationChanged(unsigned id,int slotnum,const SyNode &node,
			  const SyDestination &dst);
  void gpiChanged(unsigned id,int slotnum,const SyNode &node,
		  const SyGpioBundle &bundle);
  void gpoChanged(unsigned id,int slotnum,const SyNode &node,const SyGpo &gpo);
  void nicAddressChanged(unsigned id,const QHostAddress &nicaddr);
  void meterUpdate(unsigned id,SyLwrpClient::MeterType type,unsigned slotnum,
		   int16_t *peak_lvls,int16_t *rms_lvls);
  void audioClipAlarm(unsigned id,SyLwrpClient::MeterType type,
		      unsigned slotnum,int chan,bool state);
  void audioSilenceAlarm(unsigned id,SyLwrpClient::MeterType type,
			 unsigned slotnum,int chan,bool state);
};


#endif  // SYLWRP_CLIENT_H
