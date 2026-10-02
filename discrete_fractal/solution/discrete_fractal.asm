%define SYS_READ         0
%define SYS_WRITE        1
%define SYS_MMAP         9
%define SYS_MUNMAP       11
%define SYS_MREMAP       25
%define SYS_EXIT         60

%define PROT_READ        1
%define PROT_WRITE       2

%define MAP_PRIVATE      2
%define MAP_ANONYMOUS    32

%define MREMAP_MAYMOVE   1

%define MIN_SYMBOL       33
%define MAX_SYMBOL       126

%define OUT_BUF_SIZE     1048576
%define READ_BUF_SIZE    1048576
%define INIT_BUF_SIZE    4096

%define UINT32_MAX_DIV10 429496729
%define UINT32_MAX_MOD10 5

%define FRAME_PTR          0
%define FRAME_LEN          8
%define FRAME_POS          16
%define FRAME_DEPTH        24
%define FRAME_SIZE         32
%define STACK_INIT_FRAMES 1024

section .data

out_cap:        dq OUT_BUF_SIZE
line_cap:       dq INIT_BUF_SIZE
word_cap:       dq INIT_BUF_SIZE
rules_cap:      dq INIT_BUF_SIZE
stack_cap:      dq STACK_INIT_FRAMES

section .bss

erase_depth:    resb 128
; erase_depth[c] = 0  -> We don't know wheter c vanishes.
; erase_depth[c] = k  -> sc vanishes after k iterations.

n_value:        resq 1

rule_seen:      resb 128
rule_ptr:       resq 128
rule_len:       resq 128

word_ptr:       resq 1
word_len:       resq 1

rules_ptr:      resq 1
rules_len:      resq 1

out_ptr:        resq 1
out_len:        resq 1

read_ptr:       resq 1

stack_ptr:      resq 1
stack_len:      resq 1

line_ptr:       resq 1
line_len:       resq 1

section .text
global _start

; Cleanup returns r14b = 1 if any munmap failed.
cleanup:
    xor     r14d, r14d          ; cleanup_failed = 0

    mov     rdi, [out_ptr]
    mov     rsi, [out_cap]
    call    free_mem

    mov     rdi, [read_ptr]
    mov     esi, READ_BUF_SIZE
    call    free_mem

    mov     rdi, [line_ptr]
    mov     rsi, [line_cap]
    call    free_mem

    mov     rdi, [word_ptr]
    mov     rsi, [word_cap]
    call    free_mem

    mov     rdi, [rules_ptr]
    mov     rsi, [rules_cap]
    call    free_mem

    mov     rdi, [stack_ptr]
    mov     rsi, [stack_cap]
    shl     rsi, 5
    jnc     .free_stack

    mov     r14b, 1
    jmp     .done

.free_stack:
    call    free_mem

.done:
    ret

; Append bytes to a growable buffer.
; In:
;   rdi = &ptr, rsi = &len, rdx = &cap
;   r8  = src,  r9  = src_len
append_to_buffer:
    test    r9, r9
    je      .done

    mov     rbx, rsi        ; rbx = &len

    mov     rax, [rbx]      ; rax = len
    add     rax, r9         ; rax = needed = len + src_len
    jc      error

    cmp     rax, [rdx]      ; needed <= cap?
    jbe     .copy

    mov     rcx, [rdx]      ; rcx = old_cap
    add     rcx, rcx        ; rcx = 2 * old_cap
    jc      error
    cmp     rcx, rax
    cmovb   rcx, rax        ; rcx = max(2 * old_cap, needed)

    push    rdi             ; save &ptr
    push    rdx             ; save &cap
    push    r8              ; save src
    push    r9              ; save src_len
    push    rcx             ; save new_cap

    mov     rdi, [rdi]      ; old_ptr
    mov     rsi, [rdx]      ; old_cap
    mov     rdx, rcx        ; new_cap
    call    grow

    pop     rcx             ; new_cap
    pop     r9              ; src_len
    pop     r8              ; src
    pop     rdx             ; &cap
    pop     rdi             ; &ptr

    mov     [rdi], rax      ; *ptr = new_ptr
    mov     [rdx], rcx      ; *cap = new_cap

