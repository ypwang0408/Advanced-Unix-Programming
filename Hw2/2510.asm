mov eax, [0x600000]
add eax, [0x600004]
mov ecx, [0x600008]
mul ecx
mov [0x60000c], eax