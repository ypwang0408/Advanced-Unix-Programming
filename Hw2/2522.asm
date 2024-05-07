cmp ch, 0x60
jge l1
jmp l2
l1:
    sub ch, 0x20
    jmp end
l2:
    add ch, 0x20
    jmp end
end: