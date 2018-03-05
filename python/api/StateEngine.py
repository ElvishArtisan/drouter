#! /usr/bin/python

# StateEngine.py
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

import Drouter.Alarm
import Drouter.Destination
import Drouter.Gpi
import Drouter.Gpo
import Drouter.Node
import Drouter.Source

class StateEngine:
    def __init__(self):
        self.ready_callback=None
        self.add_callback=None
        self.delete_callback=None
        self.change_callback=None
        self.alarm_callback=None
        self.nodes={}
        self.sources={}
        self.destinations={}
        self.gpis={}
        self.gpos={}
        self.__sock=socket.socket(socket.AF_INET)
        self.__nodes_loaded=False
        self.__sources_loaded=False
        self.__destinations_loaded=False
        self.__gpis_loaded=False
        self.__gpos_loaded=False
        self.__silences_loaded=False
        self.__clips_loaded=False
        self.__loaded=False

    def setReadyCallback(self,cb):
        self.ready_callback=cb

    def setAddCallback(self,cb):
        self.add_callback=cb

    def setDeleteCallback(self,cb):
        self.delete_callback=cb

    def setChangeCallback(self,cb):
        self.change_callback=cb

    def setAlarmCallback(self,cb):
        self.alarm_callback=cb

    def clearCrosspoint(self,out_host_addr,out_slot):
        self.__sock.send("ClearCrosspoint "+out_host_addr+" "+str(out_slot)+"\r\n")
        
    def setCrosspoint(self,out_host_addr,out_slot,in_host_addr,in_slot):
        self.__sock.send("SetCrosspoint "+out_host_addr+" "+str(out_slot)+" "+in_host_addr+" "+str(in_slot)+"\r\n")

    def clearGpioCrosspoint(self,out_host_addr,out_slot):
        self.__sock.send("ClearGpioCrosspoint "+out_host_addr+" "+str(out_slot)+"\r\n")
        
    def setGpioCrosspoint(self,out_host_addr,out_slot,in_host_addr,in_slot):
        self.__sock.send("SetGpioCrosspoint "+out_host_addr+" "+str(out_slot)+" "+in_host_addr+" "+str(in_slot)+"\r\n")

    def setGpoCode(self,host_addr,slot,code):
        self.__sock.send("SetGpoState "+host_addr+" "+str(slot)+" "+code+"\r\n")

    def setGpoBitState(self,host_addr,slot,bit,state):
        self.setGpoCode(host_addr,slot,self.__bitStateCode(bit,state))

    def setGpiCode(self,host_addr,slot,code):
        self.__sock.send("SetGpiState "+host_addr+" "+str(slot)+" "+code+"\r\n")

    def setGpiBitState(self,host_addr,slot,bit,state):
        self.setGpiCode(host_addr,slot,self.__bitStateCode(bit,state))

    def start(self,hostname):
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
            self.nodes[cmds[1]]=Drouter.Node.Node(cmds)
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
            self.sources[self.key(cmds[1],int(cmds[2]))]=Drouter.Source.Source(cmds)
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
            self.sources[self.key(cmds[1],int(cmds[2]))]=Drouter.Source.Source(cmds)
            if (self.change_callback!=None) and self.__loaded and oldsrc != self.sources[self.key(cmds[1],int(cmds[2]))]:
                self.change_callback(self,"SRC",oldsrc,
                                     self.sources[self.key(cmds[1],int(cmds[2]))])

        if cmds[0]=="DSTADD":
            self.destinations[self.key(cmds[1],int(cmds[2]))]=Drouter.Destination.Destination(cmds);
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
            self.destinations[self.key(cmds[1],int(cmds[2]))]=Drouter.Destination.Destination(cmds)
            if (self.change_callback!=None) and self.__loaded and olddst != self.destinations[self.key(cmds[1],int(cmds[2]))]:
                self.change_callback(self,"DST",olddst,
                                     self.destinations[self.key(cmds[1],int(cmds[2]))])

        if cmds[0]=="GPIADD":
            self.gpis[self.key(cmds[1],int(cmds[2]))]=Drouter.Gpi.Gpi(cmds)
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
            print "GPI"
            oldgpi=self.gpis[self.key(cmds[1],int(cmds[2]))]
            self.gpis[self.key(cmds[1],int(cmds[2]))]=Drouter.Gpi.Gpi(cmds)
            if (self.change_callback!=None) and self.__loaded and oldgpi != self.gpis[self.key(cmds[1],int(cmds[2]))]:
                self.change_callback(self,"GPI",oldgpi,self.gpis[self.key(cmds[1],int(cmds[2]))])
            return

        if cmds[0]=="GPOADD":
            self.gpos[self.key(cmds[1],int(cmds[2]))]=Drouter.Gpo.Gpo(cmds)
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
            self.gpos[self.key(cmds[1],int(cmds[2]))]=Drouter.Gpo.Gpo(cmds)
            if (self.change_callback!=None) and self.__loaded and oldgpo != self.gpos[self.key(cmds[1],int(cmds[2]))]:
                self.change_callback(self,"GPO",oldgpo,self.gpos[self.key(cmds[1],int(cmds[2]))])
            return

        if cmds[0]=="SILENCE" or cmds[0]=="CLIP":
            if (self.alarm_callback!=None) and self.__loaded:
                self.alarm_callback(self,Drouter.Alarm.Alarm(cmds))
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
                self.__sock.send("SubscribeSilences\r\n")
                return;

            if not self.__silences_loaded:
                self.__silences_loaded=True
                self.__sock.send("SubscribeClips\r\n")

            if not self.__clips_loaded:
                self.__clips_loaded=True
                self.__loaded=True
                if self.ready_callback!=None:
                    self.ready_callback(self)
            return;

    def __bitStateCode(self,bit,state):
        code=""
        for i in range(0,bit):
            code+="x"
        if state:
            code+="l"
        else:
            code+="h"
        for i in range(bit+1,5):
            code+="x"
        return code
