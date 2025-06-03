/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for TCPWM QuadDec direction detection 
*              Example for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
 * Include Header files
 ********************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"

/*******************************************************************************
 * Macros
 ********************************************************************************/
#define DELAY               (500u)      /* 500 msec */
                                        /* Delay should be more than the period
                                        of input signal to the quadrature
                                        decoder */
#define ROTATION            1           /* CLOCKWISE - 1, ANTI_CLOCKWISE - 2,
                                        NO_ROTATION - 0 */
#define TCPWM_PWM_VAL       (999UL)     /* Counter value*/

#if defined (CY_IP_M7CPUSS)
#if defined (CY_DEVICE_TVIIC2D6M)
#define TRIGMUX_SW          TRIG_OUT_MUX_4_TCPWM0_ALL_CNT_TR_IN0
#define TCPWM_GRPTR         TCPWM_TR_ONE_CNT_NR
#else
#define TRIGMUX_SW          TRIG_OUT_MUX_5_TCPWM1_ALL_CNT_TR_IN0
#define TCPWM_GRPTR         TCPWM1_TR_ONE_CNT_NR
#endif
#else
#define TRIGMUX_SW          TRIG_OUT_MUX_4_TCPWM_ALL_CNT_TR_IN0
#define TCPWM_GRPTR         TCPWM_TR_ONE_CNT_NR
#endif

/*******************************************************************************
 * Function Name: main
 ********************************************************************************
 * Summary:
 *  System entrance point. This function performs
 *    1.Initializes the BSP.
 *    2.To detect the direction of rotation, quadrature decoder counter is monitored.
 *    3.The result is shown using LED.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 *******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize with config set in peripheral and enable QuadDec */
    Cy_TCPWM_QuadDec_Init(QuadDec_HW, QuadDec_NUM, &QuadDec_config);
    Cy_TCPWM_QuadDec_Enable(QuadDec_HW, QuadDec_NUM);

    /* Start QuadDec */
    Cy_TCPWM_TriggerReloadOrIndex_Single(QuadDec_HW, QuadDec_NUM);

    /* Change the TCPWM configuration to use group trigger #0 as its starting trigger.
     * To use a group trigger, 'startInput' requires value calculated with this formula:
     * 2 (for fixed value 0/1) + TCPWM1_TR_ONE_CNT_NR + groupTriggerNo
     */
    PWM_phiA_config.startInputMode = CY_TCPWM_INPUT_RISINGEDGE;
    PWM_phiB_config.startInputMode = CY_TCPWM_INPUT_RISINGEDGE;
    PWM_phiA_config.startInput = 2 + TCPWM_GRPTR;
    PWM_phiB_config.startInput = 2 + TCPWM_GRPTR;

    /* Configure two PWM for generating quadrature encoded signals*/
    /* Initialize with config set in peripheral and enable both the PWMs*/
    Cy_TCPWM_PWM_Init(PWM_phiA_HW, PWM_phiA_NUM, &PWM_phiA_config);
    Cy_TCPWM_PWM_Init(PWM_phiB_HW, PWM_phiB_NUM, &PWM_phiB_config);
    Cy_TCPWM_PWM_Enable(PWM_phiA_HW, PWM_phiA_NUM);
    Cy_TCPWM_PWM_Enable(PWM_phiB_HW, PWM_phiB_NUM);

    /* Start both the PWM Peripherals */
    /* Both PWM peripherals will be triggered simultaneously. Counter value
     * will emulate 90 degrees phase shift.
     */

#if (ROTATION == 1)
    /* Set the counter value to generate phase shift */
    Cy_TCPWM_PWM_SetCounter(PWM_phiA_HW,PWM_phiA_NUM,TCPWM_PWM_VAL);
#elif (ROTATION == 2)
    /* Set the counter value to generate phase shift */
    Cy_TCPWM_PWM_SetCounter(PWM_phiB_HW,PWM_phiB_NUM,TCPWM_PWM_VAL);
#endif

    /* Trigger a software start on the TCPWM instance. This is required when
     * no other hardware input signal is connected to the peripheral to act as
     * a trigger source.
     */
    Cy_TrigMux_SwTrigger(TRIGMUX_SW, CY_TRIGGER_TWO_CYCLES);

    /* Initial count value */
    uint16_t count      = Cy_TCPWM_QuadDec_GetCounter(QuadDec_HW, QuadDec_NUM);

    /* Wait */
    Cy_SysLib_Delay(DELAY);
    /* Update count_prev value */
    uint16_t count_prev = count;

    for(;;)
    {
        count = Cy_TCPWM_QuadDec_GetCounter(QuadDec_HW, QuadDec_NUM);
        /* For clockwise rotation count value increases and for anti-clockwise
         * counter value decreases.
         */
        /* Condition for clockwise rotation */
        if(count > count_prev)
        {
            Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_NUM, CYBSP_LED_STATE_ON);
            Cy_GPIO_Write(CYBSP_USER_LED2_PORT, CYBSP_USER_LED2_NUM, CYBSP_LED_STATE_OFF);
        }
        /* Condition for anti-clockwise rotation */
        else if(count < count_prev)
        {
            Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_NUM, CYBSP_LED_STATE_OFF);
            Cy_GPIO_Write(CYBSP_USER_LED2_PORT, CYBSP_USER_LED2_NUM, CYBSP_LED_STATE_ON);
        }
        /* No rotation */
        else
        {
            Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_NUM, CYBSP_LED_STATE_OFF);
            Cy_GPIO_Write(CYBSP_USER_LED2_PORT, CYBSP_USER_LED2_NUM, CYBSP_LED_STATE_OFF);
        }

        /* Wait */
        Cy_SysLib_Delay(DELAY);
        /* Update count_prev value */
        count_prev = count;
    }
}
/* [] END OF FILE */
