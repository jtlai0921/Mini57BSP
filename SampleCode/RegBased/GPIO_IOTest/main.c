/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * $Revision: 1 $
 * $Date: 17/07/19 6:46p $
 * @brief    Sample code for GPIO I/O feature.
 * @note
 * Copyright (C) 2017 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include "Mini57Series.h"


void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable 48MHz HIRC */
    CLK->PWRCTL = CLK->PWRCTL | CLK_PWRCTL_HIRCEN_Msk;

    /* Waiting for 48MHz clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* HCLK Clock source from HIRC */
    CLK->CLKSEL0 = CLK->CLKSEL0 | CLK_HCLK_SRC_HIRC;

    /* Enable USCI0 IP clock */
    CLK->APBCLK = CLK->APBCLK | CLK_APBCLK_USCI0CKEN_Msk;

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock and cyclesPerUs automatically. */
    SystemCoreClockUpdate();

    /* USCI-Uart0-GPD5(TX) + GPD6(RX) */
    /* Set GPD multi-function pins for USCI UART0 GPD5(TX) and GPD6(RX) */
    SYS->GPD_MFP = SYS->GPD_MFP & ~(SYS_GPD_MFP_PD5MFP_Msk | SYS_GPD_MFP_PD6MFP_Msk) | (SYS_GPD_MFP_PD5_UART0_TXD | SYS_GPD_MFP_PD6_UART0_RXD);

    /* Set GPD5 as output mode and GPD6 as Input mode */
    PD->MODE = PD->MODE & ~(GPIO_MODE_MODE5_Msk | GPIO_MODE_MODE6_Msk) | (GPIO_MODE_OUTPUT << GPIO_MODE_MODE5_Pos);

    /* Lock protected registers */
    SYS_LockReg();
}


/**
 * @brief       PortA/PortB/PortC/PortD IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PortA/PortB/PortC/PortD default IRQ, declared in startup_Mini57Series.s.
 */
uint32_t PAIntFlag=0, PBIntFlag=0, PCIntFlag=0, PDIntFlag=0;
void GPABCD_IRQHandler(void)
{
    /* PAIntFlag = GPIO_GET_INT_FLAG(PA, 0xF); */
    PAIntFlag = PA->INTSRC & 0xF;

    /* PBIntFlag = GPIO_GET_INT_FLAG(PB, 0xF); */
    PBIntFlag = PB->INTSRC & 0xF;

    /* PCIntFlag = GPIO_GET_INT_FLAG(PC, 0xF); */
    PCIntFlag = PC->INTSRC & 0xF;

    /* PDIntFlag = GPIO_GET_INT_FLAG(PD, 0xF); */
    PDIntFlag = PD->INTSRC & 0xF;

    /* clear GPIO interrupt flag */
    /* GPIO_CLR_INT_FLAG(PA, PAIntFlag); */
    PA->INTSRC = PAIntFlag;
    
    /* GPIO_CLR_INT_FLAG(PB, PBIntFlag); */
    PB->INTSRC = PBIntFlag;

    /* GPIO_CLR_INT_FLAG(PC, PCIntFlag); */
    PC->INTSRC = PCIntFlag;

    /* GPIO_CLR_INT_FLAG(PD, PDIntFlag); */
    PD->INTSRC = PDIntFlag;

    /*
    printf("GPABCD_IRQHandler() ...\n");
    printf("Interrupt Flag PA=0x%08X, PB=0x%08X, PC=0x%08X, PD=0x%08X\n",
        PAIntFlag, PBIntFlag, PCIntFlag, PDIntFlag);
    */
}


