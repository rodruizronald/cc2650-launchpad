//
//	C Standard Libraries.
//
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
//
//	XDCtools Header files.
//
#include <xdc/std.h>
#include <xdc/runtime/System.h>
//
//	BIOS Header files.
//
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Swi.h>
//
//	TI-RTOS Header files.
//
#include <ti/drivers/PIN.h>
#include <ti/drivers/Power.h>

//
//	Bitwise operations.
//
#define _BV(bit)				(1 << (bit))

//
//	Defines for the seven segment display.
//
#define SEGMENT_A								PIN_ID(28)
#define SEGMENT_B								PIN_ID(30)
#define SEGMENT_C								PIN_ID(24)
#define SEGMENT_D								PIN_ID(22)
#define SEGMENT_E								PIN_ID(23)
#define SEGMENT_F								PIN_ID(29)
#define SEGMENT_G								PIN_ID(21)

#define DIGIT_UNITS							PIN_ID(27)
#define DIGIT_TENS							PIN_ID(26)

//
//	Defines for the DHT11 sensor.
//
#define DHT11	            			PIN_ID(25)
#define DHT11_OK								0
#define DHT11_ERROR_TIMEOUT			1
#define DHT11_ERROR_CHECKSUM		2
#define DHT11_NUM_BYTES					5
#define DHT11_THRESHOLD					45

//
//	General logic states.
//
#define HIGH										1
#define LOW											0

//
//	Default task stack size.
//
#define STACK_SIZE							512

//
//	Task structure and stack.
//
Task_Struct DHT11_taskStruct;
Char DHT11_taskStack[STACK_SIZE];

//
//	Clock structure.
//
Clock_Struct Display_ClkStruct;

//
//	PIN driver handle.
//
PIN_Handle DHT11_handle;
PIN_State  DHT11_state;

PIN_Handle Display_segmentHandle;
PIN_State  Display_segmentState;

PIN_Handle Display_digitHandle;
PIN_State  Display_digitState;

//
//	PIN module initial configuration table - I/O.
//
PIN_Config gpioInitTable[] =
{
	SEGMENT_A 	| PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW  | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_B 	| PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW  | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_C 	| PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW  | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_D 	| PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW  | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_E 	| PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW  | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_F 	| PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW  | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_G 	| PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW  | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	DIGIT_UNITS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW  | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	DIGIT_TENS  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW  | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	DHT11 			| PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	PIN_TERMINATE
};

//
//	Seven segment display pin configuration tables.
//
PIN_Config Display_semgmentTable[] =
{
	SEGMENT_A | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_B | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_C | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_D | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_E | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_F | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_G | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	PIN_TERMINATE
};

PIN_Config Display_digitTable[] =
{
	DIGIT_UNITS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW  | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	DIGIT_TENS  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	PIN_TERMINATE
};

//
//	DHT11 pin configuration tables.
//
PIN_Config DHT11_outputTable[] =
{
	DHT11 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	PIN_TERMINATE
};

PIN_Config DHT11_inputTable[] =
{
	DHT11 | PIN_INPUT_EN | PIN_NOPULL,
	PIN_TERMINATE
};

//
//	Array of seven segment display numbers.
//
uint32_t displayNumber[] =
{
	(_BV(SEGMENT_A) | _BV(SEGMENT_B) | _BV(SEGMENT_C) | _BV(SEGMENT_D) | _BV(SEGMENT_E) | _BV(SEGMENT_F)),
	(_BV(SEGMENT_B) | _BV(SEGMENT_C)),
	(_BV(SEGMENT_A) | _BV(SEGMENT_B) | _BV(SEGMENT_D) | _BV(SEGMENT_E) | _BV(SEGMENT_G)),
	(_BV(SEGMENT_A) | _BV(SEGMENT_B) | _BV(SEGMENT_C) | _BV(SEGMENT_D) | _BV(SEGMENT_G)),
	(_BV(SEGMENT_B) | _BV(SEGMENT_C) | _BV(SEGMENT_F) | _BV(SEGMENT_G)),
	(_BV(SEGMENT_A) | _BV(SEGMENT_C) | _BV(SEGMENT_D) | _BV(SEGMENT_F) | _BV(SEGMENT_G)),
	(_BV(SEGMENT_A) | _BV(SEGMENT_C) | _BV(SEGMENT_D) | _BV(SEGMENT_E) | _BV(SEGMENT_F) | _BV(SEGMENT_G)),
	(_BV(SEGMENT_A) | _BV(SEGMENT_B) | _BV(SEGMENT_C)),
	(_BV(SEGMENT_A) | _BV(SEGMENT_B) | _BV(SEGMENT_C) | _BV(SEGMENT_D) | _BV(SEGMENT_E) | _BV(SEGMENT_F) | _BV(SEGMENT_G)),
	(_BV(SEGMENT_A) | _BV(SEGMENT_B) | _BV(SEGMENT_C) | _BV(SEGMENT_D) | _BV(SEGMENT_F) | _BV(SEGMENT_G)),
};

uint8_t temperature = 0, humidity = 0;

//
//	This clock function runs every 10 ms with the main
//	purpose of refreshing the seven segment led display.
//
void Display_Clock(UArg arg0)
{
	//
	//	Get the tens and units digits of the temperature value.
	//
	uint8_t tens = (uint8_t)(temperature / 10);
	uint8_t units = temperature - (tens * 10);

	//
	//	Toggle current display digit on.
	//
	PIN_setOutputValue(Display_digitHandle, DIGIT_TENS,  !PIN_getOutputValue(DIGIT_TENS));
	PIN_setOutputValue(Display_digitHandle, DIGIT_UNITS, !PIN_getOutputValue(DIGIT_UNITS));

	//
	//	Refresh display value.
	//
	if (PIN_getOutputValue(DIGIT_UNITS))
	{
		PIN_setPortOutputValue(Display_segmentHandle, displayNumber[units]);
	}
	else
	{
		PIN_setPortOutputValue(Display_segmentHandle, displayNumber[tens]);
	}
}

