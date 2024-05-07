mov eax, [0x600004]
neg eax
mov ebx, [0x600008]
cdq
idiv ebx
mov ebx, edx
mov eax, [0x600000]
imul eax, -5
cdq
idiv ebx
mov [0x60000c], eax