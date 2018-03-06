#! /usr/bin/python

# StateEngine.py
#
# Protocol D engine for executing state scripts.
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
    """
       Initialize an instance of StateEngine.
    """
    def __init__(self):
        self.__nodes={}
        self.__sources={}
        self.__destinations={}
        self.__gpis={}
        self.__gpos={}
        self.__ready_callback=None
        self.__add_callback=None
        self.__delete_callback=None
        self.__change_callback=None
        self.__alarm_callback=None
        self.__sock=socket.socket(socket.AF_INET)
        self.__nodes_loaded=False
        self.__sources_loaded=False
        self.__destinations_loaded=False
        self.__gpis_loaded=False
        self.__gpos_loaded=False
        self.__silences_loaded=False
        self.__clips_loaded=False
        self.__loaded=False

    def Destination(self,host_addr,slot):
        """
           Return a Destination object, where:

           host_addr: Host IP address of the destination node (string)

                slot: Number of the destination slot (integer, zero-based)
        """
        return self.__destinations[self.__key(host_addr,slot)]

    def Destinations(self):
        """
           Return a list of all Destination objects
        """
        return self.__destinations.values()

    def Node(self,host_addr):
        """
           Return a Node object, where:

           host_addr: Host IP address of the node node (string)
        """
        return self.__nodes[host_addr]

    def Nodes(self):
        """
           Return a list of all Node objects
        """
        return self.__nodes.values()

    def Source(self,host_addr,slot):
        """
           Return a Source object, where:

           host_addr: Host IP address of the source node (string)

                slot: Number of the source slot (integer, zero-based)
        """
        return self.__sources[self.__key(host_addr,slot)]

    def Sources(self):
        """
           Return a list of all Source objects
        """
        return self.__sources.values()

    def Gpi(self,host_addr,slot):
        """
           Return a Gpi object, where:

           host_addr: Host IP address of the gpi node (string)

                slot: Number of the gpi slot (integer, zero-based)
        """
        return self.__gpis[self.__key(host_addr,slot)]

    def Gpis(self):
        """
           Return a list of all Gpi objects
        """
        return self.__gpis.values()

    def Gpo(self,host_addr,slot):
        """
           Return a Gpo object, where:

           host_addr: Host IP address of the gpo node (string)

                slot: Number of the gpo slot (integer, zero-based)
        """
        return self.__gpos[self.__key(host_addr,slot)]

    def Gpos(self):
        """
           Return a list of all Gpo objects
        """
        return self.__gpos.values()

    def setReadyCallback(self,cb):
        """
        Set the 'ready' callback, called by StateEngine immediately after
        initialization is complete. The callback should be of the form:

           def callback(self,engine)

        where:
          engine: reference to the calling StateEngine.
        """
        self.__ready_callback=cb

    def setAddCallback(self,cb):
        """
        Set the 'add' callback, called by StateEngine immediately after
        a new object is added to the Drouter system. The callback should be
        of the form:

           def callback(self,engine,type,object)

        where:
          engine: reference to the calling StateEngine
 
            type: string, giving the type of object added. Possible values
                  are "DST", "GPI", "GPO", "NODE" or "SRC".

          object: reference to the object added. Possible classes referenced
                  are Destination, Gpi, Gpo Node or Source.
        """
        self.__add_callback=cb

    def setDeleteCallback(self,cb):
        """
        Set the 'delete' callback, called by StateEngine immediately before
        an object is removed from the Drouter system. The callback should be
        of the form:

           def callback(self,engine,type,object)

        where:
          engine: reference to the calling StateEngine
 
            type: string, giving the type of object being removed.
                  Possible values are "DST", "GPI", "GPO", "NODE" or "SRC".

          object: reference to the object removed. Possible classes referenced
                  are Destination, Gpi, Gpo Node or Source.
        """
        self.__delete_callback=cb

    def setChangeCallback(self,cb):
        """
        Set the 'change' callback, called by StateEngine immediately after
        an existing object is modified in the Drouter system. The callback
        should be of the form:

           def callback(self,engine,type,old_object,new_object)

        where:
             engine: reference to the calling StateEngine
 
               type: string, giving the type of object being modified.
                     Possible values are "DST", "GPI", "GPO", "NODE" or
                     "SRC".

         old_object: reference to the object being modified prior to the
                     modification. Possible classes referenced
                     are Destination, Gpi, Gpo Node or Source.

         new_object: reference to the object being modified after the
                     modification. Possible classes referenced
                     are Destination, Gpi, Gpo Node or Source.
        """
        self.__change_callback=cb

    def setAlarmCallback(self,cb):
     """
        Set the 'alarm' callback, called by StateEngine immediately after
        reception of an audio alarm (a CLIP or SILENCE event).

           def callback(self,engine,alarm)

        where:
             engine: reference to the calling StateEngine

              alarm: reference to an Alarm object
     """
     self.__alarm_callback=cb

    def clearCrosspoint(self,out_host_addr,out_slot):
        """
           Clear the multicast source address ("ADDR" attribute) of an
           audio destination, effectively muting it. Takes the following
           arguments:

           out_host_addr: Host IP address of the destination node (string)

                out_slot: Number of the destination slot (integer, zero-based)
        """
        self.__sock.send("ClearCrosspoint "+out_host_addr+" "+str(out_slot)+"\r\n")
        
    def setCrosspoint(self,out_host_addr,out_slot,in_host_addr,in_slot):
        """
           Set the multicast source address ("ADDR" attribute) of an
           audio destination to the multicast address ("RTPA" attribute)
           of the specified source. Takes the following arguments:

           out_host_addr: Host IP address of the destination node (string)

                out_slot: Slot number of the destination slot (integer,
                          zero-based)

            in_host_addr: Host IP address of the source node (string)

                 in_slot: Number of the source slot (integer, zero-based)
        """
        self.__sock.send("SetCrosspoint "+out_host_addr+" "+str(out_slot)+" "+in_host_addr+" "+str(in_slot)+"\r\n")

    def clearGpioCrosspoint(self,out_host_addr,out_slot):
        """
           Clear the multicast source address ("SRCA" attribute) of a
           GPO. Takes the following arguments:

           out_host_addr: Host IP address of the GPO node (string)

                out_slot: Number of the GPO slot (integer, zero-based)
        """
        self.__sock.send("ClearGpioCrosspoint "+out_host_addr+" "+str(out_slot)+"\r\n")
        
    def setGpioCrosspoint(self,out_host_addr,out_slot,in_host_addr,in_slot):
        """
           Set the multicast source address ("SRCA" attribute) of a
           GPO to the multicast value of the specified GPI.
           Takes the following arguments:

           out_host_addr: Host IP address of the GPO node (string)

                out_slot: Number of the GPO slot (integer, zero-based)

            in_host_addr: Host IP address of the GPI node (string)

                 in_slot: Number of the GPI slot (integer, zero-based)
        """
        self.__sock.send("SetGpioCrosspoint "+out_host_addr+" "+str(out_slot)+" "+in_host_addr+" "+str(in_slot)+"\r\n")

    def setGpoCode(self,host_addr,slot,code):
        """
           Simultaneously set the state of all five lines of a GPO.
           Takes the following arguments:

           host_addr: Host IP address of the GPO node (string)

                slot: Number of the GPO slot (integer, zero-based)

                code: String representing desired state, in the form "xxxxx",
                      where 'h' indicates "OFF" and 'l' indicates "ON".
        """
        self.__sock.send("SetGpoState "+host_addr+" "+str(slot)+" "+code+"\r\n")

    def setGpoBitState(self,host_addr,slot,bit,state):
        """
           Set the state of a single line of a GPO while leaving the state
           of the other lines untouched. Takes the following arguments:

           host_addr: Host IP address of the GPO node (string)

                slot: Number of the GPO slot (integer, zero-based)

                 bit: The number of the line to set, 0 = most significant line,
                      4 = least significant line (integer)

               state: The state to set (boolean)
        """
        self.setGpoCode(host_addr,slot,self.__bitStateCode(bit,state))

    def setGpiCode(self,host_addr,slot,code):
        """
           Simultaneously set the state of all five lines of a virtual GPI.
           Takes the following arguments:

           host_addr: Host IP address of the GPI node (string)

                slot: Number of the GPI slot(integer, zero-based)

                code: String representing desired state, in the form "xxxxx",
                      where 'h' indicates "OFF" and 'l' indicates "ON".

           NOTE: This method works only with "virtual" GPIs --i.e. those
                 associated with a software node, such a Linux or Windows
                 software driver.
        """
        self.__sock.send("SetGpiState "+host_addr+" "+str(slot)+" "+code+"\r\n")

    def setGpiBitState(self,host_addr,slot,bit,state):
        """
           Set the state of a single line of a virtual GPI while leaving
           the state of the other lines untouched.
           Takes the following arguments:

           host_addr: Host IP address of the GPI node (string)

                slot: Number of the GPI slot (integer, zero-based)

                 bit: The number of the line to set, 0 = most significant line,
                      4 = least significant line (integer)

               state: The state to set (boolean)

           NOTE: This method works only with "virtual" GPIs --i.e. those
                 associated with a software node, such a Linux or Windows
                 software driver.
        """
        self.setGpiCode(host_addr,slot,self.__bitStateCode(bit,state))

    def start(self,hostname):
        """
           Connect to a specified Drouter service and begin dispatching
           callbacks. Once started, a StateEngine can be interacted with
           only within one of its callback functions.
           Takes the following argument:

           hostname: The hostname or IP address of the system running
                     the Drouter service.
        """
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

    def __key(self,host_addr,slot):
        return host_addr+":"+str(slot)

    def __processMessage(self):
