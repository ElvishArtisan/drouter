#! /usr/bin/python

# Alarm.py
#
# Container class for a Protocol D Alarm
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

class Alarm:
    def __init__(self,cmds):
        self.event=cmds[0]
        self.slotNumber=cmds[2]
        self.hostAddress=cmds[1]
        self.port=cmds[3]
        self.channel=cmds[4]
        self.state=cmds[5]=="1"

    def __eq__(self,other):
        return self.event==other.event and self.slotNumber==other.slotNumber and self.hostAddress==other.hostAddress and self.port==other.port and self.channel==other.channel and self.state==other.state

    def __ne__(self,other):
        return not self.__eq__(other)

    def __str__(self):
        return "event: "+self.event+"\n"+"slotNumber: "+str(self.slotNumber)+"\n"+"hostAddress: "+self.hostAddress+"\n"+"port: "+self.port+"\n"+"channel: "+str(self.channel)+"\n"+"state: "+str(self.state)+"\n"
