# Node.py
#
# Container class for a Protocol D Node
#
#   (C) Copyright 2019 Fred Gleason <fredg@paravelsystems.com>
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

class Node(object):
    def __init__(self,cmds):
        self.__hostName=cmds[2]
        self.__hostAddress=cmds[1]
        self.__deviceName=cmds[3]
        self.__sourceQuantity=int(cmds[4])
        self.__destinationQuantity=int(cmds[5])
        self.__gpiQuantity=int(cmds[6])
        self.__gpoQuantity=int(cmds[7])

    def hostName(self):
        """
           Returns the host name of the node (string).
        """
        return self.__hostName

    def hostAddress(self):
        """
           Returns the IP address of the node (string).
        """
        return self.__hostAddress

    def deviceName(self):
        """
           Returns the device name of the node ("DEVN" attribute) (string).
        """
        return self.__deviceName

    def sourceQuantity(self):
        """
           Returns the number of source slots on this node
           ("NSRC" attribute) (string).
        """
        return self.__sourceQuantity

    def destinationQuantity(self):
        """
           Returns the number of destinations slots on this node
           ("NDST" attribute) (string).
        """
        return self.__destinationQuantity

    def gpiQuantity(self):
        """
           Returns the number of GPI slots on this node
           ("NGPI" attribute) (string).
        """
        return self.__gpiQuantity

    def gpoQuantity(self):
        """
           Returns the number of GPO slots on this node
           ("NGPO" attribute) (string).
        """
        return self.__gpoQuantity

    def __eq__(self,other):
        return self.__hostName==other.__hostName and self.__hostAddress==other.__hostAddress and self.__deviceName==other.__deviceName and self.__sourceQuantity==other.__sourceQuantity and self.__destinationQuantity==other.__destinationQuantity and self.__gpiQuantity==other.__gpiQuantity and self.__gpoQuantity==other.__gpoQuantity

    def __ne__(self,other):
        return self.__eq__(other)

    def __lt__(self,other):
        if self.__hostAddress>other.hostAddress():
            return False
        return True

    def __str__(self):
        return "hostName: "+self.__hostName+"\n"+"hostAddress: "+self.__hostAddress+"\n"+"deviceName: "+self.__deviceName+"\n"+"sourceQuantity: "+str(self.__sourceQuantity)+"\n"+"destinationQuantity: "+str(self.__destinationQuantity)+"\n"+"gpiQuantity: "+str(self.__gpiQuantity)+"\n"+"gpoQuantity: "+str(self.__gpoQuantity)+"\n"
