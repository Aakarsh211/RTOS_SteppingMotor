#include "network.h"
#include "stepper.h"
#include "gpio.h"

#define BUTTONS_DEVICE_ID 	XPAR_AXI_GPIO_INPUTS_DEVICE_ID
#define GREEN_LED_DEVICE_ID XPAR_GPIO_1_DEVICE_ID
#define GREEN_LED_CHANNEL 1
#define MOTOR_DEVICE_ID   	XPAR_GPIO_2_DEVICE_ID

#define RGB_LED_ID XPAR_AXI_GPIO_LEDS_DEVICE_ID
#define RGB_CHANNEL 2
// TODO: add macro for polling period
#define RGB_OFF     0b000
#define RGB_RED     0b100

static void stepper_control_task( void *pvParameters );
static void emergency_task( void *pvParameters );
int Initialize_UART();

XGpio red;

motor_parameters_t motor_parameters;
TaskHandle_t motorTaskHandle = NULL;

QueueHandle_t button_queue    = NULL;
QueueHandle_t motor_queue     = NULL;
QueueHandle_t emergency_queue = NULL;
QueueHandle_t led_queue       = NULL;


int main()
{
    int status;

    // Initialization of motor parameter values
	motor_parameters.current_position = 0;
	motor_parameters.final_position   = 0;
	motor_parameters.dwell_time       = 0;
	motor_parameters.rotational_speed = 0.0;
	motor_parameters.rotational_accel = 0.0;
	motor_parameters.rotational_decel = 0.0;

    button_queue    = xQueueCreate(1, sizeof(u32));
    led_queue       = xQueueCreate(1, sizeof(u8));
    motor_queue     = xQueueCreate( 25, sizeof(motor_parameters_t) );
    emergency_queue = xQueueCreate(1, sizeof(u8));

    configASSERT(led_queue);
    configASSERT(emergency_queue);
    configASSERT(button_queue);
	configASSERT(motor_queue);

	// Initialize the PMOD for motor signals (JC PMOD is being used)
	status = XGpio_Initialize(&pmod_motor_inst, MOTOR_DEVICE_ID);

	if(status != XST_SUCCESS){
		xil_printf("GPIO Initialization for Stepper Motor unsuccessful.\r\n");
		return XST_FAILURE;
	}

	//Initialize the UART
	status = Initialize_UART();

	if (status != XST_SUCCESS){
		xil_printf("UART Initialization failed\n");
		return XST_FAILURE;
	}

	// Initialize GPIO buttons
    status = XGpio_Initialize(&buttons, BUTTONS_DEVICE_ID);

    if (status != XST_SUCCESS) {
        xil_printf("GPIO Initialization Failed\r\n");
        return XST_FAILURE;
    }

    XGpio_SetDataDirection(&buttons, BUTTONS_CHANNEL, 0xFF);

    // TODO: Initialize green LEDS

   status = XGpio_Initialize(&green_leds, GREEN_LED_DEVICE_ID);

   if (status != XST_SUCCESS){
	   xil_printf("GPIO Initialization (green LEDs) Failed!/r/n");
	   return XST_FAILURE;
   }

   XGpio_SetDataDirection(&green_leds, GREEN_LED_CHANNEL, 0x00);

   status = XGpio_Initialize(&red, RGB_LED_ID);

   if (status != XST_SUCCESS){
	   xil_printf("GPIO Initialization (green LEDs) Failed!/r/n");
	   return XST_FAILURE;
   }

   XGpio_SetDataDirection(&red, 2, 0x00);

   ///////////////////

	xTaskCreate( stepper_control_task
			   , "Motor Task"
			   , configMINIMAL_STACK_SIZE*10
			   , NULL
			   , DEFAULT_THREAD_PRIO + 1
			   , &motorTaskHandle
			   );

    xTaskCreate( pushbutton_task
    		   , "PushButtonTask"
			   , THREAD_STACKSIZE
			   , NULL
			   , DEFAULT_THREAD_PRIO
			   , NULL
			   );

    xTaskCreate( emergency_task
			   , "EmergencyTask"
			   , THREAD_STACKSIZE
			   , NULL
			   , DEFAULT_THREAD_PRIO
			   , NULL
			   );

    xTaskCreate( led_task
			   , "LEDTask"
			   , THREAD_STACKSIZE
			   , NULL
			   , DEFAULT_THREAD_PRIO
			   , NULL
			   );

    sys_thread_new( "main_thrd"
				  , (void(*)(void*))main_thread
				  , 0
				  , THREAD_STACKSIZE
				  , DEFAULT_THREAD_PRIO + 1
				  );

    vTaskStartScheduler();
    while(1);
    return 0;
}

static void stepper_control_task( void *pvParameters )
{
	u32 loops=0;
	const u8 stop_animation = 0;
	long motor_position = 0;

	stepper_pmod_pins_to_output();
	stepper_initialize();

	while(1){
		// get the motor parameters from the queue. The structure "motor_parameters" stores the received data.
		while(xQueueReceive(motor_queue, &motor_parameters, 0)!= pdPASS){
			vTaskDelay(100); // polling period
		}
		xil_printf("\nreceived a package on motor queue. motor parameters:\n");
		stepper_set_speed(motor_parameters.rotational_speed);
		stepper_set_accel(motor_parameters.rotational_accel);
		stepper_set_decel(motor_parameters.rotational_decel);
		stepper_set_pos(motor_parameters.current_position);
		stepper_set_step_mode(motor_parameters.step_mode);
		xil_printf("\npars:\n");
		xQueueSend(led_queue, &motor_parameters.step_mode, 0);
		motor_position = stepper_get_pos();
		stepper_move_abs(motor_parameters.final_position);
		xQueueSend(led_queue, &stop_animation, 0);
		motor_position = stepper_get_pos();
		xil_printf("finished on position: %lli", motor_position);
		vTaskDelay(motor_parameters.dwell_time);
		loops++;
		xil_printf("\n\nloops: %d\n", loops);
	}
}


static void emergency_task(void *pvParameters)
{
    u8 emergency = 0;\
    u8 z = 1;

    while (1) {
        // Wait for emergency signal (non-blocking loop with small delay)
        vTaskDelay(100);
        xQueueReceive(emergency_queue, &emergency, 0);

//        xil_printf("EMERGE: ");
//        for (int i = 7; 0 <= i; i--) xil_printf("%d", (emergency >> i) & 1);
//        xil_printf("\r\n");

        if (emergency) {
            if (emergency & 0x2){
                emergency = 0;
                XGpio_DiscreteWrite(&red, 2, 0x0);
//                stepper_initialize();
                vTaskResume(motorTaskHandle);
                continue;
            }
            if (emergency & 0x20) {
                XGpio_DiscreteWrite(&red, 2, z << 2);
                z = !z;
            }
            if (emergency & 0x10) {
                if (3.0 < abs(stepper_get_speed())) continue;
                emergency &= 0x6F;
                vTaskSuspend(motorTaskHandle);
                stepper_initialize();
                stepper_disable_motor();
            }
            if (emergency & 0x1) {
                emergency = 0x30;
                u8 var = 3;
                xQueueSend(led_queue, &var, 0);
                stepper_setup_stop();
                continue;
            }
        }
    }
}