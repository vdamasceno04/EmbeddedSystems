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
#include <time.h>
#include <stdio.h>

#define LED_PORTN GPIO_PORTN_BASE
#define LED_PORTF GPIO_PORTF_BASE
#define LED_PIN_1 GPIO_PIN_1 // LED1 (PN1)
#define LED_PIN_0 GPIO_PIN_0 // LED2 (PN0/PF0)
#define LED_PIN_4 GPIO_PIN_4 // LED3 (PF4)

#define BUTTON_PORT   GPIO_PORTJ_BASE
#define BUTTON1_PIN   GPIO_PIN_0
#define BUTTON2_PIN   GPIO_PIN_1

#define LEDS_ON_NONE 0
#define LEDS_ON_12 1
#define LEDS_ON_34 2
#define LEDS_ON_ALL 3

volatile bool timeoutFlag = false;
volatile uint32_t startTime = 0;
volatile uint32_t reactionTime = 0;

uint32_t SysClock;
bool resetFlag = false;
int state = 0;
volatile uint32_t counter_ms = 0;
volatile bool counting = false;
volatile uint32_t debounceCounter = 0;
volatile bool debounceActive = false;
volatile bool reactionCaptured = false;


void UARTSendString(const char *str);

void SysTick_Handler(void) {
    if (counting) {
        counter_ms++;
        if (counter_ms >= 5000 & state == 1) {  // Tempo máximo para resposta
            state = 99;
            counting = false;
        }
    }

    if (debounceActive) {
        debounceCounter++;
        if (debounceCounter >= 200) {  // 200 ms de debounce
            debounceActive = false;
            debounceCounter = 0;
        }
    }
}



void Button_Handler(void) {
    if (debounceActive) return; // Ignora se ainda estiver em debounce

    uint32_t status = GPIOIntStatus(BUTTON_PORT, true); // Verifica qual botão
    GPIOIntClear(BUTTON_PORT, status); // Limpa flags

    debounceActive = true; // Ativa debounce

		if (status & BUTTON1_PIN) {
			if (state == 1 && counting) {
					reactionTime = counter_ms;
					reactionCaptured = true;
					counting = false;
					state = 2;
			}
		 else if (state == 0) {
					state = 1; // Só avança para 1 a partir do estado 0
				}
				
		}


    if (status & BUTTON2_PIN) {
        resetFlag = true;
    }
}


void UARTIntHandler(void) {
    uint32_t status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, status);
    uint8_t received = (uint8_t)UARTCharGetNonBlocking(UART0_BASE);
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
    }
}


// UART configuração
void SetupUart(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
    UARTConfigSetExpClk(UART0_BASE, SysClock, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    UARTFIFODisable(UART0_BASE);
    UARTIntEnable(UART0_BASE, UART_INT_RX);
    UARTIntRegister(UART0_BASE, UARTIntHandler);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
}

// LEDs configuração
void ConfigLEDs(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    GPIOPinTypeGPIOOutput(LED_PORTN, LED_PIN_1 | LED_PIN_0);
    GPIOPinTypeGPIOOutput(LED_PORTF, LED_PIN_4 | LED_PIN_0);
}

void ConfigPBs(void){
	GPIOPinTypeGPIOInput(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN);
	GPIOPadConfigSet(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}
	// Enviar string pela UART
void UARTSendString(const char *str) {
    while (*str != '\0') {
        UARTCharPut(UART0_BASE, *str++);
    }
}

int main(void) {
    SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);
    ConfigLEDs();
	
		// Configure BUTTON1_PIN and BUTTON2_PIN as input with pull-up resistor
    GPIOPinTypeGPIOInput(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN);
    GPIOPadConfigSet(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Configure Button interrupt
    GPIOIntDisable(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN);
    GPIOIntClear(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN);
    GPIOIntTypeSet(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN, GPIO_RISING_EDGE);
    GPIOIntRegister(BUTTON_PORT, Button_Handler);
    GPIOIntEnable(BUTTON_PORT, BUTTON1_PIN | BUTTON2_PIN);
    SetupUart();
		ConfigPBs();
	    // Configure SysTick
    SysTickPeriodSet(SysClock / 1000); // 1ms period
    SysTickIntEnable();
    SysTickEnable();

    // Enable Interrupts
    IntMasterEnable();
	
		while(1){
        if (state == 0) {
					ledsOn(LEDS_ON_ALL);
					UARTSendString("estado 0\r\n");
					reactionTime = 0;
					counter_ms = 0;
        }
				else if (state == 1){
					UARTSendString("estado 1\r\n");
					counting = true;    // Inicia contagem
				}


		else if (state == 2){
				counting = false;
				UARTSendString("estado 2\r\n");
				if (reactionCaptured) {
						char buffer[64];
						snprintf(buffer, sizeof(buffer), "Tempo de reacao: %lu ms\r\n", reactionTime);
						UARTSendString(buffer);
						reactionCaptured = false;  // Limpa a flag
				}
				state += 1;
		}							
			
				else if (state == 99)
					UARTSendString("TIMEOUT\r\n");
					ledsOn(LEDS_ON_34);
				
			if (resetFlag) {
					resetFlag = false;
					state = 0;
					counter_ms = 0;        // Zera contador
					counting = false;      // Para contagem
					debounceActive = false; // Garante que botão esteja liberado
					reactionTime = 0;
					ledsOn(LEDS_ON_ALL);   // LEDs padrão
			}


        __asm(" WFI"); 
	}
}