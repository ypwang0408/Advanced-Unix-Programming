mov rdi, 18
call r_func
jmp exit

r_func:
    push rbp
    mov rbp, rsp
    push rdi

    cmp rdi, 0
    jle case_0
    cmp rdi, 1
    je case_1

    dec rdi
    call r_func
    sub rbp, 4
    push rax

    dec rdi
    call r_func
    sub rbp, 4
    push rax

    pop rdx
    add rbp, 4
    pop rcx
    add rbp, 4
    mov rax, rcx
    add rax, rcx
    add rax, rdx
    add rax, rdx
    add rax, rdx
    jmp end

case_0:
    mov rax, 0
    jmp end

case_1:
    mov rax, 1
    jmp end

end:
    pop rdi
    mov rsp, rbp
    pop rbp
    ret
exit: