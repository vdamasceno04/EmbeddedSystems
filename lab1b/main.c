#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/systick.h"
#include <stdio.h>

// Definições de tempo
#define TIMEOUT_MS    5000
#define DEBOUNCE_MS   150

// LEDs
#define LED_PORTN GPIO_PORTN_BASE
#define LED_PORTF GPIO_PORTF_BASE
#define LED_PIN_1 GPIO_PIN_1
#define LED_PIN_0 GPIO_PIN_0
#define LED_PIN_4 GPIO_PIN_4

// Botões
#define BUTTON_PORT   GPIO_PORTJ_BASE
#define BUTTON1_PIN   GPIO_PIN_0
#define BUTTON2_PIN   GPIO_PIN_1

// Estados do sistema
typedef enum {
    STATE_INIT = 0,
    STATE_WAIT_REACTION,
    STATE_SHOW_RESULT,
    STATE_TIMEOUT
} State;

#define LEDS_ON_NONE 0
#define LEDS_ON_12   1
#define LEDS_ON_34   2
#define LEDS_ON_ALL  3

volatile uint32_t sysClock;
volatile uint32_t reactionCounterMs = 0;
volatile bool isCounting = false;
volatile uint32_t debounceCounter = 0;
volatile bool debounceActive = false;
volatile bool reactionCaptured = false;
volatile bool shouldReset = false;
volatile uint32_t reactionTime = 0;
State currentState = STATE_INIT;

void UARTSendString(const char *str);

void SysTick_Handler(void) {
    if (isCounting) {
        reactionCounterMs++;
        if (reactionCounterMs >= TIMEOUT_MS && currentState == STATE_WAIT_REACTION) {
            currentState = STATE_TIMEOUT;
            isCounting = false;
        }
    }

    if (debounceActive) {
        debounceCounter++;
        if (debounceCounter >= DEBOUNCE_MS) {
            debounceActive = false;
            debounceCounter = 0;
        }
    }
}

void Button_Handler(void) {
    if (debounceActive) return;

    uint32_t status = GPIOIntStatus(BUTTON_PORT, true);
    GPIOIntClear(BUTTON_PORT, status);

    debounceActive = true;

    if (status & BUTTON1_PIN) {
        if (currentState == STATE_WAIT_REACTION && isCounting) {
            reactionTime = reactionCounterMs;
            reactionCaptured = true;
            isCounting = false;
            currentState = STATE_SHOW_RESULT;
        } else if (currentState == STATE_INIT) {
            currentState = STATE_WAIT_REACTION;
        }
    }

    if (status & BUTTON2_PIN) {
        shouldReset = true;
    }
}

void ledsOn(int leds) {
    switch (leds) {
        case LEDS_ON_NONE:
            GPIOPinWrite(LED_PORTN, LED_PIN_1 | LED_PIN_0, 0);
            GPIOPinWrite(LED_PORTF, LED_PIN_4 | LED_PIN_0, 0);
            break;
        case LEDS_ON_12:
            GPIOPinWrite(LED_PORTN, LED_PIN_1 | LED_PIN_0, LED_PIN_1 | LED_PIN_0);
            GPIOPinWrite(LED_PORTF, LED_PIN_4 | LED_PIN_0, 0);
            break;
        case LEDS_ON_34:
            GPIOPinWrite(LED_PORTN, LED_PIN_1 | LED_PIN_0, 0);
            GPIOPinWrite(LED_PORTF, LED_PIN_4 | LED_PIN_0, LED_PIN_4 | LED_PIN_0);
            break;
        case LEDS_ON_ALL:
            GPIOPinWrite(LED_PORTN, LED_PIN_1 | LED_PIN_0, LED_PIN_1 | LED_PIN_0);
            GPIOPinWrite(LED_PORTF, LED_PIN_4 | LED_PIN_0, LED_PIN_4 | LED_PIN_0);
            break;
        default:
            GPIOPinWrite(LED_PORTN, LED_PIN_1 | LED_PIN_0, 0);
            GPIOPinWrite(LED_PORTF, LED_PIN_4 | LED_PIN_0, 0);
            break;
    }
}

void SetupUart(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
    UARTConfigSetExpClk(UART0_BASE, sysClock, 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    UARTFIFODisable(UART0_BASE);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
}

void ConfigLEDs(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(LED_PORTN, LED_PIN_1 | LED_PIN_0);
    GPIOPinTypeGPIOOutput(LED_PORTF, LED_PIN_4 | LED_PIN_0);
}

void ConfigPBs(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ));
    GPIOPinTypeGPIOInput(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN);
    GPIOPadConfigSet(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    GPIOIntDisable(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN);
    GPIOIntClear(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN);
    GPIOIntTypeSet(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN, GPIO_RISING_EDGE);
    GPIOIntRegister(BUTTON_PORT, Button_Handler);
    GPIOIntEnable(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN);
}

void UARTSendString(const char *str) {
    while (*str != '\0') {
        UARTCharPut(UART0_BASE, *str++);
    }
}

int main(void) {
    sysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
                                   SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);

    ConfigLEDs();
    ConfigPBs();
    SetupUart();

    SysTickPeriodSet(sysClock / 1000);
    SysTickIntEnable();
    SysTickEnable();

    IntMasterEnable();

    while (1) {
        switch (currentState) {
            case STATE_INIT:
                ledsOn(LEDS_ON_ALL);
                reactionCounterMs = 0;
                reactionTime = 0;
                break;

            case STATE_WAIT_REACTION:
                isCounting = true;
                break;

            case STATE_SHOW_RESULT:
                isCounting = false;
                if (reactionCaptured) {
                    char buffer[64];
                    snprintf(buffer, sizeof(buffer),
                             "Tempo de reacao: %lu ms\r\n", reactionTime);
                    UARTSendString(buffer);
                    reactionCaptured = false;
                }
                ledsOn(LEDS_ON_12);
                break;

            case STATE_TIMEOUT:
                UARTSendString("TIMEOUT\r\n");
                ledsOn(LEDS_ON_34);
                currentState ++;
                break;
        }

        if (shouldReset) {
            shouldReset = false;
            currentState = STATE_INIT;
            reactionCounterMs = 0;
            isCounting = false;
            debounceActive = false;
            reactionTime = 0;
            ledsOn(LEDS_ON_ALL);
        }

        __asm(" WFI");
    }
}
