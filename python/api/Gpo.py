# Gpo.py
#
# Container class for a Protocol D Node GPO
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

class Gpo(object):
    def __init__(self,cmds):
        self.__slotNumber=int(cmds[2])
        self.__name=cmds[5]
        self.__hostName=cmds[3]
        self.__hostAddress=cmds[1]
        self.__code=cmds[4]
        self.__sourceAddress=cmds[6]
        self.__sourceSlot=int(cmds[7])

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

    def sourceAddress(self):
        """
           Returns the IP address of the node originating GPI events for
           this GPO. ("SRCA" attribute) (string)
        """
        return self.__sourceAddress

    def sourceSlot(self):
        """
           Returns the slot number on the node originating GPI events for
           this GPO. (integer, -1 = none assigned)
        """
        return self.__sourceSlot

    def __eq__(self,other):
        return self.__slotNumber==other.__slotNumber and self.__name==other.__name and self.__hostName==other.__hostName and self.__hostAddress==other.__hostAddress and self.__code==other.__code and self.__sourceAddress==other.__sourceAddress and self.__sourceSlot==other.__sourceSlot

    def __ne__(self,other):
        return not self.__eq__(other)

    def __lt__(self,other):
        if self.__hostAddress>other.hostAddress():
            return False
        if self.__hostAddress==other.hostAddress() and self.__slotNumber>other.slotNumber():
            return False
        return True

    def __str__(self):
        return "slotNumber: "+str(self.__slotNumber)+"\n"+"name: "+self.__name+"\n"+"hostName: "+self.__hostName+"\n"+"hostAddress: "+self.__hostAddress+"\n"+"code: "+self.__code+"\n"+"sourceAddress: "+self.__sourceAddress+"\n"+"sourceSlot: "+str(self.__sourceSlot)+"\n"
