#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"

#define LED_PORTN GPIO_PORTN_BASE
#define LED_PORTF GPIO_PORTF_BASE
#define LED_PIN_1 GPIO_PIN_1 // LED1 (PN1)
#define LED_PIN_0 GPIO_PIN_0 // LED2 (PN0/PF0)
#define LED_PIN_4 GPIO_PIN_4 // LED3 (PF4)

#define PB_PORT GPIO_PORTJ_BASE
#define PB_1 GPIO_PIN_0
#define PB_2 GPIO_PIN_1

uint32_t SysClock;
volatile char rxbuffer = 0;

void UARTSendString(const char *str);

// Handler UART
void UARTIntHandler(void) {
    uint32_t status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, status);

    uint8_t received = (uint8_t)UARTCharGetNonBlocking(UART0_BASE);
		if (received == '\r') { // ENTER foi apertado
        switch (rxbuffer) {
            case '1':
                GPIOPinWrite(LED_PORTN, LED_PIN_1, ~(GPIOPinRead(LED_PORTN, LED_PIN_1)) & LED_PIN_1);
                UARTSendString("LED 1 Trocado\r\n");
                break;
            case '2':
                GPIOPinWrite(LED_PORTN, LED_PIN_0, ~(GPIOPinRead(LED_PORTN, LED_PIN_0)) & LED_PIN_0);
                UARTSendString("LED 2 Trocado\r\n");
                break;
            case '3':
                GPIOPinWrite(LED_PORTF, LED_PIN_4, ~(GPIOPinRead(LED_PORTF, LED_PIN_4)) & LED_PIN_4);
                UARTSendString("LED 3 Trocado\r\n");
                break;
            case '4':
                GPIOPinWrite(LED_PORTF, LED_PIN_0, ~(GPIOPinRead(LED_PORTF, LED_PIN_0)) & LED_PIN_0);
                UARTSendString("LED 4 Trocado\r\n");
                break;
						case '5':
								if (GPIOPinRead(PB_PORT, PB_1)==0)
									UARTSendString("Push Button 1 pressionado\r\n");
								else
									UARTSendString("Push Button 1 solto\r\n");
								break;
						case '6':
								if (GPIOPinRead(PB_PORT, PB_2)==0)
									UARTSendString("Push Button 2 pressionado\r\n");
								else
									UARTSendString("Push Button 2 solto\r\n");
								break;
						default:
                UARTSendString("Tecla invalida\r\n");
                break;
        }
        rxbuffer = 0; // limpa o buffer para a próxima entrada
    } else {
        rxbuffer = received; // guarda o caractere para quando ENTER for apertado
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
	GPIOPinTypeGPIOInput(PB_PORT, PB_1 | PB_2);
	GPIOPadConfigSet(PB_PORT, PB_1 | PB_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
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
    SetupUart();
		ConfigPBs();
    while (1) {
        __asm(" WFI"); // Espera por interrupção
    }
}
