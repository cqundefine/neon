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

section .data
hello_msg: db "Hello, world!",10
hello_msg_len: equ $ - hello_msg

section .bss
digit_byte: resb 1
digit_byte_new_line: resb 1
