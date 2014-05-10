# vibrato table generator
#
# table[position][depth] = sin( position * pi*2 / 4 / 16 ) * 64 * depth / 16

from math import *

output = open( "vibratoTable.txt", "w" );
output.write( ";---------------------------------------------------------------------------------------------\n" );
output.write( "vibratoTable:\n" )
output.write( ";---------------------------------------------------------------------------------------------" );

for i in range(0, 256):
    pos = (i >> 4)
    depth = (i & 15)
    if( depth == 0 ):
        output.write( "\n\t.byte " )
    value = round(sin( pos * pi*2 / 4 / 16 ) * 64 * depth / 16)
    output.write( hex(  int(value)  ).replace("0x","").upper().zfill(2) + "h" )
    if( (i % 16) != 15 ):
        output.write( ", " )
