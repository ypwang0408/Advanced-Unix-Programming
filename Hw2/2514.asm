mov eax, [0x600008]
sub eax, ebx
mov ebx, eax
mov eax, [0x600000]
mov ecx, [0x600004]
neg ecx
imul ecx
cdq
idiv ebx
mov [0x600008], eax