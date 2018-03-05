/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * $Revision: 1 $
 * $Date: 17/07/19 6:47p $
 * @brief    Sample code for HDIV calculation feature.
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

    /* Enable IP clock */
    /* CLK_EnableModuleClock(HDIV_MODULE); */
    CLK->AHBCLK |= CLK_AHBCLK_HDIVCKEN_Msk;

    /* Select IP clock source */
}


int main()
{
    int32_t x, y, q, r;
    int i;

    SYS_Init();

    /* Init USCI UART0 to 115200-8n1 for print message */
    UUART_Open(UUART0, 115200);

    /* printf("\n\nPDID 0x%08X\n", SYS_ReadPDID()); */    /* Display PDID */
    printf("\n\nPDID 0x%08X\n", (unsigned int)(SYS->PDID & SYS_PDID_PDID_Msk)); /* Display PDID */

    printf("CPU @ %dHz\n", SystemCoreClock);        /* Display System Core Clock */

    /*
     * This sample code will demonstrate how to divide two signed integer by HDIV engine.
     */
    printf("+-----------------------------------------+\n");
    printf("| Mini57 HDIV Sample Code                 |\n");
    printf("+-----------------------------------------+\n");

    x = 123456;
    y = 21;
    for (i=0; i<5; i++)
    {
        /* q = HDIV_Div(x, y); */
        HDIV->DIVIDEND = x;
        HDIV->DIVISOR = y;      /* HDIV begin to calculate here. */
        q = HDIV->QUOTIENT;

        /* r = HDIV_Mod(x, y); */
        // HDIV->DIVIDEND = x;
        // HDIV->DIVISOR = y;      /* HDIV begin to calculate here. */
        r = HDIV->REM;

        /* if (! HDIV_IS_DIVBYZERO()) */
        if (! ((HDIV->STATUS & HDIV_STATUS_DIVBYZERO_Msk) >> HDIV_STATUS_DIVBYZERO_Pos))
        {
            /* printf("HDIV calculation : %d / %d = %d ... %d, Divide by 0 flag : %d\n", x, y, q, r, HDIV_IS_DIVBYZERO()); */
            printf("HDIV calculation : %d / %d = %d ... %d, Divide by 0 flag : %d\n", x, y, q, r,
                    ((HDIV->STATUS & HDIV_STATUS_DIVBYZERO_Msk) >> HDIV_STATUS_DIVBYZERO_Pos));

            printf("Correct answer   : %d / %d = %d ... %d\n", x, y, x / y, x % y);
            if ((q == x / y) && (r == x % y))
                printf("==> Division PASS !!!\n");
            else
                printf("==> Division FAIL !!!\n");
        }
        else
        {
            printf("HDIV calculation : %d / %d, divide by 0 !!!\n", x, y);
            printf("==> Divide by zero checking PASS !!!\n");
        }

        y -= 7;
        printf("\n");
    }

    /* CLK_DisableModuleClock(HDIV_MODULE); */
    CLK->AHBCLK &= ~CLK_AHBCLK_HDIVCKEN_Msk;
    
    printf("=== THE END ===\n\n");
    while(1);
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
