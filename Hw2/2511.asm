mov eax, [0x600000]
imul eax, -1
mov ecx, [0x600004]
mul ecx
add eax, [0x600008]
