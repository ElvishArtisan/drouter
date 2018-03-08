#! /usr/bin/python

# Destination.py
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

class Destination(object):
    def __init__(self,cmds):
        self.__slotNumber=int(cmds[2])
        self.__name=cmds[5]
        self.__hostAddress=cmds[1]
        self.__hostName=cmds[3]
        self.__streamAddress=cmds[4]
        self.__channels=int(cmds[6])

    def slotNumber(self):
        """
           Returns the slot number (integer, zero-based). 
        """
        return self.__slotNumber

    def name(self):
        """
           Returns the slot name ("NAME" attribute) (string)
        """
        return self.__name

    def hostAddress(self):
        """
           Returns the IP address of the node (string).
        """
        return self.__hostAddress

    def hostName(self):
        """
           Returns the host name of the node (string).
        """
        return self.__hostName

    def streamAddress(self):
        """
           Returns the reception multicast stream address ("ADDR"
           attribute) (string)
        """
        return self.__streamAddress

    def streamNumber(self):
        """
           Returns the Livewire stream number corresponding to the
           streamAddress().
        """
        octets=self.__streamAddress.split(".")
        if len(octets)!=4 :
            return 0
        if int(octets[2])>127:
            return 0
        return 256*int(octets[2])+int(octets[3])

    def channels(self):
        """
           Returns the maximum number of channels of which this destination
           is capable ("NCHN" attribute) (integer).
        """
        return self.__channels

    def __eq__(self,other):
        return self.__slotNumber==other.__slotNumber and self.__name==other.__name and self.__hostAddress==other.__hostAddress and self.__hostName==other.__hostName and self.__streamAddress==other.__streamAddress and self.__channels==other.__channels

    def __ne__(self,other):
        return not self.__eq__(other)

    def __str__(self):
        return "slotNumber: "+str(self.__slotNumber)+"\n"+"name: "+self.__name+"\n"+"hostName: "+self.__hostName+"\n"+"hostAddress: "+self.__hostAddress+"\n"+"streamAddress: "+self.__streamAddress+"\n"+"streamNumber: "+str(self.streamNumber())+"\n"+"channels: "+str(self.__channels)+"\n"
