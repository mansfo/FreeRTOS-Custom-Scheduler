#ifndef INT_QUEUE_TIMER_H
#define INT_QUEUE_TIMER_H

void vInitialiseTimers( void );
void TIMER0_HANDLER( void );
void TIMER1_HANDLER( void );
void vSetDeadlineTimer( uint32_t ticks);
void vStopDeadlineTimer( void );
#endif

