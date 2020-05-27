
    .text
	.global bigint_adc
        .type   bigint_adc, %function @ Hint for linker/debugger
bigint_adc:
    stmfd   sp!,{r4-r10,lr}

    ldr     r3, [r0,#4]     
    ldr     r4, [r1,#4]     

    cmp     r3, r4
    ble     getExtends      @@ don't swap if in order

    mov     ip, r3          @@ swap l->size
    mov     r3, r4          @@  and r->size
    mov     r4, ip
    
    mov     ip, r0          @@ swap l and r
    mov     r0, r1
    mov     r1, ip

    @@ smallest bigint is now in r0, largest in r1
    @@ smallest size is in r3, largest in r4

getExtends:

    sub     r5, r3, #1


    ldr     r0, [r0]
    ldr	    r1, [r1]
    ldr     ip, [r0,r5,lsl #2]

    cmp     ip, #0
    
    movlt   r5, #-1         @@ extend with 1's if neg
    movge   r5, #0          @@ 0's if pos

    @@ r5 is now the extend bits of the smaller bigint
    @@ r0 and r1 are now pointers to blks not bigints

alloc:
    
    stmfd   sp!, {r0-r3}
    
    add     r0, r4, #1

    bl      bigint_alloc

    mov     r7, r0

    ldmfd   sp!, {r0-r3}

    @@ r7 is now a pointer to sum


    @@ ONLY WORKS IF THIS SETS THE CARRY
    cmp     r2, #1  
    mrs     ip, cpsr  

    add     r2, r0, r3, lsl #2
    
    ldr     r8, [r7]        @@ r8 is pointer to sum blks

addition:

    ldr     r9, [r0], #4
    ldr     r10,[r1], #4

    msr     cpsr, ip
    adcs    ip, r9, r10
    

    str     ip, [r8], #4

    mrs     ip, cpsr
    cmp     r0, r2
    bne     addition


    cmp     r3, r4
    beq     extend

    sub     r2, r4, r3
    add     r2, r1, r2, lsl #2

    @@ r2 is now number of bytes to loop in the next loop

addToX:

    ldr     r10,[r1], #4


    msr     cpsr, ip
    adcs    ip, r10, r5

    str     ip, [r8], #4

    mrs     ip, cpsr
    cmp     r1, r2
    bne     addToX

extend:    
    
    msr     cpsr, ip
    
    bvc     trim
    
    movcs   ip, #-1
    movcc   ip, #0
    
    str     ip, [r8]
    add	    r4, r4, #1    
    b	    return

trim:

    ldr	    r5, [r8,#-4]!
	
    cmp	    r5, #0	    
    beq	    trimLoop

    cmp	    r5, #-1
    beq     trimLoop

    b       return

trimLoop:

    cmp	    r4, #1
    beq	    return

    ldr	    r2, [r8,#-4]!
    sub	    r4, r4, #1
	 
    cmp	    r2, r5
    beq	    trimLoop	

calcX:
            
    teq     r2, r5
    addmi   r4, r4, #1  @@ leave extra extend chunk
                        @@ if sign bits are different

		
return:
    
    str     r4, [r7,#4]
    mov     r0, r7
    
    ldmfd   sp!,{r4-r10,pc}

    .size   bigint_adc, .-bigint_adc @ Hint for linker/debugger
