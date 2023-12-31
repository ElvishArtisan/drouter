// client_factory.cpp
//
// Instantiate a client instance
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

#include "client_factory.h"

Client *ClientFactory(Client::Type type,unsigned id,QObject *parent)
{
  Client *client=NULL;

  switch(type) {
  case Client::LwrpClient:
    client=new ClientLwrp(id,parent);
    break;

  case Client::LastClient:
    break;
  }
  return client;
}
