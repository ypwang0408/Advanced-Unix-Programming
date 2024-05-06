cmp eax, 0
jge .p1
mov eax, -1
jmp .p2
.p1:
mov eax, 1
.p2:
cmp ebx, 0
jge .p3
mov ebx, -1
jmp .p4
.p3:
mov ebx, 1
.p4:
cmp ecx, 0
jge .p5
mov ecx, -1
jmp .p6
.p5:
mov ecx, 1
.p6:
cmp edx, 0
jge .p7
mov edx, -1
jmp .p8
.p7:
mov edx, 1
.p8:
mov [0x600000], eax
mov [0x600004], ebx
mov [0x600008], ecx
mov [0x60000c], edx