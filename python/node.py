#! /usr/bin/python

# node.py
#
# Container class for a Protocol D Node
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

class node:
    def __init__(self,h_name,h_addr,dev_name,src_slots,dst_slots,gpi_slots,
                 gpo_slots):
        self.hostName=h_name
        self.hostAddress=h_addr
        self.deviceName=dev_name
        self.sourceQuantity=src_slots
        self.destinationQuantity=dst_slots
        self.gpiQuantity=gpi_slots
        self.gpoQuantity=gpo_slots

    def __init__(self,cmds):
        self.hostName=cmds[2]
        self.hostAddress=cmds[1]
        self.deviceName=cmds[3]
        self.sourceQuantity=int(cmds[4])
        self.destinationQuantity=int(cmds[5])
        self.gpiQuantity=int(cmds[6])
        self.gpoQuantity=int(cmds[7])

    def __str__(self):
        return "hostName: "+self.hostName+"\n"+"hostAddress: "+self.hostAddress+"\n"+"deviceName: "+self.deviceName+"\n"+"sourceQuantity: "+str(self.sourceQuantity)+"\n"+"destinationQuantity: "+str(self.destinationQuantity)+"\n"+"gpiQuantity: "+str(self.gpiQuantity)+"\n"+"gpoQuantity: "+str(self.gpoQuantity)+"\n"
