#! /usr/bin/python

# Gpi.py
#
# Container class for a Protocol D GPI
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

class Gpi(object):
    def __init__(self,cmds):
        self.__slotNumber=int(cmds[2])
        self.__hostName=cmds[3]
        self.__hostAddress=cmds[1]
        self.__code=cmds[4]

    def slotNumber(self):
        """
           Returns the slot number (integer, zero-based). 
        """
        return self.__slotNumber

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

    def code(self):
        """
           Returns the combined state in form 'xxxxx' (string).
        """
        return self.__code

    def bitState(self,bit):
        """
           Returns the state of one bit (boolean).
        """
        return self.__code[bit].lower()=="l"

    def __eq__(self,other):
        return self.__slotNumber==other.__slotNumber and self.__hostName==other.__hostName and self.__hostAddress==other.__hostAddress and self.__code==other.__code

    def __ne__(self,other):
        return not self.__eq__(other)

    def __str__(self):
        return "slotNumber: "+str(self.__slotNumber)+"\n"+"hostName: "+self.__hostName+"\n"+"hostAddress: "+self.__hostAddress+"\n"+"code: "+self.__code+"\n"
