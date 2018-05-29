# hello, world of python!
# period table generator

output = open("periodTable.txt", "w")
output.write(";---------------------------------------------------------------------------------------------\n")
output.write("periodTableL: ; ptab[0..191] = 4095 / 2^(i/192)\n")
output.write(";---------------------------------------------------------------------------------------------")

NES_CLK = 21477270.0 / 12.0

for i in range(0, 193):
    if ((i % 16) == 0):
        output.write("\n\t.byte ")

    value = int(round(4095 / 2.0 ** (i / 192.0))) % 256

    output.write(hex(int(value)).replace("0x", "").upper().zfill(3) + "h")
    if ((i % 16) != 15):
        output.write(", ")

output.write("\n;---------------------------------------------------------------------------------------------\n")
output.write("periodTableH:\n")
output.write(";---------------------------------------------------------------------------------------------")

for i in range(0, 193):
    if ((i % 16) == 0):
        output.write("\n\t.byte ")

    value = int(round((NES_CLK / 8363.0) * 2.0 ** 5.0 / 2.0 ** (i / 192.0))) >> 8

    output.write(hex(int(value)).replace("0x", "").upper().zfill(2) + "h")
    if ((i % 16) != 15):
        output.write(", ")

output.write("\n\n")

output.close()