#       print self.__accum
        cmds=self.__accum.split("\t")
        if cmds[0]=="NODEADD":
            self.__nodes[cmds[1]]=Drouter.Node.Node(cmds)
            if(self.__add_callback!=None) and self.__loaded:
                self.__add_callback(self,"NODE",self.__nodes[cmds[1]])
            return
        if cmds[0]=="NODEDEL":
            node=self.__nodes[cmds[1]]
            if (self.__delete_callback!=None) and self.__loaded:
                self.__delete_callback(self,"NODE",node)
            del self.__nodes[cmds[1]]
            return

        if cmds[0]=="SRCADD":
            self.__sources[self.__key(cmds[1],int(cmds[2]))]=Drouter.Source.Source(cmds)
            if(self.__add_callback!=None) and self.__loaded:
                self.__add_callback(self,"SRC",self.__sources[self.__key(cmds[1],int(cmds[2]))])
            return
        if cmds[0]=="SRCDEL":
            source=self.__sources[self.__key(cmds[1],int(cmds[2]))]
            if (self.__delete_callback!=None) and self.__loaded:
                self.__delete_callback(self,"SRC",source)
            del self.__sources[self.__key(cmds[1],int(cmds[2]))]
            return
        if cmds[0]=="SRC":
            oldsrc=self.__sources[self.__key(cmds[1],int(cmds[2]))]
            self.__sources[self.__key(cmds[1],int(cmds[2]))]=Drouter.Source.Source(cmds)
            if (self.__change_callback!=None) and self.__loaded and oldsrc != self.__sources[self.__key(cmds[1],int(cmds[2]))]:
                self.__change_callback(self,"SRC",oldsrc,
                                     self.__sources[self.__key(cmds[1],int(cmds[2]))])

        if cmds[0]=="DSTADD":
            self.__destinations[self.__key(cmds[1],int(cmds[2]))]=Drouter.Destination.Destination(cmds);
            if(self.__add_callback!=None) and self.__loaded:
                self.__add_callback(self,"DST",self.__destinations[self.__key(cmds[1],int(cmds[2]))])
            return
        if cmds[0]=="DSTDEL":
            destination=self.__destinations[self.__key(cmds[1],int(cmds[2]))]
            if (self.__delete_callback!=None) and self.__loaded:
                self.__delete_callback(self,"DST",destination)
            del self.__destinations[self.__key(cmds[1],int(cmds[2]))]
            return
        if cmds[0]=="DST":
            olddst=self.__destinations[self.__key(cmds[1],int(cmds[2]))]
            self.__destinations[self.__key(cmds[1],int(cmds[2]))]=Drouter.Destination.Destination(cmds)
            if (self.__change_callback!=None) and self.__loaded and olddst != self.__destinations[self.__key(cmds[1],int(cmds[2]))]:
                self.__change_callback(self,"DST",olddst,
                                     self.__destinations[self.__key(cmds[1],int(cmds[2]))])

        if cmds[0]=="GPIADD":
            self.__gpis[self.__key(cmds[1],int(cmds[2]))]=Drouter.Gpi.Gpi(cmds)
            if(self.__add_callback!=None) and self.__loaded:
                self.__add_callback(self,"GPI",self.__gpis[self.__key(cmds[1],int(cmds[2]))])
            return
        if cmds[0]=="GPIDEL":
            gpi=self.__gpis[self.__key(cmds[1],int(cmds[2]))]
            if (self.__delete_callback!=None) and self.__loaded:
                self.__delete_callback(self,"GPI",gpi)
            del self.__gpis[self.__key(cmds[1],int(cmds[2]))]
            return
        if cmds[0]=="GPI":
            oldgpi=self.__gpis[self.__key(cmds[1],int(cmds[2]))]
            self.__gpis[self.__key(cmds[1],int(cmds[2]))]=Drouter.Gpi.Gpi(cmds)
            if (self.__change_callback!=None) and self.__loaded and oldgpi != self.__gpis[self.__key(cmds[1],int(cmds[2]))]:
                self.__change_callback(self,"GPI",oldgpi,self.__gpis[self.__key(cmds[1],int(cmds[2]))])
            return

        if cmds[0]=="GPOADD":
            self.__gpos[self.__key(cmds[1],int(cmds[2]))]=Drouter.Gpo.Gpo(cmds)
            if(self.__add_callback!=None) and self.__loaded:
                self.__add_callback(self,"GPO",self.__gpos[self.__key(cmds[1],int(cmds[2]))])
            return
        if cmds[0]=="GPODEL":
            gpo=self.__gpos[self.__key(cmds[1],int(cmds[2]))]
            if (self.__delete_callback!=None) and self.__loaded:
                self.__delete_callback(self,"GPO",gpo)
            del self.__gpos[self.__key(cmds[1],int(cmds[2]))]
            return
        if cmds[0]=="GPO":
            oldgpo=self.__gpos[self.__key(cmds[1],int(cmds[2]))]
            self.__gpos[self.__key(cmds[1],int(cmds[2]))]=Drouter.Gpo.Gpo(cmds)
            if (self.__change_callback!=None) and self.__loaded and oldgpo != self.__gpos[self.__key(cmds[1],int(cmds[2]))]:
                self.__change_callback(self,"GPO",oldgpo,self.__gpos[self.__key(cmds[1],int(cmds[2]))])
            return

        if cmds[0]=="SILENCE" or cmds[0]=="CLIP":
            if (self.__alarm_callback!=None) and self.__loaded:
                self.__alarm_callback(self,Drouter.Alarm.Alarm(cmds))
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
                if self.__ready_callback!=None:
                    self.__ready_callback(self)
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
