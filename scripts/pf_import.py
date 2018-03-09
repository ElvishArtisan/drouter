#!/usr/bin/python

# pf_import.py
#
# Generate a drouter map from an existing Software Authority Protocol service
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
import socket
import sys

class Endpoint(object):
    def __init__(self,host_addr,slot):
        self.__hostAddress=host_addr
        self.__slotNumber=int(slot)

    def hostAddress(self):
        return self.__hostAddress

    def slotNumber(self):
        return self.__slotNumber

class SAParser(object):
    def __init__(self,username,password,router):
        self.username=username
        self.password=password
        self.router=router
        self.istate=0
        self.accum=""
        self.sock=socket.socket(socket.AF_INET)
        self.inputs={}
        self.max_inputs=0
        self.outputs={}
        self.max_outputs=0

    def run(self,hostname):
        conn=self.sock.connect((hostname,9500))
        self.sock.send("Login "+self.username+" "+self.password+"\r\n")
        while self.istate<5:
            c=self.sock.recv(1)
            if c[0]=="\r":
                self.processMessage(self.accum)
                self.accum=""
            else:
                if c[0]!="\n":
                    self.accum+=c
        return (self.inputs,self.max_inputs,self.outputs,self.max_outputs)

    def processMessage(self,cmd):
        if self.istate==0:  # Process login
            if cmd=="Login Successful":
                self.sock.send("SourceNames "+str(self.router)+"\r\n")
                self.istate=1
                return
            else:
                print "Login failed!"
                sys.exit(1)

        if self.istate==1:  # Start of sources
            if cmd==">>Begin SourceNames - "+str(self.router):
                self.istate=2
                return
            else:
                print "Parser error (istate="+str(self.istate)+", cmd=\""+cmd+"\")"
                sys.exit(1)

        if self.istate==2:  # Sources
            if cmd=="End SourceNames - "+str(self.router):
                self.sock.send("DestNames "+str(self.router)+"\r\n")
                self.istate=3
                return
            else:
                cmds=cmd.split("\t")
                self.inputs[int(cmds[0])]=Endpoint(cmds[3],cmds[5])
                if int(cmds[0])>self.max_inputs:
                    self.max_inputs=int(cmds[0])

        if self.istate==3:  # Start of destinations
            if cmd==">>Begin DestNames - "+str(self.router):
                self.istate=4
                return
            else:
                print "Parser error (istate="+str(self.istate)+", cmd=\""+cmd+"\")"
                sys.exit(1)

        if self.istate==4:  # destinations
            if cmd=="End DestNames - "+str(self.router):
                self.istate=5
            else:
                cmds=cmd.split("\t")
                self.outputs[int(cmds[0])]=Endpoint(cmds[3],cmds[5])
                if int(cmds[0])>self.max_outputs:
                    self.max_outputs=int(cmds[0])

#
# Read the arguments
#                
parser=argparse.ArgumentParser(description='Generate a drouter map from an existing PathFinder server')
parser.add_argument("--pf-hostname",required=True,help="hostname of PathFinder server")
parser.add_argument("--pf-username",required=True,help="username to log into PathFinder server")
parser.add_argument("--pf-password",required=True,help="password to log into PathFinder server")
parser.add_argument("--pf-router-number",required=True,help="source router number on PathFinder server")
parser.add_argument("--router-type",required=True,choices=["audio","gpio"],help="type of router map to create")
parser.add_argument("--router-number",required=True,help="router number to create")
parser.add_argument("--router-name",required=True,help="name of router to create")
parser.add_argument("--output-file",required=True,help="file to save map to");
args = parser.parse_args()

#
# Read the existing server
#
parser=SAParser(args.pf_username,args.pf_password,int(args.pf_router_number))
(inputs,max_inputs,outputs,max_outputs)=parser.run(args.pf_hostname)

#
# Write the map
#
file=open(args.output_file,"w")
file.write("[Global]\n")
file.write("RouterType="+args.router_type+"\n")
file.write("RouterName="+args.router_name+"\n")
file.write("RouterNumber="+args.router_number+"\n")
file.write("\n")
for input in range(1,max_inputs+1):
    file.write("[Input"+str(input)+"]\n")
    try:
        file.write("HostAddress="+inputs[input].hostAddress()+"\n")
        file.write("Slot="+str(inputs[input].slotNumber())+"\n")
    except(KeyError):
        file.write("HostAddress=0.0.0.0\n")
        file.write("Slot=0\n")
    file.write("\n")
for output in range(1,max_outputs+1):
    file.write("[Output"+str(output)+"]\n")
    try:
        file.write("HostAddress="+outputs[output].hostAddress()+"\n")
        file.write("Slot="+str(outputs[output].slotNumber())+"\n")
    except(KeyError):
        file.write("HostAddress=0.0.0.0\n")
        file.write("Slot=0\n")
    file.write("\n")
file.close()
