# hello, world of python!
#
# frequency table generator

output = open("frequencyTable.txt", "w")
output.write(";---------------------------------------------------------------------------------------------\n")
output.write("frequencyTableL: ; ftab[0..192] = (32767/2) * 2^(i/192)\n")
output.write(";---------------------------------------------------------------------------------------------")

for i in range(0,193):
    if( (i % 16) == 0 ):
        output.write( "\n\t.byte " )
    
    value = int(round(  (32767/2) * 2**(i/192.0) )) % 256

    output.write( hex(  int(value)  ).replace("0x","").upper().zfill(3) + "h" )
    if( (i % 16) != 15 ):
        output.write( ", " )

output.write("\n;---------------------------------------------------------------------------------------------\n")
output.write("frequencyTableH:\n")
output.write(";---------------------------------------------------------------------------------------------")

for i in range(0,193):
    if( (i % 16) == 0 ):
        output.write( "\n\t.byte " )
    
    value = int(round(  (32767/2) * 2**(i/192.0) )) >> 8

    output.write( hex(  int(value)  ).replace("0x","").upper().zfill(2) + "h" )
    if( (i % 16) != 15 ):
        output.write( ", " )

output.write( "\n\n" )

output.close()