.copy:
    mov     rdi, [rdi]      ; rdi = ptr
    add     rdi, [rbx]      ; rdi = ptr + len
    mov     rsi, r8         ; rsi = src
    mov     rcx, r9         ; rcx = src_len
    rep     movsb

    add     [rbx], r9       ; len += src_len

.done:
    ret

; Adds a frame to the stack with following parameters:
; rdi = address of fragment we want to generate,
; rsi = length of the fragment,
; rdx = depth.
push_frame:
    test    rsi, rsi
    je      .done

    mov     rax, [stack_len]    
    cmp     rax, [stack_cap]
    jb      .has_space

; Grow the stack.
    mov     rcx, [stack_cap]
    add     rcx, rcx        ; new_cap = 2 * old_cap
    jc      error           ; Overflow, cannot represent larger stack.

    push    rdi             ; Pointer to new frame.
    push    rsi             ; Length of new frame.
    push    rdx             ; Depth of new frame.
    push    rcx             ; New frame capacity.

    mov     rdi, [stack_ptr]
    mov     rsi, [stack_cap]
    shl     rsi, 5          ; Size in bytes, frame_size = 32.
    jc      error

    mov     rdx, rcx        ; New size.
    shl     rdx, 5
    jc      error   
    call    grow            ; mremap(old_ptr, old_size, new_size, MREMAP_MAYMOVE)

    pop     rcx
    pop     rdx
    pop     rsi
    pop     rdi

    mov     [stack_ptr], rax  
    mov     [stack_cap], rcx
  
.has_space:
    mov     rax, [stack_len]  ; Number of frames on stack.
    shl     rax, 5       
    add     rax, [stack_ptr]  ; New frame address.

    mov     [rax + FRAME_PTR], rdi 
    mov     [rax + FRAME_LEN], rsi  
    mov     qword [rax + FRAME_POS], 0 
    mov     [rax + FRAME_DEPTH], rdx  

    inc     qword [stack_len]     
.done:
    ret

; Process current line in line_buf.
; r15 == 0 means initial word, otherwise replacement rule.
process_line:
    test    r15, r15
    jnz     .rule_line

.word_line:
    mov     rdi, [line_ptr]
    mov     rcx, [line_len]
    call    validate_symbols

    lea     rdi, [rel word_ptr]
    lea     rsi, [rel word_len]
    lea     rdx, [rel word_cap]
    mov     r8, [line_ptr]
    mov     r9, [line_len]
    call    append_to_buffer
    ret

.rule_line:
    cmp     qword [line_len], 0
    je      error

    mov     rdi, [line_ptr]
    mov     rcx, [line_len]
    call    validate_symbols

    mov     rbx, [line_ptr]
    movzx   r8d, byte [rbx]          ; lhs = first symbol

    cmp     byte [rule_seen + r8], 0
    jne     error
    mov     byte [rule_seen + r8], 1

    mov     r10, [rules_len]         ; rhs offset
    mov     r11, [line_len]
    dec     r11                      ; rhs length

    mov     [rule_ptr + 8*r8], r10
    mov     [rule_len + 8*r8], r11

    lea     rdi, [rel rules_ptr]
    lea     rsi, [rel rules_len]
    lea     rdx, [rel rules_cap]
    mov     r8, [line_ptr]
    inc     r8                       ; rhs start = line_ptr + 1
    mov     r9, r11
    call    append_to_buffer

    ret

; Validate that all bytes are allowed symbols.
validate_symbols:
.loop:
    test    rcx, rcx                 ; length
    je      .done

    movzx   eax, byte [rdi] 

    sub     eax, MIN_SYMBOL          ; Valid iff 0 <= c-MIN <= MAX-MIN
    cmp     eax, MAX_SYMBOL - MIN_SYMBOL
    ja      error

    inc     rdi 
    dec     rcx
    jmp     .loop
.done:
    ret

