# hello world of python
# vrc6 sawtooth volume map (0..15 -> 0..42)

output = open("sawVolumeMap.txt", "w")
output.write(";--------------------------------------------------------------------------------------------\n")
output.write("@sawVolumeMap:\n")
output.write(";--------------------------------------------------------------------------------------------\n")
output.write("\t.byte ")

for i in range(0, 16):

    value = int(round(i * 42.0 / 15.0))

    output.write(hex(int(value)).replace("0x", "").upper().zfill(2) + "h")

    if (i != 15):
        output.write(", ")

output.write("\n")
output.close()
