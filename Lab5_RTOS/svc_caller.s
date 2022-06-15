	AREA	handle_pend,CODE,READONLY
	EXTERN task_switch
	GLOBAL PendSV_Handler
	GLOBAL push_LR
	PRESERVE8
PendSV_Handler
	
		MRS r0,PSP
		;LDR LR,[r0] ;reload LR
		;now store the registers
		;STMDB r0!,{r4-r11}
		STMDB r0!,{r4-r11}
		;STR LR,[r0]
		;call kernel task switch
		BL task_switch
		
		;OK, for setup purposes, r0 contains the number of threads. If this is 0 or 1, we need to do special things
		CBZ r0, noThreadsRunning
		
		MRS r0,PSP ;this is the new task stack
		MOV LR,#0xFFFFFFFD
		;LDR LR,[r0,#24]
		;get all of the registers out
		LDMIA r0!,{r4-r11}
		MSR PSP,r0
		;return?
		BX LR
noThreadsRunning
		MRS r0,PSP ;figure out where the stack is
		;LDR LR,[r0,#24] ;reload LR
		MOV LR,#0xFFFFFFFD
		;LDR PC,[r0] ;set PC to rock back to thread mode
		BX LR ;go back

push_LR
		PUSH {LR} 
		BX LR ;a simple function because inline assembly can't do this

		END