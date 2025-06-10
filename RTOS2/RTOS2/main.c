#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cmsis_os2.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"

#define MAX_N 100

typedef struct {
    uint32_t N;
    int data[MAX_N];
} SortData;

osMessageQueueId_t queueBubble;
osMessageQueueId_t queueInsert;
osMessageQueueId_t queueQuick;
osMutexId_t uartMutex;

uint32_t SysClock;
char inputBuffer[100];
int bufferIndex = 0;
volatile bool inputReady = false;

volatile bool bubbleDone = true;
volatile bool insertionDone = true;
volatile bool quickDone = true;

void SetupUart(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
    UARTConfigSetExpClk(UART0_BASE, SysClock, 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    UARTFIFODisable(UART0_BASE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
}

void UARTIntHandler(void) {
    uint32_t status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, status);
    char receivedChar;

    while (UARTCharsAvail(UART0_BASE)) {
        receivedChar = (char)UARTCharGet(UART0_BASE);
        UARTCharPut(UART0_BASE, receivedChar);
        if (receivedChar == '\r' || receivedChar == '\n') {
            inputBuffer[bufferIndex] = '\0';
            bufferIndex = 0;
            inputReady = true;
        } else if (bufferIndex < sizeof(inputBuffer) - 1) {
            inputBuffer[bufferIndex++] = receivedChar;
        }
    }
}

void SetupTimer(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));
    TimerConfigure(TIMER0_BASE, TIMER_CFG_ONE_SHOT);
}

void start_us(void) {
    TimerDisable(TIMER0_BASE, TIMER_A);
    TimerLoadSet(TIMER0_BASE, TIMER_A, 0xFFFFFFFF);
    TimerEnable(TIMER0_BASE, TIMER_A);
}

uint32_t stop_us(void) {
    TimerDisable(TIMER0_BASE, TIMER_A);
    uint32_t elapsed = 0xFFFFFFFF - TimerValueGet(TIMER0_BASE, TIMER_A);
    return (uint32_t)(((uint64_t)elapsed * 1000000) / SysClock);
}

void logMessage(const char *fmt, ...) {
    osMutexAcquire(uartMutex, osWaitForever);
    char logBuf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(logBuf, sizeof(logBuf), fmt, args);
    va_end(args);
    for (char *p = logBuf; *p; p++) {
        UARTCharPut(UART0_BASE, *p);
    }
    osMutexRelease(uartMutex);
}

void prompt() {
    logMessage("\r\nEnter N: ");
}

void printArray(const char* algo, int arr[], uint32_t N) {
    char buf[256];
    int pos = snprintf(buf, sizeof(buf), "Tick %lu [%s]: ", osKernelGetTickCount(), algo);
    for (uint32_t i = 0; i < N; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%d ", arr[i]);
        if (pos >= sizeof(buf)) break;
    }
    snprintf(buf + pos, sizeof(buf) - pos, "\r\n");
    logMessage("%s", buf);
}

void printFinalSorted(const char* algo, int arr[], uint32_t N, uint32_t duration) {
    char buf[256];
    int pos = snprintf(buf, sizeof(buf), "Tick %lu [%s]: Final Sorted: ", osKernelGetTickCount(), algo);
    for (uint32_t i = 0; i < N; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%d ", arr[i]);
        if (pos >= sizeof(buf)) break;
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "| Time: %lu us\r\n", duration);
    logMessage("%s", buf);
}

bool isSorted(int arr[], uint32_t N) {
    for (uint32_t i = 0; i < N - 1; i++) {
        if (arr[i] > arr[i + 1]) return false;
    }
    return true;
}

void bubbleSort(int arr[], uint32_t N, const char* algo, volatile bool* doneFlag) {
    start_us();
    for (uint32_t i = 0; i < N - 1; i++) {
        for (uint32_t j = 0; j < N - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int t = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = t;
            }
            osDelay(1);  // Force preemption
        }
        printArray(algo, arr, N);
        if (isSorted(arr, N)) {
            uint32_t duration = stop_us();
            printFinalSorted(algo, arr, N, duration);
            *doneFlag = true;
            return;
        }
    }
    uint32_t duration = stop_us();
    printFinalSorted(algo, arr, N, duration);
    *doneFlag = true;
}

void insertionSort(int arr[], uint32_t N, const char* algo, volatile bool* doneFlag) {
    start_us();
    for (uint32_t i = 1; i < N; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j]; j--;
            osDelay(1);  // Force preemption
        }
        arr[j + 1] = key;
        printArray(algo, arr, N);
        if (isSorted(arr, N)) {
            uint32_t duration = stop_us();
            printFinalSorted(algo, arr, N, duration);
            *doneFlag = true;
            return;
        }
        osDelay(1);
    }
    uint32_t duration = stop_us();
    printFinalSorted(algo, arr, N, duration);
    *doneFlag = true;
}

