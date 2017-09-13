// protocol.cpp
//
// Abstract base class for drouter client protocols
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

#include "protocol.h"

Protocol::Protocol(DRouter *router,Protocol::Type type,QObject *parent)
  : QObject(parent)
{
  protocol_type=type;
  protocol_drouter=router;

  connect(router,SIGNAL(nodeAdded(const SyNode &)),
	  this,SLOT(nodeAddedData(const SyNode &)));
  connect(router,SIGNAL(nodeAboutToBeRemoved(const SyNode &)),
	  this,SLOT(nodeAboutToBeRemovedData(const SyNode &)));
  connect(router,SIGNAL(srcChanged(const SyNode,int,const SySource &)),
	  this,SLOT(srcChangedData(const SyNode,int,const SySource &)));
  connect(router,SIGNAL(dstChanged(const SyNode,int,const SyDestination &)),
	  this,SLOT(dstChangedData(const SyNode,int,const SyDestination &)));
}


Protocol::Type Protocol::type() const
{
  return protocol_type;
}


void Protocol::nodeAddedData(const SyNode &node)
{
  processAddedNode(node);
}


void Protocol::nodeAboutToBeRemovedData(const SyNode &node)
{
  processAboutToBeRemovedNode(node);
}


void Protocol::srcChangedData(const SyNode &node,int slot,const SySource &src)
{
  processChangedSource(node,slot,src);
}


void Protocol::dstChangedData(const SyNode &node,int slot,
			      const SyDestination &dst)
{
  processChangedDestination(node,slot,dst);
}


void Protocol::processAddedNode(const SyNode &node)
{
}


void Protocol::processAboutToBeRemovedNode(const SyNode &node)
{
}


void Protocol::processChangedSource(const SyNode &node,int slot,
				    const SySource &src)
{
}


void Protocol::processChangedDestination(const SyNode &node,int slot,
					 const SyDestination &dst)
{
}


DRouter *Protocol::router() const
{
  return protocol_drouter;
}
