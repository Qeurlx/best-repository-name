; Random x86 Assembly Code Collection
; Various operations, functions, and patterns

section .data
    msg1 db 'Hello, World!', 0xA
    len1 equ $ - msg1
    num1 dd 0x12345678
    num2 dd 0xABCDEF00
    array1 times 10 dd 0
    counter dq 0
    buffer times 256 db 0
    flag db 0
    result dd 0
    temp dd 0
    string1 db 'Assembly', 0
    string2 db 'Programming', 0
    matrix times 16 dd 0
    lookup_table db 0,1,1,2,3,5,8,13,21,34

section .bss
    input_buffer resb 128
    output_buffer resb 128
    temp_storage resd 32
    stack_space resq 64

section .text
    global _start

_start:
    mov eax, 0x01
    mov ebx, 0x02
    add eax, ebx
    sub eax, 0x01
    xor ebx, ebx
    
    ; Arithmetic operations
    mov ecx, 100
    imul ecx, 5
    mov edx, 0
    div ecx
    inc eax
    dec ebx
    neg ecx
    
    ; Bitwise operations
    and eax, 0xFF
    or ebx, 0x0F
    xor ecx, ecx
    not edx
    shl eax, 2
    shr ebx, 1
    rol ecx, 4
    ror edx, 3
    
    ; Memory operations
    mov [num1], eax
    mov ebx, [num2]
    lea esi, [array1]
    mov [esi], eax
    mov [esi+4], ebx
    
    ; Stack operations
    push eax
    push ebx
    push ecx
    pop edx
    pop ecx
    pop ebx
    
    ; Comparison and jumps
    cmp eax, ebx
    je label1
    jne label2
    jg label3
    jl label4
    jge label5
    jle label6
    
label1:
    mov eax, 1
    jmp continue1
    
label2:
    mov eax, 2
    jmp continue1
    
label3:
    mov eax, 3
    jmp continue1
    
label4:
    mov eax, 4
    jmp continue1
    
label5:
    mov eax, 5
    jmp continue1
    
label6:
    mov eax, 6
    
continue1:
    ; Loop example
    mov ecx, 10
loop1:
    dec ecx
    cmp ecx, 0
    jne loop1
    
    ; String operations
    lea esi, [string1]
    lea edi, [buffer]
    mov ecx, 8
    rep movsb
    
    ; Call function
    call function1
    call function2
    call function3
    
    ; Exit program
    mov eax, 1
    xor ebx, ebx
    int 0x80

; Function 1: Calculate factorial
function1:
    push ebp
    mov ebp, esp
    push ebx
    push ecx
    
    mov eax, 1
    mov ecx, 5
factorial_loop:
    imul eax, ecx
    dec ecx
    cmp ecx, 0
    jg factorial_loop
    
    pop ecx
    pop ebx
    mov esp, ebp
    pop ebp
    ret

; Function 2: Sum array elements
function2:
    push ebp
    mov ebp, esp
    push esi
    push ecx
    
    lea esi, [array1]
    mov ecx, 10
    xor eax, eax
sum_loop:
    add eax, [esi]
    add esi, 4
    dec ecx
    cmp ecx, 0
    jg sum_loop
    
    mov [result], eax
    
    pop ecx
    pop esi
    mov esp, ebp
    pop ebp
    ret

; Function 3: Bit manipulation
function3:
    push ebp
    mov ebp, esp
    
    mov eax, 0xAAAAAAAA
    mov ebx, 0x55555555
    xor eax, ebx
    
    mov ecx, eax
    and ecx, 0x0F0F0F0F
    or ecx, 0xF0F0F0F0
    
    mov edx, 0x12345678
    bswap edx
    
    mov esp, ebp
    pop ebp
    ret

; Function 4: Bubble sort implementation
bubble_sort:
    push ebp
    mov ebp, esp
    push esi
    push edi
    push eax
    push ebx
    push ecx
    push edx
    
    mov esi, [ebp+8]    ; array pointer
    mov ecx, [ebp+12]   ; array length
    
outer_loop:
    dec ecx
    cmp ecx, 0
    jle sort_done
    
    mov edi, 0
    mov edx, ecx
    
inner_loop:
    mov eax, [esi+edi*4]
    mov ebx, [esi+edi*4+4]
    cmp eax, ebx
    jle no_swap
    
    mov [esi+edi*4], ebx
    mov [esi+edi*4+4], eax
    
no_swap:
    inc edi
    cmp edi, edx
    jl inner_loop
    jmp outer_loop
    
sort_done:
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop edi
    pop esi
    mov esp, ebp
    pop ebp
    ret

; Function 5: Matrix operations
matrix_multiply:
    push ebp
    mov ebp, esp
    push esi
    push edi
    push eax
    push ebx
    push ecx
    push edx
    
    mov esi, [ebp+8]    ; matrix A
    mov edi, [ebp+12]   ; matrix B
    mov eax, [ebp+16]   ; result matrix
    
    xor ebx, ebx        ; row counter
row_loop:
    cmp ebx, 4
    jge matrix_done
    
    xor ecx, ecx        ; column counter
col_loop:
    cmp ecx, 4
    jge next_row
    
    xor edx, edx
    push edx
    
    ; Dot product calculation
    mov edx, 0
