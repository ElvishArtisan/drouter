#!/usr/bin/python

# dlist.py
#
# Generate a printable list of Livewire resources
#
# (C) Copyright 2018 Fred Gleason <fredg@paravelsystems.com>
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
import Drouter.StateEngine

def PrintSources(sources,csv):
    if csv:
        print "Host_Address,Slot_Number,Host_Name,Name,Stream_Address,Stream_Number,Stream_Enabled,Channels,Block_Size"
        for source in sources:
            print "\""+source.hostAddress()+"\","+str(source.slotNumber())+",\""+source.hostName()+"\",\""+source.name()+"\",\""+source.streamAddress()+"\","+str(source.streamNumber())+","+str(source.streamEnabled())+","+str(source.channels())+","+str(source.blockSize())
    else:
        print "----------------------------------------------------------------------------------------------------------------------"
        print "| SOURCES                                                                                                            |"
        print "|--------------------------------------------------------------------------------------------------------------------|"
        print "| Host Address    | Slot | Host Name       | Name             | Stream Address  | Src Num | Enabled | Chans | Blk Sz |"
        print "|-----------------|------|-----------------|------------------|-----------------|---------|---------|-------|--------|"
        for source in sources:
            enabled="No "
            if source.streamEnabled():
                enabled="Yes"
                print "| %-15s | %4d | %-15s | %-16s | %-15s |  %5d  |   %s   |  %2d   |  %3d   |" % (source.hostAddress(),source.slotNumber(),source.hostName(),source.name(),source.streamAddress(),source.streamNumber(),enabled,source.channels(),source.blockSize())
                print "|-----------------|------|-----------------|------------------|-----------------|---------|---------|-------|--------|"


def PrintDestinations(destinations,csv):
    if csv:
        print "Host_Address,Slot_Number,Host_Name,Name,Stream_Address,Stream_Number,Channels"
        for destination in destinations:
            print "\""+destination.hostAddress()+"\","+str(destination.slotNumber())+",\""+destination.hostName()+"\",\""+destination.name()+"\",\""+destination.streamAddress()+"\","+str(destination.streamNumber())+","+str(destination.channels())
    else:
        print "---------------------------------------------------------------------------------------------------"
        print "| DESTINATIONS                                                                                    |"
        print "|-------------------------------------------------------------------------------------------------|"
        print "| Host Address    | Slot | Host Name       | Name             | Stream Address  | Stm Num | Chans |"
        print "|-----------------|------|-----------------|------------------|-----------------|---------|-------|"
        for destination in destinations:
            print "| %-15s | %4d | %-15s | %-16s | %-15s |  %5d  |  %2d   |" % (destination.hostAddress(),destination.slotNumber(),destination.hostName(),destination.name(),destination.streamAddress(),destination.streamNumber(),destination.channels())
            print "|-----------------|------|-----------------|------------------|-----------------|---------|-------|"


def PrintGpis(gpis,csv):
    if csv:
        print "Host_Address,Slot_Number,Host_Name,Code"
        for gpi in gpis:
            print "\""+gpi.hostAddress()+"\","+str(gpi.slotNumber())+",\""+gpi.hostName()+"\",\""+gpi.code()+"\""
    else:
        print "----------------------------------------------------"
        print "| GPIS                                             |"
        print "|--------------------------------------------------|"
        print "| Host Address    | Slot | Host Name       | Code  |"
        print "|-----------------|------|-----------------|-------|"
        for gpi in gpis:
            print "| %-15s | %4d | %-15s | %-5s |" % (gpi.hostAddress(),gpi.slotNumber(),gpi.hostName(),gpi.code())
            print "|-----------------|------|-----------------|-------|"


