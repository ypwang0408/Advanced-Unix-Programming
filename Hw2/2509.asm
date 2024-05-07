mov rsi, 0x600000
mov cl, 15
l:
    mov al, 0x0
    mov al, [rsi]
    cmp al, 'A'
    jge l1
    jmp l3
l1:
    cmp al, 'Z'
    jle l2
    jmp l3
l2:
    sub al, 0x20
    jmp l3
l3:
    add al, 0x20
    mov [rsi + 0x10], al
    inc rsi
    dec cl
    jnz l  