PORTB = $6000
PORTA = $6001
DDRB = $6002
DDRA = $6003

E = %10000000
RW = %01000000
RS = %00100000


.org $8000
reset:
	lda #$FF
	txs

	lda #$FF				; Set all pins on port B to Output
	sta DDRB  
	lda #$BF
	sta DDRA

	jsr lcd_init
	lda #$28				; Set 4-bit mode; 2-line display; 5x8 font
	jsr lcd_instruction
	lda #%00001110    ; Display on; cursor on; blink off
	jsr lcd_instruction
	lda #%00000110    ; Increment and shift cursor; don't shift display
 jsr lcd_instruction
 lda #%00000001    ; clear display
 jsr lcd_instruction


rx_wait:
	bit PORT A							; put PORT A into V flag (over flow flag)
 bvs rx_wait						; Loop if no start bit yet

 lda #"x"
	jsr print_char
	jmp rx_wait
lcd_wait:
	pha
 lda #%11110000   ; LCD data is input
	sta DDRB


; =============================================================================
; SUBROUTINES
; =============================================================================

; --- lcd_instruction ($8100) ---
  .org $8100
lcd_instruction:
  jsr wait_1ms      ; Busy wait substitute
  sta PORTB         ; Put instruction on bus
  
  lda #0            ; Clear Control Bits
  sta PORTA
  lda #E            ; Toggle E High
  sta PORTA
  lda #0            ; Toggle E Low
  sta PORTA
  rts

; --- print_char ($8120) ---
  .org $8120
print_char:
  jsr wait_1ms      ; Busy wait substitute
  sta PORTB         ; Put char on bus
  
  lda #RS           ; Set RS (Data mode)
  sta PORTA
  lda #(RS | E)     ; Toggle E High
  sta PORTA
  lda #RS           ; Toggle E Low
  sta PORTA
  rts

loop:
  jmp loop

; =============================================================================
; VECTORS
; =============================================================================
  .org $FFFC
  .word reset
  .word reset
  .word $0000   