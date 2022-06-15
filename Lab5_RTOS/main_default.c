/*
 * Default main.c for rtos lab.
 * @author Andrew Morton, 2018
 */
#include <LPC17xx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_THREADS 3
#define NO_THREADS 0 //no non-idle threads are running, literally do nothing
#define ONE_THREAD 1 //only one non-idle thread is running
#define NEW_THREAD 2
#define NORMAL_THREADING 3

#define CREATED 0 //created, but not running
#define ACTIVE 1 //running and active
#define WAITING 2 //not running but ready to go
#define DESTROYED 3 //for use later, especially for threads that end. This indicates that a new thread COULD go here if it needs to

uint32_t* MSR_Original = 0; //0 being the pointer location

typedef struct thread_t{
	void (*threadFunction)(void* args);
	int status;
	uint32_t* taskStack; //stack pointer for this task
}thread;

thread osThreads[MAX_THREADS]; //need to initialize this properly!

int threadNums = 0; //number of threads
int osNumThreadsRunning = 0;
uint32_t msrAddr;

//task management
int osCurrentTask = 0;
int osPrevTask = 0;

#define MSR_STACK_SIZE 0x400
#define THREAD_STACK_SIZE 0x200

//returns the thread ID, or -1 if that is not possible
int osThreadNew(void (*tf)(void*args))
{
	if(threadNums < MAX_THREADS)
	{
		osThreads[threadNums].status = WAITING; //tells the OS that it is ready but not yet run
		osThreads[threadNums].threadFunction = tf;
		osThreads[threadNums].taskStack = (uint32_t*)((msrAddr - MSR_STACK_SIZE) - (threadNums)*THREAD_STACK_SIZE);
		
		//Now we need to set up the stack
		
		//First is xpsr, the status register. If bit 24 is not set and we are in thread mode we get a hard fault, so we just make sure it's set
		*(osThreads[threadNums].taskStack) = 1<<24;
		
		//Next is the program counter, which is set to whatever the function we are running will be
		*(--osThreads[threadNums].taskStack) = (uint32_t)tf;
		
		//Next is a set of important registers. These values are meaningless but we are setting them to be nonzero so that the 
		//compiler doesn't optimize out these lines
		*(--osThreads[threadNums].taskStack) = 0xE; //LR
		*(--osThreads[threadNums].taskStack) = 0xC; //R12
		*(--osThreads[threadNums].taskStack) = 0x3; //R3
		*(--osThreads[threadNums].taskStack) = 0x2; //R2
		*(--osThreads[threadNums].taskStack) = 0x1; //R1
		*(--osThreads[threadNums].taskStack) = 0x0; // R0
		
		
		//Now we have registers R11 to R4, which again are just set to random values so that we know for sure that they exist
		*(--osThreads[threadNums].taskStack) = 0xB; //R11
		*(--osThreads[threadNums].taskStack) = 0xA; //R10
		*(--osThreads[threadNums].taskStack) = 0x9; //R9
		*(--osThreads[threadNums].taskStack) = 0x8; //R8
		*(--osThreads[threadNums].taskStack) = 0x7; //R7
		*(--osThreads[threadNums].taskStack) = 0x6; //R6
		*(--osThreads[threadNums].taskStack) = 0x5; //R5
		*(--osThreads[threadNums].taskStack) = 0x4; //R4
		
		
		//Now the stack is set up, the thread's SP is correct, since we've been decrementing it.
		threadNums++;
		osNumThreadsRunning++;
		return threadNums - 1;
	}
	return -1;
}

uint32_t msTicks = 0;

int x;
int y;

/*
uint32_t* task1Stack;
uint32_t* task2Stack;
uint32_t* osTaskStack;
*/
uint32_t* taskArray[3];

int task1Running = 0;
int task2Running = 0;

/*
void SysTick_Handler(void) {
    msTicks++;
}*/

//at the moment this just changes the stack from one to the other
int task_switch(void){
	//if we are running only a single thread at the moment, which means osNumThreadsRunning = 2, we do NOT do a switch
		if(osNumThreadsRunning == 1) //only the idle task is running
			return NO_THREADS;
		
		__set_PSP((uint32_t)osThreads[osCurrentTask].taskStack); //set the new PSP
		return NORMAL_THREADING;
		
}




