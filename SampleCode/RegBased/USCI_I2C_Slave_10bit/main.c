/******************************************************************************
 * @file     main.c
 * @version  V3.00
 * $Revision: 1 $
 * $Date: 17/08/29 8:16p $
 * @brief    Show how to set USCI_I2C in Slave 10-bit address mode and receive the data from Master.
 *           This sample code needs to work with USCI_I2C_Master_10bit.
 * @note
 * Copyright (C) 2017 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include "Mini57Series.h"

#define SLV_10BIT_ADDR (0x1E<<2)             //1111+0xx+r/w

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
volatile uint8_t g_au8SlvData[256];
volatile uint32_t slave_buff_addr;
volatile uint8_t g_au8SlvRxData[4];
volatile uint16_t g_u16SlvRcvAddr;
volatile uint8_t g_u8SlvDataLen;

volatile enum UI2C_SLAVE_EVENT s_Event;

typedef void (*UI2C_FUNC)(uint32_t u32Status);

volatile static UI2C_FUNC s_UI2C1HandlerFn = NULL;

/*---------------------------------------------------------------------------------------------------------*/
/*  USCI_I2C1 IRQ Handler                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
void USCI1_IRQHandler(void)
{
    uint32_t u32Status;

    u32Status = UI2C_GET_PROT_STATUS(UI2C1);

    if (UI2C_GET_TIMEOUT_FLAG(UI2C1))
    {
        /* Clear USCI_I2C1 Timeout Flag */
        UI2C1->PROTSTS = UI2C_PROTSTS_TOIF_Msk;
    }
    else
    {
        if (s_UI2C1HandlerFn != NULL)
            s_UI2C1HandlerFn(u32Status);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  USCI_I2C TRx Callback Function                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void USCI_I2C_SlaveTRx(uint32_t u32Status)
{
    uint32_t temp;

    if((u32Status & UI2C_PROTSTS_STARIF_Msk) == UI2C_PROTSTS_STARIF_Msk)
    {
        /* Clear START INT Flag */
        UI2C_CLR_PROT_INT_FLAG(UI2C1, UI2C_PROTSTS_STARIF_Msk);

        /* Event process */
        g_u8SlvDataLen = 0;
        s_Event = SLAVE_H_RD_ADDRESS_ACK;

        UI2C_SET_CONTROL_REG(UI2C1, (UI2C_CTL_PTRG | UI2C_CTL_AA));
    }
    else if((u32Status & UI2C_PROTSTS_ACKIF_Msk) == UI2C_PROTSTS_ACKIF_Msk)
    {
        /* Clear ACK INT Flag */
        UI2C_CLR_PROT_INT_FLAG(UI2C1, UI2C_PROTSTS_ACKIF_Msk);

        /* Event process */
        if(s_Event == SLAVE_H_WR_ADDRESS_ACK)
        {
            g_u8SlvDataLen = 0;

            s_Event = SLAVE_L_WR_ADDRESS_ACK;
            g_u16SlvRcvAddr = (uint8_t)UI2C_GET_DATA(UI2C1);
        }
        else if(s_Event == SLAVE_H_RD_ADDRESS_ACK)
        {
            g_u8SlvDataLen = 0;

            UI2C_SET_DATA(UI2C1, g_au8SlvData[slave_buff_addr]);
            slave_buff_addr++;
            g_u16SlvRcvAddr = (uint8_t)UI2C_GET_DATA(UI2C1);
        }
        else if(s_Event == SLAVE_L_WR_ADDRESS_ACK)
        {
            if((UI2C1->PROTSTS & UI2C_PROTSTS_SLAREAD_Msk) == UI2C_PROTSTS_SLAREAD_Msk)
            {
                UI2C_SET_DATA(UI2C1, g_au8SlvData[slave_buff_addr]);
                slave_buff_addr++;
            }
            else
            {
                s_Event = SLAVE_GET_DATA;
            }
            g_u16SlvRcvAddr = (uint8_t)UI2C_GET_DATA(UI2C1);
        }
        else if(s_Event == SLAVE_L_RD_ADDRESS_ACK)
        {
            UI2C_SET_DATA(UI2C1, g_au8SlvData[slave_buff_addr]);
            slave_buff_addr++;
        }
        else if(s_Event == SLAVE_GET_DATA)
        {
            temp = (uint8_t)UI2C_GET_DATA(UI2C1);
            g_au8SlvRxData[g_u8SlvDataLen] = temp;
            g_u8SlvDataLen++;

            if(g_u8SlvDataLen == 2)
            {
                temp = (g_au8SlvRxData[0] << 8);
                temp += g_au8SlvRxData[1];
                slave_buff_addr = temp;
            }
            if(g_u8SlvDataLen == 3)
            {
                temp = g_au8SlvRxData[2];
                g_au8SlvData[slave_buff_addr] = temp;
                g_u8SlvDataLen = 0;
            }
        }

        UI2C_SET_CONTROL_REG(UI2C1, (UI2C_CTL_PTRG | UI2C_CTL_AA));
    }
    else if((u32Status & UI2C_PROTSTS_NACKIF_Msk) == UI2C_PROTSTS_NACKIF_Msk)
    {
        /* Clear NACK INT Flag */
        UI2C_CLR_PROT_INT_FLAG(UI2C1, UI2C_PROTSTS_NACKIF_Msk);

        /* Event process */
        g_u8SlvDataLen = 0;
        s_Event = SLAVE_H_WR_ADDRESS_ACK;

        UI2C_SET_CONTROL_REG(UI2C1, (UI2C_CTL_PTRG | UI2C_CTL_AA));
    }
    else if((u32Status & UI2C_PROTSTS_STORIF_Msk) == UI2C_PROTSTS_STORIF_Msk)
    {
        /* Clear STOP INT Flag */
        UI2C_CLR_PROT_INT_FLAG(UI2C1, UI2C_PROTSTS_STORIF_Msk);

        /* Event process */
        g_u8SlvDataLen = 0;
        s_Event = SLAVE_H_WR_ADDRESS_ACK;

        UI2C_SET_CONTROL_REG(UI2C1, (UI2C_CTL_PTRG | UI2C_CTL_AA));
    }
}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable 48MHz HIRC */
    CLK->PWRCTL = CLK->PWRCTL | CLK_PWRCTL_HIRCEN_Msk;

    /* Waiting for 48MHz clock ready */
    while((CLK->STATUS & CLK_STATUS_HIRCSTB_Msk) != CLK_STATUS_HIRCSTB_Msk);

    /* HCLK Clock source from HIRC */
    CLK->CLKSEL0 = CLK->CLKSEL0 | CLK_HCLK_SRC_HIRC;

    /* Enable IP clock */
    CLK->APBCLK = CLK->APBCLK | (CLK_APBCLK_USCI0CKEN_Msk | CLK_APBCLK_USCI1CKEN_Msk);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock and cyclesPerUs automatically. */
    SystemCoreClockUpdate();

    /* USCI-Uart0-GPD5(TX) + GPD6(RX) */
    /* Set GPD multi-function pins for USCI UART0 GPD5(TX) and GPD6(RX) */
    SYS->GPD_MFP = (SYS->GPD_MFP & ~(SYS_GPD_MFP_PD5MFP_Msk | SYS_GPD_MFP_PD6MFP_Msk)) | (SYS_GPD_MFP_PD5_UART0_TXD | SYS_GPD_MFP_PD6_UART0_RXD);

    /* Set GPD5 as output mode and GPD6 as Input mode */
    PD->MODE = (PD->MODE & ~(GPIO_MODE_MODE5_Msk | GPIO_MODE_MODE6_Msk)) | (GPIO_MODE_OUTPUT << GPIO_MODE_MODE5_Pos);

    /* Set GPC multi-function pins for USCI I2C1 GPC0(SCL) and GPC2(SDA) */
    SYS->GPC_MFP = (SYS->GPC_MFP & ~(SYS_GPC_MFP_PC0MFP_Msk | SYS_GPC_MFP_PC2MFP_Msk)) | (SYS_GPC_MFP_PC0_I2C1_SCL | SYS_GPC_MFP_PC2_I2C1_SDA);

    /* Lock protected registers */
    SYS_LockReg();
}

