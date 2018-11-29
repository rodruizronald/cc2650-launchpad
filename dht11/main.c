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
//
//	TI-RTOS Header files.
//
#include <ti/drivers/PIN.h>
#include <ti/drivers/Power.h>

#define DHT11	            				PIN_ID(25)
#define DHT11_OK									0
#define DHT11_ERROR_TIMEOUT				1
#define DHT11_ERROR_CHECKSUM			2
#define DHT11_NUM_BYTES						5
#define DHT11_THRESHOLD						45

#define HIGH											1
#define LOW												0

#define STACK_SIZE								512

//
//	Task structure and stack.
//
Task_Struct DHT11_taskStruct;
Char DHT11_taskStack[STACK_SIZE];

//
//	PIN driver handle.
//
PIN_Handle DHT11_handle;
PIN_State  DHT11_state;

//
//	PIN initial configuration table - I/O.
//
PIN_Config gpioInitConfig[] =
{
	DHT11 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	PIN_TERMINATE
};

//
//	DHT11 pin configuration tables.
//
PIN_Config DHT11_outputConfig[] =
{
	DHT11 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	PIN_TERMINATE
};

PIN_Config DHT11_inputConfig[] =
{
	DHT11 | PIN_INPUT_EN | PIN_NOPULL,
	PIN_TERMINATE
};

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

uint8_t readSensor(uint8_t* temperature, uint8_t* humidity)
{
	//
	//	Allocate collection of pins based on the DHT11_outputConfig table.
	//
	DHT11_handle = PIN_open(&DHT11_state, DHT11_outputConfig);
	if (!DHT11_handle) System_abort("Error allocating pins - DHT11_outputConfig\n");

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
	//	Allocate collection of pins based on the DHT11_inputConfig table.
	//
	DHT11_handle = PIN_open(&DHT11_state, DHT11_inputConfig);
	if (!DHT11_handle) System_abort("Error allocating pins - DHT11_inputConfig\n");

	//
	//	Skip the following pulses.
	//
	if (skipPulse(HIGH)) return DHT11_ERROR_TIMEOUT;
	if (skipPulse(LOW))  return DHT11_ERROR_TIMEOUT;
	if (skipPulse(HIGH)) return DHT11_ERROR_TIMEOUT;

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
			//	Shift in the data, msb first if width > threshold.
			//
			bytes[i] |= ((width > DHT11_THRESHOLD) << (7 - j));
		}
	}

	uint8_t checkSum = 0;
	for (i = 0; i < (DHT11_NUM_BYTES - 1); i++)  checkSum += bytes[i];
	if (checkSum != bytes[4]) return DHT11_ERROR_CHECKSUM;

	*humidity    = bytes[0];
	*temperature = bytes[2];

	//
	//	Deallocate pins to return to the initial configuration.
	//
	PIN_close(DHT11_handle);

	return DHT11_OK;
}

void DHT11_task(UArg arg0, UArg arg1)
{
	uint8_t temperature = 0, humidity = 0;

	while(1)
	{
		switch (readSensor(&temperature, &humidity))
		{
			case DHT11_OK:
				System_printf("temperature: %d, humidity: %d\n", temperature, humidity);
				break;

			case DHT11_ERROR_TIMEOUT:
				System_printf("DHT11_ERROR_TIMEOUT\n");
				break;

			case DHT11_ERROR_CHECKSUM:
				System_printf("DHT11_ERROR_CHECKSUM\n");
				break;
		}

		System_flush();
		Task_sleep(3000000 / Clock_tickPeriod);
	}
}

int main(void)
{
	Task_Params DHT11_taskParams;

	//
	//	Power manager initialization.
	//
	Power_init();

	//
	//	PIN module initialization.
	//
	if (PIN_init(gpioInitConfig) != PIN_SUCCESS)
	{
		System_abort("Error initializing PIN module\n");
	}

	//
	//	Construct DHT11 task thread.
	//
	Task_Params_init(&DHT11_taskParams);
	DHT11_taskParams.stackSize = STACK_SIZE;
	DHT11_taskParams.stack = DHT11_taskStack;
	Task_construct(&DHT11_taskStruct, (Task_FuncPtr)DHT11_task, &DHT11_taskParams, NULL);

	//
	//	Start BIOS.
	//
	BIOS_start();

	return (0);
}
