#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"

#define LED_PORT      GPIO_PORTN_BASE
#define LED_PIN       GPIO_PIN_0
#define BUTTON1_PORT  GPIO_PORTJ_BASE
#define BUTTON1_PIN   GPIO_PIN_0
#define BUTTON2_PORT  GPIO_PORTJ_BASE
#define BUTTON2_PIN   GPIO_PIN_1

volatile bool blink = false;

void SysTick_Handler(void) {
    if (blink) {
        GPIOPinWrite(LED_PORT, LED_PIN, ~GPIOPinRead(LED_PORT, LED_PIN) & LED_PIN);
    }
}

int main(void) {
    // Set system clock to 120MHz
    SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 120000000);

    // Enable peripherals for LED and Buttons
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    // Configure LED_PIN as output
    GPIOPinTypeGPIOOutput(LED_PORT, LED_PIN);

    // Configure BUTTON1_PIN and BUTTON2_PIN as input with pull-up resistor
    GPIOPinTypeGPIOInput(BUTTON1_PORT, BUTTON1_PIN | BUTTON2_PIN);
    GPIOPadConfigSet(BUTTON1_PORT, BUTTON1_PIN | BUTTON2_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Configure SysTick
    SysTickPeriodSet(SysCtlClockGet() / 2); // 0.5s period
    SysTickIntEnable();
    SysTickEnable();

    // Enable Interrupts
    IntMasterEnable();

    while (1) {
        if (GPIOPinRead(BUTTON1_PORT, BUTTON1_PIN) == 0) { // Button 1 is pressed
            blink = false;
            GPIOPinWrite(LED_PORT, LED_PIN, LED_PIN);       // Turn on LED
        } else if (GPIOPinRead(BUTTON2_PORT, BUTTON2_PIN) == 0) { // Button 2 is pressed
            blink = true;
        } else {
            blink = false;
            GPIOPinWrite(LED_PORT, LED_PIN, 0);              // Turn off LED
        }
    }
}