void UUART0_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init USCI                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset USCI0 */
    SYS->IPRST1 |= SYS_IPRST1_USCI0RST_Msk;
    SYS->IPRST1 &= ~SYS_IPRST1_USCI0RST_Msk;

    /* Configure USCI0 as UART mode */
    UUART0->CTL = (2 << UUART_CTL_FUNMODE_Pos);                                 /* Set UART function mode */
    UUART0->LINECTL = UUART_WORD_LEN_8 | UUART_LINECTL_LSB_Msk;                 /* Set UART line configuration */
    UUART0->DATIN0 = (2 << UUART_DATIN0_EDGEDET_Pos);                           /* Set falling edge detection */
    UUART0->BRGEN = (51 << UUART_BRGEN_CLKDIV_Pos) | (7 << UUART_BRGEN_DSCNT_Pos); /* Set UART baud rate as 115200bps */
    UUART0->PROTCTL |= UUART_PROTCTL_PROTEN_Msk;                                /* Enable UART protocol */
}

void UI2C1_Init(uint32_t u32ClkSpeed)
{
    uint32_t u32ClkDiv;
    uint32_t u32Pclk = SystemCoreClock;

    /* The USCI usage is exclusive */
    /* If user configure the USCI port as UI2C function, that port cannot use UUART or USPI function. */
    /* Open USCI_I2C1 and set clock to 100k */
    u32ClkDiv = (uint32_t) ((((((u32Pclk/2)*10)/(u32ClkSpeed))+5)/10)-1); /* Compute proper divider for USCI_I2C clock */

    /* Enable USCI_I2C protocol */
    UI2C1->CTL &= ~UI2C_CTL_FUNMODE_Msk;
    UI2C1->CTL = 4 << UI2C_CTL_FUNMODE_Pos;

    /* Data format configuration */
    /* 8 bit data length */
    UI2C1->LINECTL &= ~UI2C_LINECTL_DWIDTH_Msk;
    UI2C1->LINECTL |= 8 << UI2C_LINECTL_DWIDTH_Pos;

    /* MSB data format */
    UI2C1->LINECTL &= ~UI2C_LINECTL_LSB_Msk;

    /* Set USCI_I2C bus clock */
    UI2C1->BRGEN &= ~UI2C_BRGEN_CLKDIV_Msk;
    UI2C1->BRGEN |=  (u32ClkDiv << UI2C_BRGEN_CLKDIV_Pos);
    UI2C1->PROTCTL |=  UI2C_PROTCTL_PROTEN_Msk;

    /* Enable USCI_I2C1 10-bit address mode */
    UI2C1->PROTCTL |= UI2C_PROTCTL_ADDR10EN_Msk;

    /* Set USCI_I2C1 Slave Addresses */
    UI2C1->DEVADDR0  = 0x116;   /* Slave Address : 0x116 */

    /* Set USCI_I2C1 Slave Addresses Mask */
    UI2C1->ADDRMSK0  = 0x4;      /* Slave Address : 0x4 */

    UI2C_ENABLE_PROT_INT(UI2C1, (UI2C_PROTIEN_ACKIEN_Msk | UI2C_PROTIEN_NACKIEN_Msk | UI2C_PROTIEN_STORIEN_Msk | UI2C_PROTIEN_STARIEN_Msk));
    NVIC_EnableIRQ(USCI1_IRQn);
}

int main()
{
    uint32_t i;
    SYS_Init();

    /* Init USCI UART0 to 115200-8n1 for print message */
    UUART0_Init();

    /*
        This sample code sets USCI_I2C bus clock to 100kHz. Then, Master accesses Slave with Byte Write
        and Byte Read operations, and check if the read data is equal to the programmed data.
    */

    printf("+-------------------------------------------------------+\n");
    printf("|          USCI_I2C Driver Sample Code(Slave)           |\n");
    printf("+-------------------------------------------------------+\n");

    /* Init USCI_I2C1 */
    UI2C1_Init(100000);

    /* USCI_I2C1 enter no address SLV mode */
    s_Event = SLAVE_H_WR_ADDRESS_ACK;
    UI2C_SET_CONTROL_REG(UI2C1, UI2C_CTL_AA);

    for (i = 0; i < 0x100; i++)
    {
        g_au8SlvData[i] = 0;
    }

    /* USCI_I2C1 function to Slave receive/transmit data */
    s_UI2C1HandlerFn = USCI_I2C_SlaveTRx;

    printf("\n");
    printf("USCI_I2C1 Slave Mode is Running.\n");

    while(1);
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