int main()
{
    int32_t i32Err;

    SYS_Init();

    /* Init USCI UART0 to 115200-8n1 for print message */
    UUART_Open(UUART0, 115200);

    /* printf("\n\nPDID 0x%08X\n", SYS_ReadPDID()); */    /* Display PDID */
    printf("\n\nPDID 0x%08X\n", (unsigned int)(SYS->PDID & SYS_PDID_PDID_Msk)); /* Display PDID */

    printf("CPU @ %dHz\n", SystemCoreClock);        /* Display System Core Clock */

    /*
     * This sample code will use GPIO driver to control the GPIO pin direction and
     * the high/low state, and show how to use GPIO interrupts.
     */
    printf("+-----------------------------------------+\n");
    printf("| Mini57 GPIO Driver Sample Code          |\n");
    printf("+-----------------------------------------+\n");

    /*-----------------------------------------------------------------------------------------------------*/
    /* GPIO Basic Mode Test --- Use Pin Data Input/Output to control GPIO pin                              */
    /*-----------------------------------------------------------------------------------------------------*/
    printf("  >> Please connect PB.0 and PC.2 first << \n");
    printf("     Press any key to start test by using [Pin Data Input/Output Control] \n\n");
    getchar();

    /* Config multiple function to GPIO mode for PB0 and PC2 */
    SYS->GPB_MFP = (SYS->GPB_MFP & ~SYS_GPB_MFP_PB0MFP_Msk) | SYS_GPB_MFP_PB0_GPIO;
    SYS->GPC_MFP = (SYS->GPC_MFP & ~SYS_GPC_MFP_PC2MFP_Msk) | SYS_GPC_MFP_PC2_GPIO;

    /* GPIO_SetMode(PB, BIT0, GPIO_PMD_OUTPUT); */
    PB->MODE = (PB->MODE & ~GPIO_MODE_MODE0_Msk) | (GPIO_MODE_OUTPUT << GPIO_MODE_MODE0_Pos);

    /* GPIO_SetMode(PC, BIT2, GPIO_PMD_INPUT); */
    PC->MODE = (PC->MODE & ~GPIO_MODE_MODE2_Msk) | (GPIO_MODE_INPUT << GPIO_MODE_MODE2_Pos);

    /* GPIO_EnableInt(PC, 2, GPIO_INT_RISING); */     /* Enable PC2 interrupt by rising edge trigger */
    PC->INTEN = (PC->INTEN & ~(GPIO_INT_BOTH_EDGE << 2)) | (GPIO_INT_RISING << 2);
    PC->INTTYPE = (PC->INTTYPE & ~(BIT0 << 2)) | (GPIO_INTTYPE_EDGE << 2);

    NVIC_EnableIRQ(GP_IRQn);                    /* Enable GPIO NVIC */

    PAIntFlag = 0;
    PBIntFlag = 0;
    PCIntFlag = 0;
    PDIntFlag = 0;
    i32Err = 0;
    printf("  GPIO Output/Input test ...... \n");

    /* Use Pin Data Input/Output Control to pull specified I/O or get I/O pin status */
    PB0 = 0;                /* Output low */
    CLK_SysTickDelay(10);   /* wait for IO stable */
    if (PC2 != 0) {         /* check if the PB3 state is low */
        i32Err = 1;
    }

    PB0 = 1;                /* Output high */
    CLK_SysTickDelay(10);   /* wait for IO stable */
    if (PC2 != 1) {         /* check if the PB3 state is high */
        i32Err = 1;
    }

    /* show the result */
    if ( i32Err ) {
        printf("  [FAIL] --- Please make sure PB.0 and PC.2 are connected. \n");
    } else {
        printf("  [OK] \n");
    }

    printf("  Check Interrupt Flag PA=0x%08X, PB=0x%08X, PC=0x%08X, PD=0x%08X\n",
        PAIntFlag, PBIntFlag, PCIntFlag, PDIntFlag);

    /* GPIO_SetMode(PB, BIT0, GPIO_PMD_INPUT); */     /* Configure PB.0 to default Input mode */
    PB->MODE = (PB->MODE & ~GPIO_MODE_MODE0_Msk) | (GPIO_MODE_INPUT << GPIO_MODE_MODE0_Pos);

    /* GPIO_SetMode(PC, BIT2, GPIO_PMD_INPUT); */     /* Configure PC.2 to default Input mode */
    PC->MODE = (PC->MODE & ~GPIO_MODE_MODE2_Msk) | (GPIO_MODE_INPUT << GPIO_MODE_MODE2_Pos);

    printf("=== THE END ===\n\n");
    while(1);
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
