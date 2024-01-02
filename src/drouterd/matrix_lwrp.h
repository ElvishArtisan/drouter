// matrix_lwrp.h
//
// LWRP matrix implementation
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

#ifndef MATRIX_LWRP_H
#define MATRIX_LWRP_H

#include <sy5/sylwrp_client.h>

#include "matrix.h"

class MatrixLwrp :public Matrix
{
  Q_OBJECT;
 public:
  MatrixLwrp(unsigned id,Config *conf,QObject *parent=0);
  ~MatrixLwrp();
  bool isConnected() const;
  QHostAddress hostAddress() const;
  QString hostName() const;
  QString deviceName() const;
  unsigned dstSlots() const;
  unsigned srcSlots() const;
  SySource *src(int slot) const;
  SyDestination *dst(int slot) const;
  unsigned gpis() const;
  unsigned gpos() const;
  int srcNumber(int slot) const;
  QHostAddress srcAddress(int slot) const;
  QString srcName(int slot) const;
  bool srcEnabled(int slot) const;
  unsigned srcChannels(int slot) const;
  unsigned srcPacketSize(int slot);
  QHostAddress dstAddress(int slot) const;
  void setDstAddress(int slot,const QHostAddress &addr);
  void setDstAddress(int slot,const QString &addr);
  QString dstName(int slot) const;
  unsigned dstChannels(int slot) const;
  SyGpioBundle *gpiBundle(int slot) const;
  void setGpiCode(int slot,const QString &code);
  SyGpo *gpo(int slot) const;
  void setGpoCode(int slot,const QString &code);
  void setGpoSourceAddress(int slot,const QHostAddress &s_addr,
			   int s_slot);
  bool clipAlarmActive(int slot,SyLwrpClient::MeterType type,int chan) const;
  bool silenceAlarmActive(int slot,SyLwrpClient::MeterType type,int chan) const;
  void setClipMonitor(int slot,SyLwrpClient::MeterType type,int lvl,int msec);
  void setSilenceMonitor(int slot,SyLwrpClient::MeterType type,int lvl,
			 int msec);
  void connectToHost(const QHostAddress &addr,uint16_t port,
		     const QString &pwd,bool persistent=false);
  void sendRawLwrp(const QString &cmd);

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


#endif  // MATRIX_LWRP_H
