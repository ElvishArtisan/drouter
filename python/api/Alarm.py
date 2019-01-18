# Alarm.py
#
# Container class for a Protocol D Alarm
#
#   (C) Copyright 2018-2019 Fred Gleason <fredg@paravelsystems.com>
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

class Alarm(object):
    def __init__(self,cmds):
        self.__event=cmds[0]
        self.__slotNumber=int(cmds[2])
        self.__hostAddress=cmds[1]
        self.__port=cmds[3]
        self.__channel=cmds[4]
        self.__state=cmds[5]=="1"

    def event(self):
        """
           Returns the type of alarm event (string).
           Possible values are "CLIP" or "SILENCE".
        """
        return self.__event

    def slotNumber(self):
        """
           Returns the slot number that originated the alarm
           (integer, zero-based). 
        """
        return self.__slotNumber

    def hostAddress(self):
        """
           Returns the IP address of the node that originated the alarm
           (string).
        """
        return self.__hostAddress

    def port(self):
        """
           Returns the port that originated the alarm (string).
           Possible values are "INPUT" or "OUTPUT".
        """
        return self.__port

    def channel(self):
        """
           Returns the channel that originated the alarm (string).
           Possible values are "LEFT" or "RIGHT".
        """
        return self.__channel

    def state(self):
        """
           Returns the state of the alarm (boolean).
           True = active, False = clear
        """
        return self.__state

    def __eq__(self,other):
        return self.__event==other.__event and self.__slotNumber==other.__slotNumber and self.__hostAddress==other.__hostAddress and self.__port==other.__port and self.__channel==other.__channel and self.__state==other.__state

    def __ne__(self,other):
        return not self.__eq__(other)

    def __str__(self):
        return "event: "+self.__event+"\n"+"slotNumber: "+str(self.__slotNumber)+"\n"+"hostAddress: "+self.__hostAddress+"\n"+"port: "+self.__port+"\n"+"channel: "+str(self.__channel)+"\n"+"state: "+str(self.__state)+"\n"
