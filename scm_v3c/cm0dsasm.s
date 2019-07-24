Stack_Size      EQU     0x0800							; 4KB of STACK

                AREA    STACK, NOINIT, READWRITE, ALIGN=4
Stack_Mem       SPACE   Stack_Size
__initial_sp	


Heap_Size       EQU     0x0400							; 2KB of HEAP

                AREA    HEAP, NOINIT, READWRITE, ALIGN=4
__heap_base				
Heap_Mem        SPACE   Heap_Size
__heap_limit


; Vector Table Mapped to Address 0 at Reset

					PRESERVE8
					THUMB

        				AREA	RESET, DATA, READONLY
        				EXPORT 	__Vectors
					
__Vectors		    	DCD		__initial_sp
        				DCD		Reset_Handler
        				DCD		0
        				DCD		0
        				DCD		0
        				DCD		0
        				DCD		0
        				DCD		0
        				DCD		0
        				DCD		0
        				DCD		0
        				DCD 	0
        				DCD		0
        				DCD		0
        				DCD 	0
        				DCD		0
        				
        				; External Interrupts
						        				
        				DCD		UART_Handler
        				DCD		interrupt_gpio3_activehigh_debounced
        				DCD		optical_irq_in
        				DCD		ADC_Handler
        				DCD		0
        				DCD		0
        				DCD		RF_Handler
        				DCD		RFTIMER_Handler
        				DCD		interrupt_rawchips_startval
        				DCD		interrupt_rawchips_32
        				DCD		0
        				DCD		optical_sfd_interrupt
        				DCD		interrupt_gpio8_activehigh
        				DCD		interrupt_gpio9_activelow
        				DCD		interrupt_gpio10_activelow
        				DCD		0 
              
                AREA |.text|, CODE, READONLY
;Interrupt Handlers
Reset_Handler   PROC
		GLOBAL Reset_Handler
		ENTRY
		
		LDR     R1, =0xE000E100           ;Interrupt Set Enable Register
		LDR     R0, =0x0009				;<- REMEMBER TO ENABLE THE INTERRUPTS!!
		STR     R0, [R1]
		
		;IP wake up just to solve interrupts
		; LDR r0, =0xE000ED10; System Control Register address
		; LDR r1, [r0]
		; MOVS r2, #0x6
		; ORRS r1, r2; Set SLEEPONEXIT bit
		; STR r1, [r0]
		
		IMPORT  __main
		LDR     R0, =__main               
		BX      R0                        ;Branch to __main
				ENDP
	
	
UART_Handler    PROC
		EXPORT UART_Handler
		IMPORT UART_ISR
		
		PUSH    {R0,LR}
		
		MOVS R0, #1 ; 		;MASK all interrupts
		MSR PRIMASK, R0 ; 		
		
		BL UART_ISR

				
		MOVS	R0, #0		;ENABLE all interrupts
		MSR	PRIMASK, R0
		
		POP     {R0,PC}
				ENDP             	
		
ADC_Handler	PROC	
		EXPORT 	ADC_Handler
		IMPORT	ADC_ISR
		
		PUSH	{R0,LR}
		
		MOVS 	R0, #1 ; 		;MASK all interrupts
		MSR 	PRIMASK, R0 ; 
		;STR		R0,[R1]	
		
		BL 	ADC_ISR
		
		MOVS	R0, #0		;ENABLE all interrupts
		MSR	PRIMASK, R0
		
		POP	{R0,PC}
		
			ENDP
				
RF_Handler	PROC	
		EXPORT 	RF_Handler
		IMPORT	RF_ISR
		
		PUSH	{R0,LR}
		
		MOVS 	R0, #1 ; 		;MASK all interrupts
		MSR 	PRIMASK, R0 ; 
		;STR		R0,[R1]	
		
		BL 	RF_ISR
		
		MOVS	R0, #0		;ENABLE all interrupts
		MSR	PRIMASK, R0
		
		POP	{R0,PC}
		
			ENDP
		
RFTIMER_Handler	PROC	
		EXPORT 	RFTIMER_Handler
		IMPORT	RFTIMER_ISR
		
		PUSH	{R0,LR}
		
		MOVS 	R0, #1 ; 		;MASK all interrupts
		MSR 	PRIMASK, R0 ; 
		;STR		R0,[R1]	
		
		BL 	RFTIMER_ISR
		
		MOVS	R0, #0		;ENABLE all interrupts
		MSR	PRIMASK, R0
		
		POP	{R0,PC}
		
			ENDP

; -----------------------------
; BEGIN EXTERNAL INTERRUPT ISRS
; -----------------------------
interrupt_gpio3_activehigh_debounced	PROC	
		EXPORT 	interrupt_gpio3_activehigh_debounced
		IMPORT	INTERRUPT_GPIO3_ISR
		
		PUSH	{R0,LR}
		
		MOVS 	R0, #1 ; 		;MASK all interrupts
		MSR 	PRIMASK, R0 ; 
		;STR		R0,[R1]	
		
		BL 	INTERRUPT_GPIO3_ISR
		
		MOVS	R0, #0		;ENABLE all interrupts
		MSR	PRIMASK, R0
		
		POP	{R0,PC}
		
			ENDP

