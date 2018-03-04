#!/usr/bin/python

# show_changes.py
#
# Use the drouter Python API to show changes in a Livewire network in realtime.
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

import drouter.dparser

# ############################################################################
#
# Callbacks
#
#  These are called by the 'dparser' object in response to specific events.
#

#
# Called immediately after the 'dparser' object has completed initialization.
# This is the place to do any needed startup initialization (create objects,
# open connections, etc).
#
def RouterReady(dparser):
    print "RouterReady() ran!"


#
# Called every time a new object -- a node, source, destination or GPIO
# resource -- is added to the system.
#
def ObjectAdded(dparser,type,object):
    if type=="NODE":
        print "ADDED NODE"
        print object

    if type=="SRC":
        print "ADDED SRC"
        print object

    if type=="DST":
        print "ADDED DST"
        print object

    if type=="GPI":
        print "ADDED GPI"
        print object

    if type=="GPO":
        print "ADDED GPO"
        print object

#
# Called immediately before an object -- a node, source, destination or GPIO
# resource -- is removed from the system.
#
def ObjectDeleted(dparser,type,object):
    if type=="NODE":
        print "DELETED NODE"
        print object

    if type=="SRC":
        print "DELETED SRC"
        print object

    if type=="DST":
        print "DELETED DST"
        print object

    if type=="GPI":
        print "DELETED GPI"
        print object

    if type=="GPO":
        print "DELETED GPO"
        print object

#
# Called whenever an object -- a node, source, destination or GPIO resource --
# reports a change to its configuration or state.
#
def ObjectChanged(dparser,type,old,new):
    if type=="NODE":
        print "OLD NODE"
        print old
        print "NEW NODE"
        print new

    if type=="SRC":
        print "OLD SRC"
        print old
        print "NEW SRC"
        print new

    if type=="DST":
        print "OLD DST"
        print old
        print "NEW DST"
        print new

    if type=="GPI":
        print "OLD GPI"
        print old
        print "NEW GPI"
        print new

    if type=="GPO":
        print "OLD GPO"
        print old
        print "NEW GPO"
        print new

#
# Called whenever an audio alarm -- a SILENCE or a CLIP -- changes state.
#
def Alarm(dparser,alarm):
    print "ALARM"
    print alarm 

# ############################################################################
#
# Main Loop
#
# Create a 'dparser' object to talk to the drouter service.
#
parser=drouter.dparser.dparser()

#
# Set the callbacks so we receive notifications of changes.
#
parser.setReadyCallback(RouterReady)
parser.setAddCallback(ObjectAdded)
parser.setDeleteCallback(ObjectDeleted)
parser.setChangeCallback(ObjectChanged)
parser.setAlarmCallback(Alarm)

#
# Start the loop, giving the hostname/address of the drouter service.
#
parser.start("localhost")
