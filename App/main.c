#include "api_task.h"
#include "api_test.h"

int main(int argc, char **argv){

	(void) argc;
	(void) argv;

	UART_init();
	vInitRTOSPrimitives();
	
	#if USE_TESTS
		xTaskCreate(
			vTestRunner,
			"TestRunner",
			4096,
			NULL,
			configMAX_PRIORITIES - 1,
			NULL
		);
	#else
    	extern SchedulerConfig_t cfg;
		vInit(&cfg);
    	vStart();
	#endif


    vTaskStartScheduler();
	
    for( ; ; );
}