uint8_t skipPulse(uint8_t state)
{
	uint16_t loopCnt = 10000;

	//
	//	Loop until the passed state has been reached.
	//
	while (PIN_getInputValue(DHT11) == state)
		if (loopCnt-- == 0) return DHT11_ERROR_TIMEOUT;

	return DHT11_OK;
}

uint8_t readSensor(void)
{
	//
	//	Allocate collection of pins based on DHT11_outputTable.
	//
	DHT11_handle = PIN_open(&DHT11_state, DHT11_outputTable);
	if (!DHT11_handle) System_abort("Error allocating pins - DHT11_outputTable\n");

	//
	//	Data buffer.
	//
 	uint8_t bytes[DHT11_NUM_BYTES];
	uint8_t i = 0, j = 0;

	//
	//	Zero out the data buffer.
	//
	for (i = 0; i < DHT11_NUM_BYTES; i++) bytes[i] = 0;

	//
	//	Request sample.
	//
	PIN_setOutputValue(DHT11_handle, DHT11, LOW);
	//
	//	Sleep for 18 ms.
	//
	Task_sleep(18000 / Clock_tickPeriod);

	//
	//	Deallocate pins to return to the initial configuration.
	//
	PIN_close(DHT11_handle);

	//
	//	Allocate collection of pins based on DHT11_inputTable.
	//
	DHT11_handle = PIN_open(&DHT11_state, DHT11_inputTable);
	if (!DHT11_handle) System_abort("Error allocating pins - DHT11_inputTable\n");

	//
	//	Skip the following pulses.
	//
	if (skipPulse(HIGH)) return DHT11_ERROR_TIMEOUT;
	if (skipPulse(LOW))  return DHT11_ERROR_TIMEOUT;
	if (skipPulse(HIGH)) return DHT11_ERROR_TIMEOUT;

	//
	//	Disable Swi to prevent Display_Clock preemption.
	//
	UInt key = Swi_disable();

	//
	//	Read output - 40 bits => 5 bytes or timeout.
	//
	UInt32 lastTick = 0, width = 0;
	for (i = 0; i < DHT11_NUM_BYTES; i++)
	{
		for (j = 0; j < 8; j++)
		{
			if (skipPulse(LOW))  return DHT11_ERROR_TIMEOUT;

			lastTick = Clock_getTicks();

			if (skipPulse(HIGH)) return DHT11_ERROR_TIMEOUT;

			//
			//	Calculate width of last HIGH pulse.
			//
			width = (Clock_getTicks() - lastTick) * Clock_tickPeriod;

			//
			//	Shift in the data, MSB first if width > threshold.
			//
			bytes[i] |= ((width > DHT11_THRESHOLD) << (7 - j));
		}
	}

	//
	//	Restore Swis.
	//
	Swi_restore(key);

	//
	//	Get the checksum and look an error.
	//
	uint8_t checkSum = 0;
	for (i = 0; i < (DHT11_NUM_BYTES - 1); i++)
	{
		checkSum += bytes[i];
	}
	if (checkSum != bytes[4])
	{
		return DHT11_ERROR_CHECKSUM;
	}

	humidity    = bytes[0];
	temperature = bytes[2];

  //
	//	Deallocate pins to return to the initial configuration.
	//
	PIN_close(DHT11_handle);

	return DHT11_OK;
}

//
//	This task functions runs an infinite loop where
//	the temperature sensor (DHT11) is read every 3 ms.
//
void DHT11_task(UArg arg0, UArg arg1)
{
	uint32_t lastTick = 0;
	uint32_t delayTime = (3000000 / Clock_tickPeriod);

	while(1)
	{
		//
		//	Refresh the temperature on the display every 3 seconds.
		//
		if ((Clock_getTicks()  - lastTick) > delayTime)
		{
			if (readSensor() != DHT11_OK)
			{
				PIN_close(DHT11_handle);
			}
			lastTick = Clock_getTicks();
		}
	}
}

int main(void)
{
	Task_Params  DHT11_taskParams;
	Clock_Params Display_clkParams;

	//
	//	Power manager initialization.
	//
	Power_init();

	//
	//	PIN module initialization.
	//
	if (PIN_init(gpioInitTable) != PIN_SUCCESS)
	{
		System_abort("Error initializing PIN module\n");
	}

	//
	//	Allocate collection of pins.
	//
	Display_segmentHandle = PIN_open(&Display_segmentState, Display_semgmentTable);
	if (!Display_segmentHandle)
	{
		System_abort("Error allocating Display_semgmentTable\n");
	}

	Display_digitHandle = PIN_open(&Display_digitState, Display_digitTable);
	if (!Display_digitHandle)
	{
		System_abort("Error allocating Display_digitTable\n");
	}

	//
	//	Construct DHT11 task thread.
	//
	Task_Params_init(&DHT11_taskParams);
	DHT11_taskParams.stackSize = STACK_SIZE;
	DHT11_taskParams.stack = DHT11_taskStack;
	Task_construct(&DHT11_taskStruct, (Task_FuncPtr)DHT11_task, &DHT11_taskParams, NULL);

	//
	//	Construct a periodic Clock Instance.
	//
	Clock_Params_init(&Display_clkParams);
	Display_clkParams.period = 1000;
	Display_clkParams.startFlag = TRUE;
	Clock_construct(&Display_ClkStruct, (Clock_FuncPtr)Display_Clock, 1000, &Display_clkParams);

  BIOS_start();

  return (0);
}
