#
import array as arr
#
def genArray(ar,tx):
    out="const uint16_t "+tx+"[194]={"
    for i in range(0,194):
        out+=str(ar[i])
        out+=str(",")
    out+="};\n"
    print(out)

patdef = arr.array('i')
patyellow = arr.array('i')
patdarkgreen = arr.array('i')
darkgreen=15*32 # Dark green
yellow=0xFFE0
white=0x07E0
for i in range(0,194):
    patdef.append(0)
    patyellow.append(yellow)
    patdarkgreen.append(darkgreen)
    if( (i/24)*24==i):
        patdef[i]=darkgreen
patdef[0]=    white
patdef[193]=    white
patdef[1+192/2] = darkgreen

genArray(patdef,"defaultPattern")
genArray(patyellow,"yellowPattern")
genArray(patdarkgreen,"darkGreenPattern")
#

