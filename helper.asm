section .text
global hello
hello:
    mov rax, 1
    mov rdi, 1
    mov rsi, hello_msg
    mov rdx, hello_msg_len
    syscall
    ret

global put_digit
put_digit:
    mov rax, rdi
    add rax, 0x30
    mov byte [rel digit_byte], al
    mov byte [rel digit_byte_new_line], 10
    mov rax, 1
    mov rdi, 1
    mov rsi, digit_byte
    mov rdx, 2
    syscall
    ret

global add_two
add_two:
    mov rax, rdi
    add rax, rsi
    ret

global strlen
strlen:
        push rbp
        mov rbp, rsp
        mov qword [rbp-24], rdi
        mov qword [rbp-8], 0
        jmp .L2
.L3:
        add qword [rbp-8], 1
.L2:
        mov rdx, qword [rbp-24]
        mov rax, qword [rbp-8]
        add rax, rdx
        movzx eax, byte [rax]
        test al, al
        jne .L3
        mov rax, qword [rbp-8]
        pop rbp
        ret

global print
print:
    mov rsi, rdi
    call strlen
    mov rdx, rax
    mov rax, 1
    mov rdi, 1
    syscall
    ret

section .data
hello_msg: db "Hello, world!",10
hello_msg_len: equ $ - hello_msg

section .bss
digit_byte: resb 1
digit_byte_new_line: resb 1