interrupt_gpio8_activehigh	PROC	
		EXPORT 	interrupt_gpio8_activehigh
		IMPORT	INTERRUPT_GPIO8_ISR
		
		PUSH	{R0,LR}
		
		MOVS 	R0, #1 ; 		;MASK all interrupts
		MSR 	PRIMASK, R0 ; 
		;STR		R0,[R1]	
		
		BL 	INTERRUPT_GPIO8_ISR
		
		MOVS	R0, #0		;ENABLE all interrupts
		MSR	PRIMASK, R0
		
		POP	{R0,PC}
		
			ENDP

interrupt_gpio9_activelow	PROC	
		EXPORT 	interrupt_gpio9_activelow
		IMPORT	INTERRUPT_GPIO9_ISR
		
		PUSH	{R0,LR}
		
		MOVS 	R0, #1 ; 		;MASK all interrupts
		MSR 	PRIMASK, R0 ; 
		;STR		R0,[R1]	
		
		BL 	INTERRUPT_GPIO9_ISR
		
		MOVS	R0, #0		;ENABLE all interrupts
		MSR	PRIMASK, R0
		
		POP	{R0,PC}
		
			ENDP

interrupt_gpio10_activelow	PROC	
		EXPORT 	interrupt_gpio10_activelow
		IMPORT	INTERRUPT_GPIO10_ISR
		
		PUSH	{R0,LR}
		
		MOVS 	R0, #1 ; 		;MASK all interrupts
		MSR 	PRIMASK, R0 ; 
		;STR		R0,[R1]	
		
		BL 	INTERRUPT_GPIO10_ISR
		
		MOVS	R0, #0		;ENABLE all interrupts
		MSR	PRIMASK, R0
		
		POP	{R0,PC}
		
			ENDP
; ---------------------------
; END EXTERNAL INTERRUPT ISRS
; ---------------------------


interrupt_rawchips_startval	PROC	
		EXPORT 	interrupt_rawchips_startval
		IMPORT	RAWCHIPS_STARTVAL_ISR
		
		PUSH	{R0,LR}
		
		MOVS 	R0, #1 ; 		;MASK all interrupts
		MSR 	PRIMASK, R0 ; 
		;STR		R0,[R1]	
		
		BL 	RAWCHIPS_STARTVAL_ISR
		
		MOVS	R0, #0		;ENABLE all interrupts
		MSR	PRIMASK, R0
		
		POP	{R0,PC}
		
			ENDP
				
interrupt_rawchips_32	PROC	
		EXPORT 	interrupt_rawchips_32
		IMPORT	RAWCHIPS_32_ISR
		
		PUSH	{R0,LR}
		
		MOVS 	R0, #1 ; 		;MASK all interrupts
		MSR 	PRIMASK, R0 ; 
		;STR		R0,[R1]	
		
		BL 	RAWCHIPS_32_ISR
		
		MOVS	R0, #0		;ENABLE all interrupts
		MSR	PRIMASK, R0
		
		POP	{R0,PC}
		
			ENDP


optical_irq_in	PROC	
		EXPORT 	optical_irq_in
		IMPORT	OPTICAL_32_ISR
		
		PUSH	{R0,LR}
		
		MOVS 	R0, #1 ; 		;MASK all interrupts
		MSR 	PRIMASK, R0 ; 
		;STR		R0,[R1]	
		
		BL 	OPTICAL_32_ISR
		
		MOVS	R0, #0		;ENABLE all interrupts
		MSR	PRIMASK, R0
		
		POP	{R0,PC}
		
			ENDP
				

optical_sfd_interrupt	PROC	
		EXPORT 	optical_sfd_interrupt
		IMPORT	OPTICAL_SFD_ISR
		
		PUSH	{R0,LR}
		
		MOVS 	R0, #1 ; 		;MASK all interrupts
		MSR 	PRIMASK, R0 ; 
		;STR		R0,[R1]	
		
		BL 	OPTICAL_SFD_ISR
		
		MOVS	R0, #0		;ENABLE all interrupts
		MSR	PRIMASK, R0
		
		POP	{R0,PC}
		
			ENDP

		ALIGN 4
			 
; User Initial Stack & Heap
                IF      :DEF:__MICROLIB
                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit
                ELSE
                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap
__user_initial_stackheap

                LDR     R0, =  Heap_Mem
                LDR     R1, =(Stack_Mem + Stack_Size)
                LDR     R2, = (Heap_Mem +  Heap_Size)
                LDR     R3, = Stack_Mem
                BX      LR

                ALIGN

                ENDIF


		END                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     
   