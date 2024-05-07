mov cx, 0
mov rsi, 0x600000
l1:
    mov dx, 0
    l2:
        mov ax, [rsi + rdx * 4]
        mov bx, [rsi + rdx * 4 + 4]
        cmp ax, bx
        jg greater
        l3:
        inc rdx
        cmp rdx, 9
        jl l2
        jmp end
    greater:
        mov [rsi + rdx * 4 + 4], ax
        mov [rsi + rdx * 4], bx
        jmp l3
    end:
    inc cx
    cmp cx, 10
    jne l1