def PrintGpos(gpos,csv):
    if csv:
        print "Host_Address,Slot_Number,Host_Name,Code,Source_Address,Source_Slot"
        for gpo in gpos:
            print "\""+gpo.hostAddress()+"\","+str(gpo.slotNumber())+",\""+gpo.hostName()+"\",\""+gpo.code()+"\",\""+gpo.sourceAddress()+"\","+str(gpo.sourceSlot())
    else:
        print "---------------------------------------------------------------------------------"
        print "| GPOS                                                                          |"
        print "|-------------------------------------------------------------------------------|"
        print "| Host Address    | Slot | Host Name       | Code  | Src Address     | Src Slot |"
        print "|-----------------|------|-----------------|-------|-----------------|----------|"
        for gpo in gpos:
            print "| %-15s | %4d | %-15s | %-5s | %-15s |   %4d   |" % (gpo.hostAddress(),gpo.slotNumber(),gpo.hostName(),gpo.code(),gpo.sourceAddress(),gpo.sourceSlot())
            print "|-----------------|------|-----------------|-------|-----------------|----------|"


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
# For this script, we simply enumerate all sources, then exit
#
def EngineReady(engine,priv):
    if priv.sources or priv.all:
        sources=[]
        for source in engine.Sources():
            if priv.node_address=="" or priv.node_address==source.hostAddress():
                sources.append(source)
        sources.sort()
        if len(sources)>0:
            PrintSources(sources,priv.csv)
            if priv.all:
                print

    if priv.destinations or priv.all:
        destinations=[]
        for destination in engine.Destinations():
            if priv.node_address=="" or priv.node_address==destination.hostAddress():
                destinations.append(destination)
        destinations.sort()
        if len(destinations)>0:
            PrintDestinations(destinations,priv.csv)
            if priv.all:
                print

    if priv.gpis or priv.all:
        gpis=[]
        for gpi in engine.Gpis():
            if priv.node_address=="" or priv.node_address==gpi.hostAddress():
                gpis.append(gpi)
        gpis.sort()
        if len(gpis)>0:
            PrintGpis(gpis,priv.csv)
            if priv.all:
                print

    if priv.gpos or priv.all:
        gpos=[]
        for gpo in engine.Gpos():
            if priv.node_address=="" or priv.node_address==gpo.hostAddress():
                gpos.append(gpo)
        gpos.sort()
        if len(gpos)>0:
            PrintGpos(gpos,priv.csv)

    exit(0)

# ############################################################################
#
# Event Loop
#
#
# Read the arguments
#
parser=argparse.ArgumentParser(description='Generate a printable list of Sources and Destinations')
parser.add_argument("--hostname",required=False,default="",help="hostname of drouter server (overrides $DROUTER_SERVER)")
parser.add_argument("--node-address",required=False,default="",help="display only resources from specified node")
parser.add_argument("--sources",required=False,action="store_const",const=True,default=False,help="display sources")
parser.add_argument("--destinations",required=False,action="store_const",const=True,default=False,help="display destinations")
parser.add_argument("--gpis",required=False,action="store_const",const=True,default=False,help="display gpis")
parser.add_argument("--gpos",required=False,action="store_const",const=True,default=False,help="display gpos")
parser.add_argument("--all",required=False,action="store_const",const=True,default=False,help="display all types")
parser.add_argument("--csv",required=False,action="store_const",const=True,default=False,help="display results in CSV format")
args = parser.parse_args()

#
# Get the hostname of the drouter service
#
if args.hostname=="":
    try:
        args.hostname=os.environ["DROUTER_HOSTNAME"]
    except(KeyError):
        args.hostname="localhost"

if not args.sources:
    if not args.destinations:
        if not args.gpis:
            if not args.gpos:
                args.all=True

#
# Create a 'StateEngine' object to talk to the drouter service.
#
engine=Drouter.StateEngine.StateEngine()
engine.setPrivateObject(args)

#
# Set the "ready" callback so we receive notification when the engine
# has completed initialization.
#
engine.setReadyCallback(EngineReady)

#
# Start the engine, giving the hostname/address of the Drouter service.
#
engine.start(args.hostname)