dot_loop:
    cmp edx, 4
    jge store_result
    
    push eax
    push ebx
    
    mov eax, ebx
    shl eax, 2
    add eax, edx
    mov eax, [esi+eax*4]
    
    mov ebx, edx
    shl ebx, 2
    add ebx, ecx
    imul eax, [edi+ebx*4]
    
    pop ebx
    add [esp+4], eax
    pop eax
    
    inc edx
    jmp dot_loop
    
store_result:
    pop edx
    push eax
    mov eax, ebx
    shl eax, 2
    add eax, ecx
    pop eax
    mov [eax+eax*4], edx
    
    inc ecx
    jmp col_loop
    
next_row:
    inc ebx
    jmp row_loop
    
matrix_done:
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop edi
    pop esi
    mov esp, ebp
    pop ebp
    ret

; Function 6: String length calculation
strlen_func:
    push ebp
    mov ebp, esp
    push edi
    
    mov edi, [ebp+8]
    xor eax, eax
    xor ecx, ecx
    not ecx
    cld
    repne scasb
    not ecx
    dec ecx
    mov eax, ecx
    
    pop edi
    mov esp, ebp
    pop ebp
    ret

; Function 7: Memory copy
memcpy_func:
    push ebp
    mov ebp, esp
    push esi
    push edi
    push ecx
    
    mov edi, [ebp+8]    ; destination
    mov esi, [ebp+12]   ; source
    mov ecx, [ebp+16]   ; count
    
    cld
    rep movsb
    
    pop ecx
    pop edi
    pop esi
    mov esp, ebp
    pop ebp
    ret

; Function 8: Random number generator (LCG)
random_lcg:
    push ebp
    mov ebp, esp
    
    mov eax, [counter]
    imul eax, 1103515245
    add eax, 12345
    and eax, 0x7FFFFFFF
    mov [counter], eax
    
    mov esp, ebp
    pop ebp
    ret

; Function 9: Binary search
binary_search:
    push ebp
    mov ebp, esp
    push esi
    push ebx
    push ecx
    push edx
    
    mov esi, [ebp+8]    ; array
    mov ebx, [ebp+12]   ; length
    mov edx, [ebp+16]   ; search value
    
    xor eax, eax        ; left
    mov ecx, ebx        ; right
    dec ecx
    
search_loop:
    cmp eax, ecx
    jg not_found
    
    mov ebx, eax
    add ebx, ecx
    shr ebx, 1
    
    mov edi, [esi+ebx*4]
    cmp edi, edx
    je found
    jl search_right
    
search_left:
    mov ecx, ebx
    dec ecx
    jmp search_loop
    
search_right:
    mov eax, ebx
    inc eax
    jmp search_loop
    
found:
    mov eax, ebx
    jmp search_done
    
not_found:
    mov eax, -1
    
search_done:
    pop edx
    pop ecx
    pop ebx
    pop esi
    mov esp, ebp
    pop ebp
    ret

; Function 10: Fibonacci sequence
fibonacci:
    push ebp
    mov ebp, esp
    push ebx
    
    mov ecx, [ebp+8]
    cmp ecx, 0
    je fib_zero
    cmp ecx, 1
    je fib_one
    
    mov eax, 0
    mov ebx, 1
    dec ecx
    
fib_loop:
    mov edx, eax
    add edx, ebx
    mov eax, ebx
    mov ebx, edx
    dec ecx
    cmp ecx, 0
    jg fib_loop
    
    mov eax, ebx
    jmp fib_done
    
fib_zero:
    xor eax, eax
    jmp fib_done
    
fib_one:
    mov eax, 1
    
fib_done:
    pop ebx
    mov esp, ebp
    pop ebp
    ret

; More random operations
random_ops:
    bsf eax, ebx
    bsr ecx, edx
    bt eax, 5
    bts ebx, 7
    btr ecx, 3
    btc edx, 15
    
    movzx eax, bl
    movsx ebx, cx
    
    xchg eax, ebx
    xchg ecx, edx
    
    cmov eax, ebx
    cmove ecx, edx
    cmovne eax, ebx
    cmovg ecx, edx
    
    setne al
    sete bl
    setg cl
    setl dl
    
    test eax, eax
    test ebx, 0xFF
    test ecx, edx
    
    ret

; Advanced bit manipulation
bit_tricks:
    ; Count set bits
    mov eax, [num1]
    xor ecx, ecx
count_bits:
    test eax, eax
    jz count_done
    mov ebx, eax
    dec ebx
    and eax, ebx
    inc ecx
    jmp count_bits
count_done:
    
    ; Reverse bits
    mov eax, [num2]
    mov ecx, 32
    xor ebx, ebx
reverse_loop:
    shl ebx, 1
    shr eax, 1
    adc ebx, 0
    dec ecx
    jnz reverse_loop
    
    ; Check power of two
    mov eax, [num1]
    test eax, eax
    jz not_power
    mov ebx, eax
    dec ebx
    and eax, ebx
    jz is_power
not_power:
    xor eax, eax
    jmp power_done
is_power:
    mov eax, 1
power_done:
    
    ret