alloc:
    xor     edi, edi
    mov     edx, PROT_READ | PROT_WRITE
    mov     r10d, MAP_PRIVATE | MAP_ANONYMOUS
    mov     r8d, -1
    xor     r9d, r9d
    mov     eax, SYS_MMAP
    syscall
    cmp     rax, -4096
    ja      error
    ret

grow:
    mov     r10d, MREMAP_MAYMOVE
    mov     eax, SYS_MREMAP
    syscall
    cmp     rax, -4096
    ja      error
    ret

free_mem:
    test    rdi, rdi
    je      .done
    test    rsi, rsi
    je      .done
    mov     eax, SYS_MUNMAP
    syscall
    cmp     rax, -4096
    jbe     .done
    mov     r14b, 1
.done:
    ret

; Checks the correctness of n and reads it into rax.
; Accepts only decimal digits and values <= UINT32_MAX.
; In:
;   rsi = pointer to argv[1]
parse_n:    
    xor     eax, eax                ; Result accumulator.

    cmp     byte [rsi], 0           ; n is empty.
    je      error
.loop:
    movzx   ecx, byte [rsi]         ; Current symbol.
    test    ecx, ecx
    je      .done

    sub     ecx, '0'
    cmp     ecx, 9
    ja      error

    cmp     rax, UINT32_MAX_DIV10   ; Is it possibleto add another number?
    ja      error
    jb      .safe

    cmp     rcx, UINT32_MAX_MOD10
    ja      error
.safe:
    imul    rax, rax, 10            ; We don't want to modify rdx.
    add     rax, rcx                ; Add the last number.

    inc     rsi                     ; Go to next character.
    jmp     .loop
.done:
    ret

; Writes the current output buffer to stdout.
; Uses:
;   rax, rdi, rsi, rdx, r8, r9, rcx, r11, rbx
flush_out:
    mov     r8, [out_len]           ; Number of remaining bytes.
    test    r8, r8
    je      .done

    mov     r9, [out_ptr]           ; Current pointer.

.write_loop:
    mov     eax, SYS_WRITE
    mov     edi, 1                  ; stdout
    mov     rsi, r9
    mov     rdx, r8
    syscall                         ; write(1, r9, r8)

    cmp     rax, -4096
    ja      error
    test    rax, rax
    jz      error

    add     r9, rax                 ; ptr += written
    sub     r8, rax                 ; remaining -= written
    jne     .write_loop

    mov     qword [out_len], 0
.done:
    ret

; Append one byte to the output buffer.
; In:
;   bl = byte to append
; Uses:
;   rax, rcx, rdi, rsi, rdx, r8, r9, r11, rbx
emit_char:
    mov     rax, [out_len]
    cmp     rax, [out_cap]
    jb      .has_space

    call    flush_out
    xor     eax, eax              ; after flush, out_len = 0

.has_space:
    mov     rcx, [out_ptr]
    mov     [rcx + rax], bl     
    inc     rax
    mov     [out_len], rax
    ret   

; Append bytes to the output buffer.
; In:
;   rdi = src, rsi = len
; Uses:
;   rax, rcx, rdi, rsi, rdx, r8, r9, r11, r12, r13, r14
emit_bytes:
.loop:
    test    r13, r13
    je      .done

    mov     rax, [out_cap]
    sub     rax, [out_len]      ; rax = free space

    test    rax, rax
    jne     .have_space

    call    flush_out
    jmp     .loop
.have_space:                    ; take = min(len, free)
    mov     r14, r13            ; take = length
    cmp     r14, rax
    jbe     .take_ok
    mov     r14, rax            ; take = free

.take_ok:
; dst = out_ptr + out_len

    mov     rdi, [out_ptr]
    add     rdi, [out_len]      ; destination
    mov     rsi, r12            ; src
    mov     rcx, r14            ; count
    rep     movsb               ; copy rcx bytes from [rsi] to [rdi]

    add     [out_len], r14      ; out_len += take
    add     r12, r14            ; src += take
    sub     r13, r14            ; len -= take

    jmp     .loop
.done:
    ret

