#!%PYTHON_BANGPATH%

# show_gpis.py
#
# Drouter state script to enumerate all gpis
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
# For this script, we simply enumerate all gpis, then exit
#
def EngineReady(engine,priv):
    gpis=engine.Gpis()

    for gpi in gpis:
        print("*********************************")
        print(gpi)
        print("*********************************")

    exit(0)

# ############################################################################
#
# Event Loop
#
# Create a 'StateEngine' object to talk to the drouter service.
#
engine=Drouter.StateEngine.StateEngine()

#
# Set the "ready" callback so we receive notification when the engine
# has completed initialization.
#
engine.setReadyCallback(EngineReady)

#
# Start the engine, giving the hostname/address of the Drouter service.
#
engine.start("localhost")
