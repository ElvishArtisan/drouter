#! /usr/bin/python

# Source.py
#
# Container class for a Protocol D Source
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

class Source(object):
    def __init__(self,cmds):
        self.__slotNumber=int(cmds[2])
        self.__name=cmds[5]
        self.__hostAddress=cmds[1]
        self.__hostName=cmds[3]
        self.__streamAddress=cmds[4]
        self.__streamEnabled=cmds[6]=="1"
        self.__channels=int(cmds[7])
        self.__blockSize=int(cmds[8])

    def slotNumber(self):
        """
           Returns the slot number (integer, zero-based). 
        """
        return self.__slotNumber

    def name(self):
        """
           Returns the slot name ("PSNM" attribute) (string)
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
           Returns the transmission multicast stream address ("RTPA"
           attribute) (string)
        """
        return self.__streamAddress

    def channels(self):
        """
           Returns the number of channels which this is sending
           ("NCHN" attribute) (integer).
        """
        return self.__channels

    def blockSize(self):
        """
           Returns the number of PCM frames per packet ("RTPP" attribute)
           (integer).
        """
        return self.__blockSize

    def __eq__(self,other):
        return self.__slotNumber==other.__slotNumber and self.__name==other.__name and self.__hostAddress==other.__hostAddress and self.__hostName==other.__hostName and self.__streamAddress==other.__streamAddress and self.__streamEnabled==other.__streamEnabled and self.__channels==other.__channels and self.__blockSize==other.__blockSize

    def __ne__(self,other):
        return not self.__eq__(other)

    def __str__(self):
        return "slotNumber: "+str(self.__slotNumber)+"\n"+"name: "+self.__name+"\n"+"hostName: "+self.__hostName+"\n"+"hostAddress: "+self.__hostAddress+"\n"+"streamAddress: "+self.__streamAddress+"\n"+"streamEnabled: "+str(self.__streamEnabled)+"\n"+"channels: "+str(self.__channels)+"\n"+"blockSize: "+str(self.__blockSize)+"\n"
