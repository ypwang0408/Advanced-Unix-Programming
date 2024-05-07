mov rsi, 0x600000
mov di, 0x8000
mov cx, 16
l:
    mov bx, ax
    and bx, di
    dec cx
    shr bx, cl
    shr di, 1
    add bx, 0x30
    mov [rsi], bl
    inc rsi
    cmp cx, 0
    jne l