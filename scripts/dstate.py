#!%PYTHON_BANGPATH%

# dstate.py
#
# Return the active state of a Drouter instance
#
# (C) Copyright 2019 Fred Gleason <fredg@paravelsystems.com>
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

import argparse
import os
import sys
import Drouter.StateEngine

def EngineReady(engine,priv):
    print(str(engine.isActive()))
    sys.exit(0)

# ############################################################################
#
# Event Loop
#
#
# Read the arguments
#
parser=argparse.ArgumentParser(description='Print the tether state of a Drouter instance')
parser.add_argument("--hostname",required=False,default="",help="hostname of drouter server (overrides $DROUTER_SERVER)")
args = parser.parse_args()

#
# Get the hostname of the drouter service
#
if args.hostname=="":
    try:
        args.hostname=os.environ["DROUTER_HOSTNAME"]
    except(KeyError):
        args.hostname="localhost"

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
engine.start(args.hostname)

