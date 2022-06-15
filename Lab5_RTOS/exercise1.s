	AREA	handle_pend,CODE,READONLY ;If you don't have this, the linker won't have a clue what to do
	GLOBAL PendSV_Handler ;PendSV_Handler is global so that it can be linked elsewhhere
	PRESERVE8 ;You must indicate that this function preserves 8-byte stack alignment. It does, I promise, but the linker doesn't know that

PendSV_Handler ;It has to be called this, and it must start in the first column
		
		;Load the constant 0xFFFFFFFD into LR here - yes, it is probably already loaded. Do it again to prove you can.
		
		;return? Maybe, if you loaded the right constant
		BX LR


		END ;You need this to end the file or the assembler won't know what to do