_start:
    cld

    cmp     qword [rsp], 2
    jne     error

    mov     rsi, [rsp + 16]
    call    parse_n
    mov     [n_value], rax

; Initialize buffers:
    xor     r15d, r15d           ; line_num = 0

    mov     esi, OUT_BUF_SIZE    ; Buffer for output.
    call    alloc
    mov     [out_ptr], rax       ; Ptr to buffer.

    mov     esi, READ_BUF_SIZE   ; Buffer for read.
    call    alloc
    mov     [read_ptr], rax             

    mov     esi, INIT_BUF_SIZE   ; Buffer for line-read.
    call    alloc
    mov     [line_ptr], rax             

    mov     esi, INIT_BUF_SIZE   ; Buffer for word-read.
    call    alloc
    mov     [word_ptr], rax

    mov     esi, INIT_BUF_SIZE   ; Buffer for rules.
    call    alloc
    mov     [rules_ptr], rax

    mov     esi, STACK_INIT_FRAMES * FRAME_SIZE
    call    alloc
    mov     [stack_ptr], rax

; Input buffer:    
read_loop:
    mov     eax, SYS_READ
    xor     edi, edi
    mov     rsi, [read_ptr]
    mov     edx, READ_BUF_SIZE
    syscall

    cmp     rax, -4096
    ja      error

    test    rax, rax              ; No progress.
    jz      eof

    mov     r12, [read_ptr]       ; Current position.
    lea     r13, [r12 + rax]      ; End of read block.
    mov     r8, r12               ; Start of current chunk.

scan_loop:
    cmp     r12, r13              ; Is it the end of block?
    jae     end_of_block

    cmp     byte [r12], 10        ; Is it '\n'?
    je      newline

    inc     r12
    jmp     scan_loop

newline:
    ; append chunk [r8, r12) to line buffer
    lea     rdi, [rel line_ptr]
    lea     rsi, [rel line_len]
    lea     rdx, [rel line_cap]
    mov     r9, r12
    sub     r9, r8                ; Chunk length.
    call    append_to_buffer

    call    process_line

    mov     qword [line_len], 0
    inc     r15

    inc     r12                   ; skip '\n'
    mov     r8, r12               ; Next chunk starts after newline.
    jmp     scan_loop

end_of_block:
    ; Append remaining chunk [r8, r13) to line buffer
    lea     rdi, [rel line_ptr]
    lea     rsi, [rel line_len]
    lea     rdx, [rel line_cap]
    mov     r9, r13
    sub     r9, r8                ; Chunk length.
    call    append_to_buffer

    jmp     read_loop

eof:
    cmp     qword [line_len], 0
    jne     error

    test    r15, r15
    jz      error

    mov     r15d, 128             ; Max 128 rounds accounts for ASCII 0..127.

; How many iterations to vanish a symbol:
; erase_depth[c] = 0 means "unknown / does not disappear".
; erase_depth[c] = k means "symbol c disappears after k iterations".
outer:
    xor     r14d, r14d            ; No new disappearing symbol discovered.
    mov     ebx, MAX_SYMBOL               

symbol_loop:
    cmp     ebx, MIN_SYMBOL
    jb      round_done


    cmp     byte [rule_seen + rbx], 0 
    je      next_symbol          ; If a symbol doesn't have a replacement rule
                                 ; it cannot disappear.

    cmp     byte [erase_depth + rbx], 0
    jne     next_symbol           ; We already know when it disappears.

    mov     rsi, [rule_len + 8*rbx] 
                                  ; rsi = length of RHS of rule c.
    test    rsi, rsi
    jne     nonempty_rhs

    mov     byte [erase_depth + rbx], 1
                                  ; Empty rule.
    mov     r14b, 1
    jmp     next_symbol

nonempty_rhs:
    mov     rdi, [rules_ptr]      ; rdi =  RHS address
    add     rdi, [rule_ptr + 8*rbx]
                                  ; rule_ptr stores an offset from rules_ptr.

    xor     r11d, r11d            ; max_depth = 0
    xor     ecx, ecx              ; i = 0

