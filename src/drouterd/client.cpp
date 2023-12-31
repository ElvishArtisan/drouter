// client.cpp
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

#include "client.h"

Client::Client(QObject *parent)
  : QObject(parent)
{
}


Client::~Client()
{
}


QString Client::typeString(Client::Type type)
{
  QString ret="unknown";

  switch(type) {
  case Client::LwrpClient:
    ret="lwrp";
    break;

  case Client::LastClient:
    break;
  }

  return ret;
}