void quickSort(int arr[], int low, int high, uint32_t N, const char* algo, volatile bool* doneFlag, bool* sorted, uint32_t* duration) {
    if (*sorted) return;

    if (low < high) {
        int pivot = arr[high];
        int i = (low - 1);
        for (int j = low; j <= high - 1; j++) {
            if (arr[j] <= pivot) {
                i++;
                int t = arr[i]; arr[i] = arr[j]; arr[j] = t;
            }
            osDelay(1);  // Force preemption
        }
        int t = arr[i + 1]; arr[i + 1] = arr[high]; arr[high] = t;

        printArray(algo, arr, N);
        if (isSorted(arr, N)) {
            *duration = stop_us();
            printFinalSorted(algo, arr, N, *duration);
            *doneFlag = true;
            *sorted = true;
            return;
        }

        osDelay(1);
        quickSort(arr, low, i, N, algo, doneFlag, sorted, duration);
        osDelay(1);
        quickSort(arr, i + 2, high, N, algo, doneFlag, sorted, duration);
    }
}

void Thread_Sort(void *argument) {
    const char *algo = (const char *)argument;
    osMessageQueueId_t queue;
    volatile bool* doneFlag;

    if (algo == "Bubble") { queue = queueBubble; doneFlag = &bubbleDone; }
    else if (algo == "Insertion") { queue = queueInsert; doneFlag = &insertionDone; }
    else { queue = queueQuick; doneFlag = &quickDone; }

    while (1) {
        SortData data;
        osMessageQueueGet(queue, &data, NULL, osWaitForever);

        if (algo == "Bubble") {
            bubbleSort(data.data, data.N, algo, doneFlag);
        } else if (algo == "Insertion") {
            insertionSort(data.data, data.N, algo, doneFlag);
        } else {
            start_us();
            bool sorted = false;
            uint32_t duration = 0;
            quickSort(data.data, 0, data.N - 1, data.N, algo, doneFlag, &sorted, &duration);
            if (!sorted) {
                duration = stop_us();
                printFinalSorted(algo, data.data, data.N, duration);
                *doneFlag = true;
            }
        }
    }
}

void Thread_Input(void *argument) {
    memset(inputBuffer, 0, sizeof(inputBuffer));
    osDelay(200);

    while (1) {
        while (!(bubbleDone && insertionDone && quickDone)) {
            osDelay(10);
        }

        prompt();
        memset(inputBuffer, 0, sizeof(inputBuffer));
        bufferIndex = 0;
        inputReady = false;

        while (!inputReady) {
            osDelay(10);
        }

        uint32_t N = strtoul(inputBuffer, NULL, 10);
        inputReady = false;

        if (N == 0 || N > MAX_N) {
            logMessage("Invalid N! Max %d.\r\n", MAX_N);
            continue;
        }

        SortData data;
        data.N = N;

        logMessage("\r\nRandom numbers: ");
        for (uint32_t i = 0; i < N; i++) {
            data.data[i] = rand() % 1000;
            logMessage("%d ", data.data[i]);
        }
        logMessage("\r\n");

        bubbleDone = false;
        insertionDone = false;
        quickDone = false;

        osMessageQueuePut(queueBubble, &data, 0, 0);
        osMessageQueuePut(queueInsert, &data, 0, 0);
        osMessageQueuePut(queueQuick, &data, 0, 0);
    }
}

int main(void) {
    SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);

    SetupUart();
    SetupTimer();

    osKernelInitialize();

    queueBubble = osMessageQueueNew(10, sizeof(SortData), NULL);
    queueInsert = osMessageQueueNew(10, sizeof(SortData), NULL);
    queueQuick = osMessageQueueNew(10, sizeof(SortData), NULL);

    uartMutex = osMutexNew(NULL);

		osThreadAttr_t bubbleAttr = { .priority = osPriorityHigh, .stack_size = 4096 };
		osThreadAttr_t insertAttr = { .priority = osPriorityNormal, .stack_size = 4096 };
		osThreadAttr_t quickAttr = { .priority = osPriorityLow, .stack_size = 4096 };


    osThreadNew(Thread_Sort, (void *)"Bubble", &bubbleAttr);
    osThreadNew(Thread_Sort, (void *)"Insertion", &insertAttr);
    osThreadNew(Thread_Sort, (void *)"Quick", &quickAttr);
		
		
		
    osThreadNew(Thread_Input, NULL, NULL);

    UARTIntRegister(UART0_BASE, UARTIntHandler);
    UARTIntEnable(UART0_BASE, UART_INT_RX);

    osKernelStart();

    while (1);
}
