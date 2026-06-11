#include "FreeRTOS.h"
#include "IntTimer.h"
#include "SMM_MPS2.h"
#include "semphr.h"


#define tmrTIMER_0_FREQUENCY	( 3000UL )
#define tmrTIMER_1_FREQUENCY	( 2001UL )



volatile uint32_t ulNest;
extern SemaphoreHandle_t xBinarySemaphore;

void TIMER0_HANDLER( void )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	CMSDK_TIMER0->INTCLEAR = ( 1ul <<  0 );
    xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
/*-----------------------------------------------------------*/

void TIMER1_HANDLER( void )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	CMSDK_TIMER1->INTCLEAR = ( 1ul <<  0 );
	CMSDK_TIMER1->CTRL = 0;
	xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
/*-----------------------------------------------------------*/

void vSetDeadlineTimer( uint32_t ticks)
{
	if (ticks == 0) ticks=1; 
	CMSDK_TIMER1->CTRL=0;
	CMSDK_TIMER1->VALUE= ticks;
	CMSDK_TIMER1->INTCLEAR =1;
	CMSDK_TIMER1->CTRL= ( ( 1ul <<  3 ) |( 1ul <<  0 ) );
}

void vStopDeadlineTimer( void )
{
	CMSDK_TIMER1->CTRL=0;
}

/*-----------------------------------------------------------*/

void vInitialiseTimers( void )
{
	CMSDK_TIMER0->INTCLEAR = ( 1ul <<  0 );
	CMSDK_TIMER0->RELOAD   = ( configCPU_CLOCK_HZ / tmrTIMER_0_FREQUENCY ) + 1UL;
	CMSDK_TIMER0->CTRL     = ( ( 1ul <<  3 ) |
						     ( 1ul <<  0 ) );
							 
	CMSDK_TIMER1->INTCLEAR = ( 1ul <<  0 );
	CMSDK_TIMER1->RELOAD   = ( configCPU_CLOCK_HZ / tmrTIMER_1_FREQUENCY ) + 1UL;
	CMSDK_TIMER1->CTRL     = 0;
	NVIC_SetPriority( TIMER1_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1 );
	NVIC_EnableIRQ( TIMER1_IRQn ); 
	NVIC_SetPriority( TIMER0_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY );
	NVIC_EnableIRQ( TIMER0_IRQn );
	
}

