#! /usr/bin/python

# dparser.py
#
# Protocol D parser for drouter
#
#   (C) Copyright 2018 Fred Gleason <fredg@paravelsystems.com>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License version 2 as
#   published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public
#   License along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

import socket

import drouter.destination
import drouter.gpi
import drouter.gpo
import drouter.node
import drouter.source

class dparser:
    def __init__(self,ready_cb,add_cb,del_cb,change_cb):
        self.ready_callback=ready_cb
        self.add_callback=add_cb
        self.delete_callback=del_cb
        self.change_callback=change_cb
        self.__sock=socket.socket(socket.AF_INET)
        self.nodes={}
        self.sources={}
        self.destinations={}
        self.gpis={}
        self.gpos={}
        self.__nodes_loaded=False
        self.__sources_loaded=False
        self.__destinations_loaded=False
        self.__gpis_loaded=False
        self.__gpos_loaded=False
        self.__loaded=False

    def connect(self,hostname):
        self.__conn=self.__sock.connect((hostname,23883))
        self.__accum=""
        c=""
        self.__sock.send("SubscribeNodes\r\n")
        while 1<2:
            c=self.__sock.recv(1)
            if c[0]=="\r":
                self.__processMessage()
                self.__accum=""
            else:
                if c[0]!="\n":
                    self.__accum+=c

    def key(self,host_addr,slot):
        return host_addr+":"+str(slot)

    def __processMessage(self):
#       print self.__accum
        cmds=self.__accum.split("\t")
        if cmds[0]=="NODEADD":
            self.nodes[cmds[1]]=drouter.node.node(cmds)
            if(self.add_callback!=None) and self.__loaded:
                self.add_callback(self,"NODE",self.nodes[cmds[1]])
            return
        if cmds[0]=="NODEDEL":
            node=self.nodes[cmds[1]]
            if (self.delete_callback!=None) and self.__loaded:
                self.delete_callback(self,"NODE",node)
            del self.nodes[cmds[1]]
            return

        if cmds[0]=="SRCADD":
            self.sources[self.key(cmds[1],int(cmds[2]))]=drouter.source.source(cmds)
            if(self.add_callback!=None) and self.__loaded:
                self.add_callback(self,"SRC",self.sources[self.key(cmds[1],int(cmds[2]))])
            return
        if cmds[0]=="SRCDEL":
            source=self.sources[self.key(cmds[1],int(cmds[2]))]
            if (self.delete_callback!=None) and self.__loaded:
                self.delete_callback(self,"SRC",source)
            del self.sources[self.key(cmds[1],int(cmds[2]))]
            return
        if cmds[0]=="SRC":
            oldsrc=self.sources[self.key(cmds[1],int(cmds[2]))]
            self.sources[self.key(cmds[1],int(cmds[2]))]=drouter.source.source(cmds)
            if (self.change_callback!=None) and self.__loaded and oldsrc != self.sources[self.key(cmds[1],int(cmds[2]))]:
                self.change_callback(self,"SRC",oldsrc,
                                     self.sources[self.key(cmds[1],int(cmds[2]))])

        if cmds[0]=="DSTADD":
            self.destinations[self.key(cmds[1],int(cmds[2]))]=drouter.destination.destination(cmds);
            if(self.add_callback!=None) and self.__loaded:
                self.add_callback(self,"DST",self.destinations[self.key(cmds[1],int(cmds[2]))])
            return
        if cmds[0]=="DSTDEL":
            destination=self.destinations[self.key(cmds[1],int(cmds[2]))]
            if (self.delete_callback!=None) and self.__loaded:
                self.delete_callback(self,"DST",destination)
            del self.destinations[self.key(cmds[1],int(cmds[2]))]
            return
        if cmds[0]=="DST":
            olddst=self.destinations[self.key(cmds[1],int(cmds[2]))]
            self.destinations[self.key(cmds[1],int(cmds[2]))]=drouter.destination.destination(cmds)
            if (self.change_callback!=None) and self.__loaded and olddst != self.destinations[self.key(cmds[1],int(cmds[2]))]:
                self.change_callback(self,"DST",olddst,
                                     self.destinations[self.key(cmds[1],int(cmds[2]))])

        if cmds[0]=="GPIADD":
            self.gpis[self.key(cmds[1],int(cmds[2]))]=drouter.gpi.gpi(cmds)
            if(self.add_callback!=None) and self.__loaded:
                self.add_callback(self,"GPI",self.gpis[self.key(cmds[1],int(cmds[2]))])
            return
        if cmds[0]=="GPIDEL":
            gpi=self.gpis[self.key(cmds[1],int(cmds[2]))]
            if (self.delete_callback!=None) and self.__loaded:
                self.delete_callback(self,"GPI",gpi)
            del self.gpis[self.key(cmds[1],int(cmds[2]))]
            return
        if cmds[0]=="GPI":
            oldgpi=self.gpis[self.key(cmds[1],int(cmds[2]))]
            self.gpis[self.key(cmds[1],int(cmds[2]))]=drouter.gpi.gpi(cmds)
            if (self.delete_callback!=None) and self.__loaded and oldgpi != self.gpis[self.key(cmds[1],int(cmds[2]))]:
                self.change_callback(self,"GPI",oldgpi,self.gpis[self.key(cmds[1],int(cmds[2]))])
            return

        if cmds[0]=="GPOADD":
            self.gpos[self.key(cmds[1],int(cmds[2]))]=drouter.gpo.gpo(cmds)
            if(self.add_callback!=None) and self.__loaded:
                self.add_callback(self,"GPO",self.gpos[self.key(cmds[1],int(cmds[2]))])
            return
        if cmds[0]=="GPODEL":
            gpo=self.gpos[self.key(cmds[1],int(cmds[2]))]
            if (self.delete_callback!=None) and self.__loaded:
                self.delete_callback(self,"GPO",gpo)
            del self.gpos[self.key(cmds[1],int(cmds[2]))]
            return
        if cmds[0]=="GPO":
            oldgpo=self.gpos[self.key(cmds[1],int(cmds[2]))]
            self.gpos[self.key(cmds[1],int(cmds[2]))]=drouter.gpo.gpo(cmds)
            if (self.delete_callback!=None) and self.__loaded and oldgpo != self.gpos[self.key(cmds[1],int(cmds[2]))]:
                self.change_callback(self,"GPO",oldgpo,self.gpos[self.key(cmds[1],int(cmds[2]))])
            return

        if cmds[0]=="ok":
            if not self.__nodes_loaded:
                self.__nodes_loaded=True
                self.__sock.send("SubscribeSources\r\n")
                return;

            if not self.__sources_loaded:
                self.__sources_loaded=True
                self.__sock.send("SubscribeDestinations\r\n")
                return;

            if not self.__destinations_loaded:
                self.__destinations_loaded=True
                self.__sock.send("SubscribeGpis\r\n")
                return;

            if not self.__gpis_loaded:
                self.__gpis_loaded=True
                self.__sock.send("SubscribeGpos\r\n")
                return;

            if not self.__gpos_loaded:
                self.__gpos_loaded=True
                self.__loaded=True
                if self.ready_callback!=None:
                    self.ready_callback(self)
                return;
            return;