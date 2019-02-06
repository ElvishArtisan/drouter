#!%PYTHON_BANGPATH%

# show_multicast.py
#
# Drouter state script to show multicast state changes in a Livewire network
# in realtime.
#
# (C) Copyright 2018-2019 Fred Gleason <fredg@paravelsystems.com>
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

import Drouter.StateEngine

# ############################################################################
#
# Callbacks
#
#  These are called by the 'StateEngine' object in response to specific events.
#

#
# Called immediately after the 'StateEngine' object has completed
# initialization. This is the place to do any needed startup initialization
# (create objects, open connections, etc).
#
def EngineReady(engine,priv):
    print("EngineReady() ran!")


#
# Called every time a multicast update is received.
#
def UpdateReceived(engine,priv,type,object):
    if type=="MGPI":
        print("Multicast GPI update received")
        print(object)

    if type=="MGPO":
        print("Multicast GPO update received")
        print(object)


# ############################################################################
#
# Event Loop
#
# Create a 'StateEngine' object to talk to the drouter service.
#
engine=Drouter.StateEngine.StateEngine()

#
# Set the callbacks so we receive notifications of changes.
#
engine.setReadyCallback(EngineReady)
engine.setMulticastReceivedCallback(UpdateReceived)

#
# Start the engine, giving the hostname/address of the Drouter service.
#
engine.start("localhost")
