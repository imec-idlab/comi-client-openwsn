/**
 * Author: Xavier Vilajosana (xvilajosana@eecs.berkeley.edu)
 *         Pere Tuset (peretuset@openmote.com)
 * Date:   July 2013
 * Description: CC2538-specific definition of the "debugpins" bsp module.
 */

#include <headers/hw_memmap.h>
#include <headers/hw_types.h>

#include <source/gpio.h>

#include "board.h"
#include "debugpins.h"
#include "ioc.h"

//=========================== defines =========================================
// Board dbPINS defines
#define BSP_PINA_BASE           GPIO_A_BASE
#define BSP_PIND_BASE           GPIO_D_BASE

#define BSP_PINA_4              GPIO_PIN_4      //!< PA4 -- frame -RF1.5 				//A-K Input for Now
#define BSP_PINA_5              GPIO_PIN_5      //!< PA5 -- isr   -RF1.11

#define BSP_PIND_3              GPIO_PIN_3      //!< PD3 -- slot  -RF1.6				//A-K App output for now
#define BSP_PIND_2              GPIO_PIN_2      //!< PD2 -- fsm   -RF1.8
#define BSP_PIND_1              GPIO_PIN_1      //!< PD1 -- task  -RF1.10
#define BSP_PIND_0              GPIO_PIN_0      //!< PD0 -- radio -RF1-12				// A-K we can get the radio duty cycle

//=========================== variables =======================================

//=========================== prototypes ======================================

void bspDBpinToggle(uint32_t base,uint8_t ui8Pin);
static void FireButtonIntHandler(void);
void (*fire_int_callback)(uint8_t value);
//=========================== public ==========================================

void debugpins_init() {

	   volatile uint32_t i;

	GPIOPinTypeGPIOOutput(BSP_PINA_BASE, BSP_PINA_5);
   //GPIOPinTypeGPIOOutput(BSP_PINA_BASE, BSP_PINA_4 | BSP_PINA_5);
   GPIOPinTypeGPIOOutput(BSP_PIND_BASE, BSP_PIND_3 | BSP_PIND_2 | BSP_PIND_1 | BSP_PIND_0);

   GPIOPinWrite(BSP_PINA_BASE, (BSP_PINA_5), 0x00);
   GPIOPinWrite(BSP_PIND_BASE, (BSP_PIND_3 | BSP_PIND_2 | BSP_PIND_1 | BSP_PIND_0), 0);

   fire_int_callback = NULL;
   /* The button is an input GPIO on falling edge */
   GPIOPinTypeGPIOInput(BSP_PINA_BASE, BSP_PINA_4);
   IOCPadConfigSet(BSP_PINA_BASE, BSP_PINA_4, IOC_OVERRIDE_PUE);
   GPIOIntTypeSet(BSP_PINA_BASE, BSP_PINA_4, GPIO_FALLING_EDGE);

   /* Register the interrupt */
   GPIOPortIntRegister(BSP_PINA_BASE, &FireButtonIntHandler);

   /* Clear and enable the interrupt */
   GPIOPinIntClear(BSP_PINA_BASE, BSP_PINA_4);
   IntMasterEnable();
   GPIOPinIntEnable(BSP_PINA_BASE, BSP_PINA_4);

}

static void FireButtonIntHandler(void) {
    uint32_t ui32GPIOIntStatus;

    /* Disable the interrupts and get the interrupt state */
    IntMasterDisable();

    /* Notify the user about the button being pressed */
    if(fire_int_callback != NULL) {
    	fire_int_callback(0);
    }

    GPIOPinIntClear(BSP_PINA_BASE, BSP_PINA_4);

    /* Enable back the interrupts */
    IntMasterEnable();
}

// PA4
//void debugpins_frame_toggle() {
//   bspDBpinToggle(BSP_PINA_BASE, BSP_PINA_4);
//}
//void debugpins_frame_clr() {
//    GPIOPinWrite(BSP_PINA_BASE, BSP_PINA_4, 0);
//}
//void debugpins_frame_set() {
//   GPIOPinWrite(BSP_PINA_BASE, BSP_PINA_4, BSP_PINA_4);
//}

// PD3
void debugpins_slot_toggle() {
	bspDBpinToggle(BSP_PIND_BASE, BSP_PIND_3);
}
void debugpins_slot_clr() {
	GPIOPinWrite(BSP_PIND_BASE, BSP_PIND_3, 0);
}
void debugpins_slot_set() {
	GPIOPinWrite(BSP_PIND_BASE, BSP_PIND_3, BSP_PIND_3);
}

// PD2
void debugpins_fsm_toggle() {
	bspDBpinToggle(BSP_PIND_BASE, BSP_PIND_2);
}
void debugpins_fsm_clr() {
	GPIOPinWrite(BSP_PIND_BASE, BSP_PIND_2, 0);
}
void debugpins_fsm_set() {
	GPIOPinWrite(BSP_PIND_BASE, BSP_PIND_2, BSP_PIND_2);
}

// PD1
void debugpins_task_toggle() {
	bspDBpinToggle(BSP_PIND_BASE,BSP_PIND_1);
}
void debugpins_task_clr() {
	GPIOPinWrite(BSP_PIND_BASE, BSP_PIND_1, 0);
}
void debugpins_task_set() {
	GPIOPinWrite(BSP_PIND_BASE, BSP_PIND_1, BSP_PIND_1);
}

// PA5
void debugpins_isr_toggle() {
	bspDBpinToggle(BSP_PINA_BASE, BSP_PINA_5);
}
void debugpins_isr_clr() {
	GPIOPinWrite(BSP_PINA_BASE, BSP_PINA_5, 0);
}
void debugpins_isr_set() {
	GPIOPinWrite(BSP_PINA_BASE, BSP_PINA_5, BSP_PINA_5);
}

// PD0
void debugpins_radio_toggle() {
	bspDBpinToggle(BSP_PIND_BASE, BSP_PIND_0);
}
void debugpins_radio_clr() {
	GPIOPinWrite(BSP_PIND_BASE, BSP_PIND_0, 0);
}
void debugpins_radio_set() {
	GPIOPinWrite(BSP_PIND_BASE, BSP_PIND_0, BSP_PIND_0);
}

//------------ private ------------//

void bspDBpinToggle(uint32_t base, uint8_t ui8Pin)
{
    //
    // Get current pin values of selected bits
    //
    uint32_t ui32Toggle = GPIOPinRead(base, ui8Pin);

    //
    // Invert selected bits
    //
    ui32Toggle = (~ui32Toggle) & ui8Pin;

    //
    // Set GPIO
    //
    GPIOPinWrite(base, ui8Pin, ui32Toggle);
}