rhs_loop:
    cmp     rcx, rsi              ; Have we checked all RHS symbols?
    jae     rhs_all_erasable

    movzx   eax, byte [rdi + rcx] 
                                  ; eax = RHS[i], current symbol. 
    movzx   edx, byte [erase_depth + rax]
                                  ; edx = erase_depth[RHS[i]].                
    test    edx, edx
    je      next_symbol

    ; max_depth = max(max_depth, erase_depth[rhs[i]])
    cmp     edx, r11d
    jbe     rhs_next
    mov     r11d, edx

rhs_next:
    inc     rcx
    jmp     rhs_loop

; If a symbol disappears one iteration after
; all symbols in RHS disappear:
; erase_depth[c] = 1 + max_depth.
rhs_all_erasable:
    inc     r11d
    mov     byte [erase_depth + rbx], r11b
    mov     r14b, 1

next_symbol:
    dec     ebx
    jmp     symbol_loop

round_done:
    test    r14b, r14b
    je      generate

    dec     r15d
    jne     outer

; Stack with frames.
; frame -> expand(fragment[pos..len-1], depth)
generate:                    
    mov     rdi, [word_ptr]     
    mov     rsi, [word_len]
    mov     rdx, [n_value]
    call    push_frame               ; Push the first frame.
.loop:
    mov     rax, [stack_len]         ; Frames count on stack.
    test    rax, rax
    jz      .finish

    dec     rax                      ; stack_len - 1
    shl     rax, 5                   ; rax *= 32, because FRAME_SIZE = 32
    add     rax, [stack_ptr]         ; Top frame address.

    mov     rdx, [rax + FRAME_DEPTH] ; Top frame depth.
    test    rdx, rdx
    jnz     .depth_nonzero
.depth_zero:
    mov     r12, [rax + FRAME_POS]    ; Position in the word to output.

    mov     r13, [rax + FRAME_LEN]    ; rsi = len - pos
    sub     r13, r12                  ; Number of characters to output.

    add     r12, [rax + FRAME_PTR]    ; rdi = ptr + pos

    call    emit_bytes

    dec     qword [stack_len]
    jmp     .loop

.depth_nonzero:
    ; if pos == len
    mov     rcx, [rax + FRAME_POS]
    cmp     rcx, [rax + FRAME_LEN]
    jb      .has_char                 ; There is a char in the frame.

    dec     qword [stack_len]           
    jmp     .loop

.has_char:
    mov     rdi, [rax + FRAME_PTR]    ; rdi = ptr
    movzx   ecx, byte [rdi + rcx]     ; ecx = ptr[pos]

    inc     qword [rax + FRAME_POS]   ; pos++

    cmp     byte [rule_seen + rcx], 0
    jne     .has_rule

    mov     bl, cl                    ; Character number.
    call    emit_char
    jmp     .loop

.has_rule:
    mov     rdx, [rax + FRAME_DEPTH]  ; Current depth.

    movzx   r11d, byte [erase_depth + rcx]  
    test    r11d, r11d
    je      .not_erasing_known

    cmp     rdx, r11                  ; Does the character have time to vanish?
    jae     .loop                          

.not_erasing_known:
    mov     rsi, [rule_len + 8*rcx]   ; rsi = length of RHS for this character.
    test    rsi, rsi
    je      .loop

    mov     rdi, [rules_ptr]
    add     rdi, [rule_ptr + 8*rcx]   ; RHS address for the character.

    dec     rdx                       ; rdx = depth - 1
    call    push_frame                ; push_frame(RHS, rhs_len, depth - 1)
    jmp     .loop

.finish:                              ; Stack is empty.
    mov     bl, 10                    ; Print newline.
    call    emit_char
    call    flush_out

    call    cleanup

    test    r14b, r14b                ; cleanup_failed flag
    jnz     error_no_cleanup

    mov     eax, SYS_EXIT
    xor     edi, edi
    syscall

error:
    call    cleanup

error_no_cleanup:
    mov     eax, SYS_EXIT
    mov     edi, 1
    syscall