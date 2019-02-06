# MGpo.py
#
# Container class for a Protocol D Multicast GPO
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

class MGpo(object):
    def __init__(self,cmds):
        self.__originAddress=cmds[1]
        self.__sourceNumber=int(cmds[2])
        self.__code=cmds[3]

    def originAddress(self):
        """
           Returns the IPv4 address of the originating device.
        """
        return self.__originAddress

    def sourceNumber(self):
        """
           Returns the Livewire source number (integer, 1 - 32767). 
        """
        return self.__sourceNumber

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
        return self.__sourceNumber==other.__sourceNumber and self.__code==other.__code and self.__originAddress==other.__originAddress

    def __ne__(self,other):
        return not self.__eq__(other)

    def __lt__(self,other):
        if self.__sourceNumber>other.sourceNumber():
            return False
        return True

    def __str__(self):
        return "sourceNumber: "+str(self.__sourceNumber)+"\n"+"originAddress: "+self.__originAddress+"\n"+"code: "+self.__code+"\n"
