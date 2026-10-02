global arithmetic_sequence

; Opis rejestrów:
;   rdi - wskaźnik na Ak (cel stosq; początkowo w rdx)
;   rsi - wskaźnik na A1 (źródło lodsq)
;   r9  - wskaźnik na A0
;   rcx - licznik słów n (dla loop)
;   r8  - mnożnik: k (k >= 0) lub |k|+1 (k < 0, po zamianie A0/A1)
;   r10 - pożyczka wielosłownego odejmowania (dolny bajt r10b)
;   r11 - przeniesienie między iteracjami pętli

arithmetic_sequence:
  mov  r9, rdi      ; Przenosimy A0, bo rdx będzie zmieniamy podczas mul.
  mov  rdi, rdx     ; Przenosimy Ak, żeby użyć efektywnegi zapisywania.
  xor  r10, r10     ; pożyczka = 0
  xor  r11, r11     ; przeniesienie = 0
  test r8, r8       ; Sprawdzamy, czy k jest nieujemne.
  jns  .loop
  xchg rsi, r9      ; Zamieniamy A0 z A1, gdy k jest ujemne.
  neg  r8
  inc  r8           ; Zamieniamy k na |k|+1, żeby wzór się zgadzał.
; Główna pętla, wykonuje się n razy.
; Liczymy kolejne 64-bitowe fragmenty delty_i 
; i sukcesywnie mnożymy je przez k.
.loop:
  lodsq             ; Wykonujemy mov rax, [rsi] oraz add rsi, 8.
  shr  r10, 1       ; Ustawiamy znacznik pożyczki. 
  sbb  rax, [r9]    ; Odejmujemy A0, czyli obliczamy delta_i.
  setc r10b         ; Przenosimy pożyczkę do dolnego bajtu.  
                    ; Przy ostatnim obrocie jest to znak delty.
  mul  r8           ; Mnożymy delta_i przez k.
  add  rax, r11     ; Dodajemy przeniesienie z poprzedniego obrotu.
  adc  rdx, 0       ; Dodajemy przeniesienie, jeśli wystąpiło.
  add  rax, [r9]    ; Dodajemy A_0 do delty.
  adc  rdx, 0       ; Dodajemy przeniesienie, jeśli wystąpiło.
  stosq             ; Wykonujemy mov [rdi], rax oraz add rdi, 8.
  mov  r11, rdx     ; Ustawiamy r11 na starszą część wyniku.
  add  r9, 8
  loop .loop
; Ustawiamy 64 przedostatnich bitów Ak.
; Liczymy różnicę bitów znaku.
  mov  rax, [rsi-8] ; Zapamiętujemy ostatnie 64 bity A1.
  sar  rax, 63      ; Wyłuskujemy bit znaku i go powielamy. 
  mov  rcx, [r9-8]  ; Zapamiętujemy ostatnie 64 bity A0.
  sar  rcx, 63      ; Wyłuskujemy bit znaku i go powielamy. 
  shr  r10, 1
  sbb  rax, rcx     ; Obliczamy delta_i.
  mul  r8           ; Mnożymy delta_i przez k.  
  add  rax, rcx     ; Dodajemy A0 do delta_i * k.
; Liczymy ostatnie 64 bity Ak i poprawiamy przedostatnie.  
  adc  rdx, rax
  add  rax, r11     ; Dodajemy przeniesienie.
  adc  rdx, 0       ; Dodajemy przeniesienie, jeśli wystąpiło.
  ret
