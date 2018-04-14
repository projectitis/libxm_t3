from glob import glob
from os import path
from PIL import Image

###
### Compile XM module to c header files
###

# This code is available under the MIT license
# Copyright (c) 2018 Peter Vullings (Projectitis)
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Step through all XM files in this folder and prepare
# a header file for each so that the resource may be compiled
# directly into the program.

# Get list of XM files in this folder
resources = glob('./*.xm')
for file in list(resources):

	# Split the filename by dot to get the parts
	filename = path.basename(file)
	fileparts = filename.split('.')
	if len(fileparts) < 2:
		continue
	
	# first and last parts of filename are name and extn
	name = fileparts.pop(0)
	extn = fileparts.pop().lower();
	
	# Start the output
	outstr = '';
	
	# Some useful messages to console
	print()
	print('Processing',filename,'(mod)')
	
	# Open raw file and step bytes
	c = 1
	b = 1
	with open(file, "rb") as f:
		byte = f.read(1)
		outstr += ' {:3d}'.format(byte[0])
		while byte:
			byte = f.read(1)
			if byte:
				outstr += ',{:3d}'.format(byte[0])
				c += 1
				b += 1
				if c == 64:
					outstr += '\n'
					c = 0
	
	# Generate file header
	outstr = ('// Tracker module as byte array\n'
		'const uint32_t '+name+'_size = '+str(b)+';\n'
		'const char '+name+'[] = {\n' + outstr)	
					
	# footer
	outstr += '};\n'
	
	# console
	print(b,'bytes in output');
				
	### Write to file
	if outstr:
		outfile = open('./'+name+'.h', 'w')
		outfile.write(outstr)
		outfile.close()			
				
# Wait before exit for user to read messages
print()
input("Press enter to exit")