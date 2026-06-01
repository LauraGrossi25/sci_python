//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "scicomm.h"

#define TAM_BUFFER_DAC 200
#define TAM_BUFFER_ADC 100

volatile Protocol_Header_t g_prot_header = {CMD_NONE,0};
volatile int g_dado;

uint16_t dac_buffer[TAM_BUFFER_DAC];
volatile uint16_t adc_buffer[TAM_BUFFER_ADC];

volatile uint32_t adc_isr_count = 0;

volatile uint32_t timer_isr_count = 0;

volatile uint16_t adc_teste = 0;

__interrupt void INT_myCPUTIMER0_ISR(void);
__interrupt void INT_myADC0_1_ISR(void);

//
// Fun��o Principal
//
void main(void)
{
    // Inicializa��o do dispositivo
    Device_init();
    Interrupt_initModule();
    Interrupt_initVectorTable();
    Board_init();

    uint16_t i;

for(i = 0; i < TAM_BUFFER_DAC; i++)
{
    dac_buffer[i] = 2048;
}

    // Habilita interrup��es globais e de tempo real
    EINT;
    ERTM;

    while (1)
    {
        if (g_prot_header.cmd != CMD_NONE)
        {
            switch (g_prot_header.cmd)
            {
                case CMD_RECEIVE_INT:
                    g_dado = protocolReceiveInt(SCI0_BASE);
                    break;

                case CMD_SEND_INT:
                    protocolSendInt(SCI0_BASE, g_dado);
                    break;
            }

            // Limpa status de interrup��o e reseta comando
            SCI_clearInterruptStatus(SCI0_BASE, SCI_INT_RXFF);
            g_prot_header.cmd = CMD_NONE;
        }
    }
}

//
// Rotina de Interrup��o da SCI (Recep��o)
//
__interrupt void INT_SCI0_RX_ISR(void)
{
    uint16_t header[PROTOCOL_HEADER_SIZE];
    uint16_t cmd;

    SCI_readCharArray(SCI0_BASE, header, PROTOCOL_HEADER_SIZE);
    cmd = header[0];
    g_prot_header.data_len = header[1] | (header[2] << 8);
    g_prot_header.cmd = (cmd < CMD_COUNT)? (SCI_Command_e)cmd : CMD_NONE;

    Interrupt_clearACKGroup(INT_SCI0_RX_INTERRUPT_ACK_GROUP);
}

__interrupt void INT_myCPUTIMER0_ISR(void)
{
    timer_isr_count++;
    static uint16_t cnt_dac = 0;

    DAC_setShadowValue(
        myDAC0_BASE,
        dac_buffer[cnt_dac]);

    cnt_dac = (cnt_dac + 1) % TAM_BUFFER_DAC;

     ADC_forceSOC(
        myADC0_BASE,
        myADC0_FORCE_SOC0);

    DEVICE_DELAY_US(10);

    adc_teste =
        ADC_readResult(
            myADC0_RESULT_BASE,
            myADC0_SOC0);


    Interrupt_clearACKGroup(
        INT_myCPUTIMER0_INTERRUPT_ACK_GROUP);
}

__interrupt void INT_myADC0_1_ISR(void)
{
    adc_isr_count++;
    static uint16_t cnt_adc = 0;

    adc_buffer[cnt_adc] =
        ADC_readResult(
            myADC0_RESULT_BASE,
            myADC0_SOC0);

    cnt_adc = (cnt_adc + 1) % TAM_BUFFER_ADC;

    ADC_clearInterruptStatus(
        myADC0_BASE,
        ADC_INT_NUMBER1);

    Interrupt_clearACKGroup(
        INT_myADC0_1_INTERRUPT_ACK_GROUP);
}