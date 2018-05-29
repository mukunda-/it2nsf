# hello world of python
# tempo table tab[32..255] = round(4096 * 150 / i)

output = open("tempoTable.txt", "w")
output.write(";--------------------------------------------------------------------------------------------\n")
output.write("tempoTableL:\n")
output.write(";--------------------------------------------------------------------------------------------")

for i in range(32, 256):

    if ((i - 32) % 16 == 0):
        output.write("\n\t.byte ")

    value = int(round(4096.0 * 150.0 / i)) % 256

    output.write(hex(int(value)).replace("0x", "").upper().zfill(3) + "h")

    if ((i - 32) % 16 != 15):
        output.write(", ")

output.write("\n;--------------------------------------------------------------------------------------------\n")
output.write("tempoTableH:\n")
output.write(";--------------------------------------------------------------------------------------------")

for i in range(32, 256):

    if ((i - 32) % 16 == 0):
        output.write("\n\t.byte ")

    value = int(round(4096.0 * 150.0 / i)) / 256

    output.write(hex(int(value)).replace("0x", "").upper().zfill(3) + "h")

    if ((i - 32) % 16 != 15):
        output.write(", ")

output.write("\n")
output.close()
