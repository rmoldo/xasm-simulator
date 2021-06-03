start:
	jmp main

func:
	xor $r2, $r2
	cmp $r2, 0
	beq zero
	bne nzero

zero:
	mov $r2, 1234
	mov $r3, 1000
	inc $r2
	dec $r2
	sub $r2, $r3
	br fin
		
nzero:
	mov $r2, 0
	br fin
fin:
	ret

main:
	call func
	
	mov $r10, 32767
	push $r10

	mov 0($r10), 1234
	
	dec ($r10)
	add $r10, 10
	
	pop $r10
	
	call func2	
	
	mov $r9, 1
	neg $r9
	add $r9, 1	
	
	halt

func2:
	mov $r10, 102	
	sec

	ror $r10

	ret
