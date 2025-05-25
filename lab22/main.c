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
#include "driverlib/timer.h"
#include "driverlib/adc.h"
#include <stdio.h>

#define ADC_SEQUENCER 3 // Usando o sequenciador 3 do ADC para uma única amostra

uint32_t SysClock;

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
    STATE_OFF,
    STATE_LOW,
    STATE_MEDIUM,
		STATE_HIGH
} State;

#define LEDS_ON_1 0
#define LEDS_ON_12   1
#define LEDS_ON_123   2
#define LEDS_ON_ALL  3

#define LOW 25
#define MEDIUM 30
#define HIGH 35

State currentState = STATE_INIT;
uint32_t adcValue;


void ledsOn(int leds) {
    uint8_t portN_value = 0;
    uint8_t portF_value = 0;

    switch (leds) {
        case LEDS_ON_1:
            portN_value = LED_PIN_1; // Apenas LED1
            break;

        case LEDS_ON_12:
            portN_value = LED_PIN_0 | LED_PIN_1; // LED1 e LED2
            break;

        case LEDS_ON_123:
            portN_value = LED_PIN_0 | LED_PIN_1;        // LED1 e LED2
            portF_value = LED_PIN_4;                    // LED3
            break;

        case LEDS_ON_ALL:
            portN_value = LED_PIN_0 | LED_PIN_1;        // LED1 e LED2
            portF_value = LED_PIN_0 | LED_PIN_4;        // LED3 e LED4
            break;

        default:
            break;
    }

    GPIOPinWrite(LED_PORTN, LED_PIN_0 | LED_PIN_1, portN_value);
    GPIOPinWrite(LED_PORTF, LED_PIN_0 | LED_PIN_4, portF_value);
}

void UARTIntHandler(void) {
    uint32_t status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, status);
}

void Timer0IntHandler(void) {
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // Lê o valor do ADC (potenciômetro)
    ADCProcessorTrigger(ADC0_BASE, ADC_SEQUENCER);
    while(!ADCIntStatus(ADC0_BASE, ADC_SEQUENCER, false));
    ADCIntClear(ADC0_BASE, ADC_SEQUENCER);
    ADCSequenceDataGet(ADC0_BASE, ADC_SEQUENCER, &adcValue);

    // Exibe via UART (opcional)
    char buffer[10];
    int len = snprintf(buffer, sizeof(buffer), "%u\r\n", adcValue);
    for (int i = 0; i < len; i++) {
        UARTCharPut(UART0_BASE, buffer[i]);
    }

    // Define o estado com base na porcentagem do valor ADC
    if (adcValue < 1024) {
        currentState = STATE_OFF;
    } else if (adcValue < 2048) {
        currentState = STATE_LOW;
    } else if (adcValue < 3072) {
        currentState = STATE_MEDIUM;
    } else {
        currentState = STATE_HIGH;
    }
}


void SetupTimer(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    uint32_t timerPeriod = SysClock / 100; 
    TimerLoadSet(TIMER0_BASE, TIMER_A, timerPeriod - 1);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntRegister(TIMER0_BASE, TIMER_A, Timer0IntHandler);
    TimerEnable(TIMER0_BASE, TIMER_A);
}

void SetupADC(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0));
    ADCSequenceConfigure(ADC0_BASE, ADC_SEQUENCER, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, ADC_SEQUENCER, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, ADC_SEQUENCER);
    ADCIntClear(ADC0_BASE, ADC_SEQUENCER);
}

void SetupUart(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
    UARTConfigSetExpClk(UART0_BASE, SysClock, 115200,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    UARTFIFODisable(UART0_BASE);
    UARTIntEnable(UART0_BASE, UART_INT_RX);
    UARTIntRegister(UART0_BASE, UARTIntHandler);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
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

int main(void) {
    SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);
    
    SetupUart();
    SetupTimer();
    SetupADC();
    ConfigLEDs();

    IntMasterEnable();

    while (1) {
        switch (currentState) {
            case STATE_OFF:
                ledsOn(LEDS_ON_1);
                break;

            case STATE_LOW:
								ledsOn(LEDS_ON_12);
                break;

            case STATE_MEDIUM:
								ledsOn(LEDS_ON_123);
                break;

            case STATE_HIGH:
								ledsOn(LEDS_ON_ALL);
                break;
        }

        __asm(" WFI");
    }
}
