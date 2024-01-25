// matrix.h
//
// Abstract router matrix implementation
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

#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>

#include <vector>

#include <QHostAddress>
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

#include <sy5/sylwrp_client.h>

#include "drconfig.h"

class Matrix :public QObject
{
  Q_OBJECT;
 public:
  Matrix(DRConfig::MatrixType type,unsigned id,DRConfig *conf,QObject *parent=0);
  ~Matrix();
  DRConfig::MatrixType matrixType() const;
  unsigned id() const;
  virtual bool isConnected() const=0;
  virtual QHostAddress hostAddress() const=0;
  virtual QString hostName() const=0;
  virtual QString deviceName() const=0;
  virtual QString description() const=0;
  virtual unsigned dstSlots() const;
  virtual unsigned srcSlots() const;
  virtual SySource *src(int slot) const;
  virtual SyDestination *dst(int slot) const;
  virtual int srcNumber(int slot) const;
  virtual QHostAddress srcAddress(int slot) const;
  virtual QString srcName(int slot) const;
  virtual bool srcEnabled(int slot) const;
  virtual unsigned srcChannels(int slot) const;
  virtual unsigned srcPacketSize(int slot);
  virtual QHostAddress dstAddress(int slot) const;
  virtual void setDstAddress(int slot,const QHostAddress &addr);
  virtual void setDstAddress(int slot,const QString &addr);
  virtual QString dstName(int slot) const;
  virtual unsigned dstChannels(int slot) const;
  virtual unsigned gpis() const;
  virtual unsigned gpos() const;
  virtual SyGpioBundle *gpiBundle(int slot) const;
  virtual void setGpiCode(int slot,const QString &code);
  virtual SyGpo *gpo(int slot) const;
  virtual void setGpoCode(int slot,const QString &code);
  virtual void setGpoSourceAddress(int slot,const QHostAddress &s_addr,
				   int s_slot);
  virtual bool clipAlarmActive(int slot,SyLwrpClient::MeterType type,
			       int chan) const;
  virtual bool silenceAlarmActive(int slot,SyLwrpClient::MeterType type,
				  int chan) const;
  virtual void setClipMonitor(int slot,SyLwrpClient::MeterType type,int lvl,
			      int msec);
  virtual void setSilenceMonitor(int slot,SyLwrpClient::MeterType type,int lvl,
				 int msec);
  virtual void connectToHost(const QHostAddress &addr,uint16_t port,
  			     const QString &pwd,bool persistent=false)=0;
  virtual void sendRawLwrp(const QString &cmd);

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
  void audioClipAlarm(unsigned id,SyLwrpClient::MeterType type,
		      unsigned slotnum,int chan,bool state);
  void audioSilenceAlarm(unsigned id,SyLwrpClient::MeterType type,
			 unsigned slotnum,int chan,bool state);

 protected:
  DRConfig *config() const;

 private:
  DRConfig::MatrixType d_matrix_type;
  unsigned d_id;
  DRConfig *d_config;
};


#endif  // MATRIX_H
