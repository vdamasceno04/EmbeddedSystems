#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "cmsis_os2.h"
#include "tm4c1294ncpdt.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"

// --- LEDs ---
#define LED_PORTN GPIO_PORTN_BASE
#define LED_PORTF GPIO_PORTF_BASE
#define LED_PIN_1 GPIO_PIN_1 // LED 1 (Thread 1)
#define LED_PIN_0 GPIO_PIN_0 // LED 2 (Thread 2)
#define LED_PIN_4 GPIO_PIN_4 // LED 3 (Thread 3)

// --- Variáveis Globais ---
uint32_t SysClock;

// --- Configuração dos Periféricos ---

void ConfigLEDs(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    GPIOPinTypeGPIOOutput(LED_PORTN, LED_PIN_1 | LED_PIN_0);
    GPIOPinTypeGPIOOutput(LED_PORTF, LED_PIN_4);
}

// --- Funções de Utilitário e Interrupção ---

// Thread para controlar o LED 1 (acende/apaga a cada 1 segundo)
void Thread_Led1(void *argument){
    while(1){
        uint8_t current_state = GPIOPinRead(LED_PORTN, LED_PIN_1);
        GPIOPinWrite(LED_PORTN, LED_PIN_1, ~current_state);
        osDelay(1000); // Espera 1000 ms
    }
}

// Thread para controlar o LED 2 (pisca 2x por segundo)
void Thread_Led2(void *argument){
    while(1){
        GPIOPinWrite(LED_PORTN, LED_PIN_0, LED_PIN_0); // Liga
        osDelay(250);
        GPIOPinWrite(LED_PORTN, LED_PIN_0, 0); // Desliga
        osDelay(250);
    }
}

// Thread para controlar o LED 3 (pisca a cada 0.5 segundo)
void Thread_Led3(void *argument){
    while(1){
        GPIOPinWrite(LED_PORTF, LED_PIN_4, LED_PIN_4); // Liga
        osDelay(250);
        GPIOPinWrite(LED_PORTF, LED_PIN_4, 0); // Desliga
        osDelay(250);
    }
}

int main(void) {
    // Configura o clock do sistema para 120MHz
    SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);
		SystemCoreClock = SysClock;
    ConfigLEDs();

    osKernelInitialize();
    osThreadAttr_t ledAttr = { .name = "LED_Thread", .priority = osPriorityNormal };

    osThreadNew(Thread_Led1, NULL, &ledAttr);
    osThreadNew(Thread_Led2, NULL, &ledAttr);
    osThreadNew(Thread_Led3, NULL, &ledAttr);

    osKernelStart();

    // Loop infinito de segurança
    while (1);
}