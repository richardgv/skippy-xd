#!/usr/bin/python

import os
import sys

filename = "environment.h"

os.system("rm -f " + filename)

long_size_proc = os.popen("getconf LONG_BIT")
long_size = long_size_proc.read().rstrip()

fileheader = '''/* Skippy - Seduces Kids Into Perversion
 *
 * Copyright (C) 2004 Hyriand <hyriand@thegraveyard.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H
'''

filefooter = '''
#endif
'''

if long_size == "64":
	architecture_constants = '''
#define ENVIRONMENT_64_BIT 1
'''
elif long_size == "32":
	architecture_constants = '''
#define ENVIRONMENT_32_BIT = 1;
'''
else:
	print("Could not determine architecture, \"getconf LONG_BIT\" returned neither \"32\" nor \"64\"")
	sys.exit(1)

print("Outputing environment.h for " + long_size + "-bit environment.")
file_handle = open(filename, "w")
file_handle.write(fileheader + architecture_constants + filefooter)
file_handle.close()

sys.exit(0)