static void trigger_pendsv(void)
{
	if(osCurrentTask >= 0)
	{
		osThreads[osCurrentTask].status = WAITING;
	//	osThreads[osCurrentTask].taskStack = (uint32_t*)(__get_PSP());// - 8*4);
		osPrevTask = osCurrentTask;
	}
	osThreads[osCurrentTask].taskStack = (uint32_t*)(__get_PSP() - 17*4); //we are about to push a bunch of things
	osCurrentTask = (osCurrentTask+1)%(threadNums);
	osThreads[osCurrentTask].status = ACTIVE;
	//osThreads[osCurrentTask].taskStack = (uint32_t*)(__get_PSP() - 8*4);
	//__set_PSP((uint32_t)osThreads[osCurrentTask].taskStack);
	
	volatile uint32_t *icsr = (void*)0xE000ED04; //Interrupt Control/Status Vector
	*icsr = 0x1<<28; //tell the chip to do the pendsv by writing 1 to the PDNSVSET bit

	//flush things
		__ASM("isb"); //This just prevents other things in the pipeline from running before we return
	/*
	thread osNext;
	thread osCurr;
	if(osCurrentTask >= 0)
		osCurr = osThreads[osCurrentTask];
	else
		osCurr = osThreads[0];
	
	if(threadNums == 1) //only one task is running anyway, and it's the idle task
		osNext = osCurr;
	else
	{
		
		printf("Current task: %d\n",osCurrentTask);
		//we need to find the next task that is ready to run
		int threadIndex = (osCurrentTask+1)%threadNums;
		if(osCurrentTask<0)
			osCurrentTask = 0; //initialization
			//OK, so now it's time to set the current task to run
			osThreads[osCurrentTask].status = WAITING; //we set this in case there is only one task to run
		
			while(osThreads[threadIndex].status != WAITING)
		{
			threadIndex = (threadIndex+1)%threadNums;
			printf("Current task: %d\n",threadIndex);
		}

		osThreads[osCurrentTask].taskStack = (uint32_t*)(__get_PSP()  - 8*4); //just before we switch it out, tell it where its stack left off
		osThreads[threadIndex].status = ACTIVE; //so now it's a set and reset if there is only one task
		osNext = osThreads[threadIndex];
		osCurrentTask = threadIndex;
	}
	
	//trigger the pendSV iff the old and new threads are not the same
	if(osNext.taskStack != osCurr.taskStack) //if they have the same stack they are the same thread
	{
		volatile uint32_t *icsr = (void*)0xE000ED04; //Interrupt Control/Status Vector
		*icsr = 0x1<<28; //tell the chip to do the pendsv by writing 1 to the PDNSVSET bit

		//flush things
		__ASM("isb"); //This just prevents other things in the pipeline from running before we return
	}
	*/
	return;
}


void task1(void* args)
{
	while(1)
	{
		x++;
	  printf("In task 1. x is: %d\n",x);
		trigger_pendsv(); //this is actually going to be "yield" eventually
	}
}
void task2(void* args)
{
	while(1)
	{
		y++;
		printf("In task 2. y is: %d\n",y);
		trigger_pendsv(); //this is actually going to be "yield" eventually
	}
}

void osIdleTask(void* args)
{
	while(1)
	{
		printf("in task 0\n");
		//otherwise just dip
		trigger_pendsv();
	}
	
}

int main(void) {
	SystemInit();//set the LEDs to be outputs. You may or may not care about this
	LPC_GPIO1->FIODIR |= 1<<28; //LED on pin 28
	LPC_GPIO1->FIODIR |= 1<<29;
	LPC_GPIO1->FIODIR |= 1U<<31;
	LPC_GPIO2->FIODIR |= 1<<2;
	LPC_GPIO2->FIODIR |= 1<<3;
	LPC_GPIO2->FIODIR |= 1<<4;
	LPC_GPIO2->FIODIR |= 1<<5;
	LPC_GPIO2->FIODIR |= 1<<6;
	
	/* set the PendSV interrupt priority to the lowest level 0xFF,
		 which makes it very negative*/
	*(uint32_t volatile *)0xE000ED20 |= (0xFFU << 16);
	
	msrAddr = *MSR_Original;
	printf("MSR Originally at: %d\n",msrAddr);
	x = 0;
	y = 0;
	
	//set up my threads
	osThreadNew(osIdleTask);
	osThreadNew(task1);
	osThreadNew(task2);
	printf("Status: Threadnums: %d, osRunningThreads: %d\n",threadNums,osNumThreadsRunning);
		osCurrentTask = -1;
	//This is the only thread that starts itself
	__set_CONTROL(1<<1);
	__set_PSP((uint32_t)osThreads[0].taskStack);
	
	//activate yourself
	//osThreads[osCurrentTask].status = ACTIVE;
	//osIdleTask(&x);
	trigger_pendsv();
}
