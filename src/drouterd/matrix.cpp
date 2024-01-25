// matrix.cpp
//
// Abstract router matrix implementation
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

#include "matrix.h"

Matrix::Matrix(DRConfig::MatrixType matrix_type,unsigned id,DRConfig *conf,
	       QObject *parent)
  : QObject(parent)
{
  d_matrix_type=matrix_type;
  d_id=id;
  d_config=conf;
}


unsigned Matrix::id() const
{
  return d_id;
}


Matrix::~Matrix()
{
}


DRConfig::MatrixType Matrix::matrixType() const
{
  return d_matrix_type;
}


unsigned Matrix::dstSlots() const
{
  return 0;
}


unsigned Matrix::srcSlots() const
{
  return 0;
}


SySource *Matrix::src(int slot) const
{
  return NULL;
}


SyDestination *Matrix::dst(int slot) const
{
  return NULL;
}


int Matrix::srcNumber(int slot) const
{
  return 0;
}


QHostAddress Matrix::srcAddress(int slot) const
{
  return QHostAddress();
}


QString Matrix::srcName(int slot) const
{
  return QString();
}


bool Matrix::srcEnabled(int slot) const
{
  return false;
}


unsigned Matrix::srcChannels(int slot) const
{
  return 0;
}


unsigned Matrix::srcPacketSize(int slot)
{
  return 0;
}


QHostAddress Matrix::dstAddress(int slot) const
{
  return QHostAddress();
}


void Matrix::setDstAddress(int slot,const QHostAddress &addr)
{
}


void Matrix::setDstAddress(int slot,const QString &addr)
{
}


QString Matrix::dstName(int slot) const
{
  return QString();
}


unsigned Matrix::dstChannels(int slot) const
{
  return 0;
}


unsigned Matrix::gpis() const
{
  return 0;
}


unsigned Matrix::gpos() const
{
  return 0;
}


SyGpioBundle *Matrix::gpiBundle(int slot) const
{
  return NULL;
}


void Matrix::setGpiCode(int slot,const QString &code)
{
}


SyGpo *Matrix::gpo(int slot) const
{
  return NULL;
}


void Matrix::setGpoCode(int slot,const QString &code)
{
}


void Matrix::setGpoSourceAddress(int slot,const QHostAddress &s_addr,
				   int s_slot)
{
}


bool Matrix::clipAlarmActive(int slot,SyLwrpClient::MeterType type,
			     int chan) const
{
  return false;
}


bool Matrix::silenceAlarmActive(int slot,SyLwrpClient::MeterType type,
				int chan) const
{
  return false;
}


void Matrix::setClipMonitor(int slot,SyLwrpClient::MeterType type,int lvl,
			    int msec)
{
}


void Matrix::setSilenceMonitor(int slot,SyLwrpClient::MeterType type,int lvl,
			       int msec)
{
}


DRConfig *Matrix::config() const
{
  return d_config;
}


void Matrix::sendRawLwrp(const QString &cmd)
{
}
