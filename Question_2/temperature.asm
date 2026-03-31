section .data
    filename        db  "temperature_data.txt", 0
    err_msg         db  "Error: Could not open file.", 10
    err_len         equ $ - err_msg

    label_total     db  "Total readings: "
    len_total       equ $ - label_total
    label_valid     db  "Valid readings: "
    len_valid       equ $ - label_valid
    newline         db  10

section .bss
    fd              resq 1          ; File descriptor
    buffer          resb 4096       ; 4KB read buffer
    bytes_read      resq 1
    total_count     resq 1          ; X: Total entries
    valid_count     resq 1          ; Y: Non-empty readings
    num_str         resb 20         ; Buffer for integer to string conversion

section .text
    global _start

_start:
    ; -- File Handling: Open --
    ; sys_open (rax=2), filename (rdi), flags (rsi=0, O_RDONLY)
    mov rax, 2
    mov rdi, filename
    xor rsi, rsi
    syscall

    ; Check if file opened successfully
    test rax, rax
    js open_error
    mov [fd], rax

    ; -- File Handling: Read --
    ; sys_read (rax=0), fd (rdi), buffer (rsi), count (rdx)
    mov rax, 0
    mov rdi, [fd]
    mov rsi, buffer
    mov rdx, 4096
    syscall
    mov [bytes_read], rax

    ; -- Logic: String Traversal --
    xor rbx, rbx                    ; rbx = current index in buffer
    xor r12, r12                    ; r12 = Total entries (X)
    xor r13, r13                    ; r13 = Valid entries (Y)
    xor r14, r14                    ; r14 = Character count in current line (flag)

process_loop:
    cmp rbx, [bytes_read]
    je end_of_file

    mov al, [buffer + rbx]

    ; Check for CR (\r)
    cmp al, 13
    je next_char

    ; Check for LF (\n)
    cmp al, 10
    je line_break

    cmp al, 32
    ja mark_valid

next_char:
    inc rbx
    jmp process_loop

mark_valid:
    inc r14                         ; Found a "valid" character
    inc rbx
    jmp process_loop

line_break:
    inc r12
    test r14, r14
    jz reset_line
    inc r13

reset_line:
    xor r14, r14
    inc rbx
    jmp process_loop

end_of_file:
    test r14, r14
    jz print_results
    inc r12
    inc r13

    ; -- Output Generation --
print_results:
    ; Print "Total readings:"
    mov rax, 1
    mov rdi, 1
    mov rsi, label_total
    mov rdx, len_total
    syscall

    mov rdi, r12
    call print_number

    ; Print "Valid readings:"
    mov rax, 1
    mov rdi, 1
    mov rsi, label_valid
    mov rdx, len_valid
    syscall

    mov rdi, r13
    call print_number

    ; -- Control Flow: Termination --
    mov rax, 3                      ; sys_close
    mov rdi, [fd]
    syscall

exit_success:
    mov rax, 60                     ; sys_exit
    xor rdi, rdi
    syscall

open_error:
    mov rax, 1
    mov rdi, 2
    mov rsi, err_msg
    mov rdx, err_len
    syscall
    mov rax, 60
    mov rdi, 1
    syscall

; -- Helper: Integer to String --
print_number:
    mov rax, rdi
    mov rcx, num_str
    add rcx, 19
    mov byte [rcx], 10
    mov r8, 1

.convert_loop:
    xor rdx, rdx
    mov rbx, 10
    div rbx
    add dl, '0'                     ; Convert to ASCII
    dec rcx
    mov [rcx], dl
    inc r8
    test rax, rax
    jnz .convert_loop

    ; Print the converted string
    mov rax, 1
    mov rdi, 1
    mov rsi, rcx
    mov rdx, r8
    syscall
    ret
