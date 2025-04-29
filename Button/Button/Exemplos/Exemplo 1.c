#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

#define LED_PORT      GPIO_PORTN_BASE
#define LED_PIN       GPIO_PIN_1
#define BUTTON_PORT   GPIO_PORTJ_BASE
#define BUTTON_PIN    GPIO_PIN_0

int main(void) {
    // Set system clock to 120MHz
    SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 120000000);

    // Enable peripherals for LED and Button
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    // Configure LED_PIN as output
    GPIOPinTypeGPIOOutput(LED_PORT, LED_PIN);

    // Configure BUTTON_PIN as input with pull-up resistor
    GPIOPinTypeGPIOInput(BUTTON_PORT, BUTTON_PIN);
    GPIOPadConfigSet(BUTTON_PORT, BUTTON_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    while(1) {
        if (GPIOPinRead(BUTTON_PORT, BUTTON_PIN) == 0) { // Button is pressed
            GPIOPinWrite(LED_PORT, LED_PIN, LED_PIN);   // Turn on LED
        } else {
            GPIOPinWrite(LED_PORT, LED_PIN, 0);          // Turn off LED
        }
    }
}
