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
//
//	Bitwise operations.
//
#define _BV(bit)				(1 << (bit))

//
//	Defines for the seven segment display.
//
#define SEGMENT_A		PIN_ID(28)
#define SEGMENT_B		PIN_ID(30)
#define SEGMENT_C		PIN_ID(24)
#define SEGMENT_D		PIN_ID(22)
#define SEGMENT_E		PIN_ID(23)
#define SEGMENT_F		PIN_ID(29)
#define SEGMENT_G		PIN_ID(21)

#define DISPLAY_UNITS	PIN_ID(27)
#define DISPLAY_TENS	PIN_ID(26)

//
//	Default task stack size.
//
#define STACK_SIZE		512

//
//	Task structure and stack.
//
Task_Struct segmentDisplay_TaskStruct;
Char segmentDisplay_TaskStack[STACK_SIZE];

//
//	Clock structure.
//
Clock_Struct segmentDisplay_ClkStruct;

//
//	PIN driver handle.
//
PIN_Handle segmentPin_Handle;
PIN_State  segmentPin_State;

PIN_Handle displayPin_Handle;
PIN_State  displayPin_State;

//
//	PIN default pin configuration table.
//
PIN_Config gpioInit_Table[] =
{
	SEGMENT_A	  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_B	  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_C   | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_D	  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_E	  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_F	  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	SEGMENT_G	  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MIN,
	DISPLAY_UNITS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	DISPLAY_TENS  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	PIN_TERMINATE
};

//
//	PIN seven segment led pin configuration table.
//
PIN_Config segmentPin_Table[] =
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

//
//	PIN display digits pin configuration table.
//
PIN_Config displayPin_Table[] =
{
	DISPLAY_UNITS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	DISPLAY_TENS  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
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

bool flag = true;
uint8_t currentUnit = 0, currentTen = 0;

void segmentDisplay_Clock(UArg arg0)
{
	//
	//	Switch the display pin on from units to tens and vice versa.
	//
	if (flag)
	{
		PIN_setOutputValue(displayPin_Handle, DISPLAY_TENS,  0);
		PIN_setOutputValue(displayPin_Handle, DISPLAY_UNITS, 1);
		PIN_setPortOutputValue(segmentPin_Handle, displayNumber[currentUnit]);
	}
	else
	{
		PIN_setOutputValue(displayPin_Handle, DISPLAY_UNITS, 0);
		PIN_setOutputValue(displayPin_Handle, DISPLAY_TENS,  1);
		PIN_setPortOutputValue(segmentPin_Handle, displayNumber[currentTen]);
	}

	flag = !flag;
}

void segmentDisplay_Task(UArg arg0, UArg arg1)
{
	uint8_t i = 0, j = 0;
	while (1)
	{
		//
		//	Count up to 99 and restart.
		//
		for (i = 0; i < 10; i++)
		{
			currentTen = i;
			for (j = 0; j < 10; j++)
			{
				currentUnit = j;
				Task_sleep(500000 / Clock_tickPeriod);
			}
		}
	}
}

int main(void)
{
	Task_Params segmentDisplay_TaskParams;
	Clock_Params segmentDisplay_ClkParams;

	//
	//	Power manager initialization.
	//
	Power_init();

	//
	//	PIN module initialization.
	//
	if (PIN_init(gpioInit_Table) != PIN_SUCCESS)
	{
		System_abort("Error initializing gpioInit_Table\n");
	}

	//
	//	Allocate collection of pins.
	//
	segmentPin_Handle = PIN_open(&segmentPin_State, segmentPin_Table);
	if (!segmentPin_Handle)
	{
		System_abort("Error allocating segmentPin_Table\n");
	}

	displayPin_Handle = PIN_open(&displayPin_State, displayPin_Table);
	if (!displayPin_Handle)
	{
		System_abort("Error allocating displayPin_Table\n");
	}

	//
	//	Construct a Task thread.
	//
	Task_Params_init(&segmentDisplay_TaskParams);
	segmentDisplay_TaskParams.stackSize = STACK_SIZE;
	segmentDisplay_TaskParams.stack = segmentDisplay_TaskStack;
	Task_construct(&segmentDisplay_TaskStruct, (Task_FuncPtr)segmentDisplay_Task, &segmentDisplay_TaskParams, NULL);

	//
	//	Construct a periodic Clock Instance.
	//
	Clock_Params_init(&segmentDisplay_ClkParams);
	segmentDisplay_ClkParams.period = 1000;
	segmentDisplay_ClkParams.startFlag = TRUE;
	Clock_construct(&segmentDisplay_ClkStruct, (Clock_FuncPtr)segmentDisplay_Clock, 1000, &segmentDisplay_ClkParams);

  BIOS_start();

  return (0);
}
