// protocolfactory.cpp
//
// Create a protocol instance.
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

#include "protocol_d.h"
#include "protocol_sa.h"
#include "protocolfactory.h"

Protocol *ProtocolFactory(DRouter *router,Protocol::Type type,int sock,
			  QObject *parent)
{
  Protocol *p=NULL;

  switch(type) {
  case Protocol::ProtocolD:
    p=new ProtocolD(router,sock,parent);
    break;

  case Protocol::ProtocolSa:
    p=new ProtocolSa(router,sock,parent);
    break;
  }

  return p;
}
