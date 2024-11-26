#!/bin/bash

# Run all integrated tests in this directory
#
#   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as
#   published by the Free Software Foundation; either version 2 of
#   the License, or (at your option) any later version.
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

#
# General Tests
#
echo ======================================
echo  ProtocolJ - General Tests
echo ======================================
jtest $1 --hostname=localhost:9600 --connection-type=tcp --tests=tests_general.tst
echo

#
# Livewire Tests
#
echo ======================================
echo  ProtocolJ - Livewire Audio Tests
echo ======================================
jtest $1 --hostname=localhost:9600 --connection-type=tcp --tests=tests_livewire_audio.tst
echo

echo ======================================
echo  ProtocolJ - Livewire GPIO Tests
echo ======================================
jtest $1 --hostname=localhost:9600 --connection-type=tcp --tests=tests_livewire_gpio.tst
echo

#
# GVG7000 Tests
#
echo ======================================
echo  ProtocolJ - GVG7000 Audio Tests
echo ======================================
jtest $1 --hostname=localhost:9600 --connection-type=tcp --tests=tests_gvg7000_audio.tst
echo

#
# Action Tests
#
#echo ======================================
#echo  ProtocolJ - Action Tests
#echo ======================================
#jtest $1 --hostname=localhost:9600 --connection-type=tcp --tests=tests_action.tst
#echo
