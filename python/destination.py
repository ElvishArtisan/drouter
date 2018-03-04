#! /usr/bin/python

# destination.py
#
# Container class for a Protocol D Destination
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

class destination:
    def __init__(self,cmds):
        self.slotNumber=int(cmds[2])
        self.name=cmds[5]
        self.hostAddress=cmds[1]
        self.hostName=cmds[3]
        self.streamAddress=cmds[4]
        self.channels=int(cmds[6])

    def __eq__(self,other):
        return self.slotNumber==other.slotNumber and self.name==other.name and self.hostAddress==other.hostAddress and self.hostName==other.hostName and self.streamAddress==other.streamAddress and self.channels==other.channels

    def __ne__(self,other):
        return not self.__eq__(other)

    def __str__(self):
        return "slotNumber: "+str(self.slotNumber)+"\n"+"name: "+self.name+"\n"+"hostName: "+self.hostName+"\n"+"hostAddress: "+self.hostAddress+"\n"+"streamAddress: "+self.streamAddress+"\n"+"channels: "+str(self.channels)+"\n"
