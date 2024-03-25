from pwn import *
elf = ELF('./tmp/maze')
print("main =", hex(elf.symbols['main']))
print("{:<12s} {:<10s} {:<10s}".format("Func", "GOT Offset", "Symbol Offset"))
idx = 1
for s in [ f"move_{i}" for i in range(1200)]:
   if s in elf.got:
      print("{:<12s} {:<10x} {:<10x}".format(s, elf.got[s], elf.symbols[s]))
      #print("%s, " % (hex(elf.got[s] - elf.symbols['main'])), end="")