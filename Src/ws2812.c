/*
 * ws2812.c
 *
 *  Created on: Feb 26, 2026
 *      Author: saiikishen
 */


#include "ws2812.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265f

static uint16_t pwmData[(24 * MAX_LED) + 50];

static uint8_t  LED_Data[MAX_LED][4];
static uint8_t  LED_Mod[MAX_LED][4];

static volatile int datasentflag = 0;

static TIM_HandleTypeDef *_htim;
static uint32_t _channel;


void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        HAL_TIM_PWM_Stop_DMA(htim, _channel);
        datasentflag = 1;
    }
}


void WS2812_Init(TIM_HandleTypeDef *htim, uint32_t channel)
{
    _htim    = htim;
    _channel = channel;

    memset(LED_Data, 0, sizeof(LED_Data));
    memset(LED_Mod,  0, sizeof(LED_Mod));
    memset(pwmData,  0, sizeof(pwmData));


    for (int i = 0; i < MAX_LED; i++)
        LED_Data[i][0] = i;

    HAL_Delay(100);

    for (int i = 0; i < 3; i++)
    {
        WS2812_Clear();
        HAL_Delay(10);
    }
}



void WS2812_SetLED(uint8_t led, uint8_t red, uint8_t green, uint8_t blue)
{
    if (led >= MAX_LED) return;
    LED_Data[led][0] = led;
    LED_Data[led][1] = green;
    LED_Data[led][2] = red;
    LED_Data[led][3] = blue;
}


void WS2812_SetBrightness(uint8_t brightness)
{
if (brightness > 45) brightness = 45;
for (int i = 0; i < MAX_LED; i++) {
LED_Mod[i][0] = LED_Data[i][0];
if (brightness == 0) {
LED_Mod[i][1] = LED_Mod[i][2] = LED_Mod[i][3] = 0;
} else {
for (int j = 1; j < 4; j++) {

uint16_t scaled = ((uint16_t)LED_Data[i][j] * brightness) / 45;
LED_Mod[i][j] = (uint8_t)scaled;
}
}
}
}

void WS2812_Send(void)
{
    uint32_t indx = 0;

    memset(pwmData, 0, sizeof(pwmData));

    for (int i = 0; i < MAX_LED; i++) {
#if USE_BRIGHTNESS
        uint32_t color = ((uint32_t)LED_Mod[i][1] << 16)
                       | ((uint32_t)LED_Mod[i][2] << 8)
                       |  (uint32_t)LED_Mod[i][3];
#else
        uint32_t color = ((uint32_t)LED_Data[i][1] << 16)
                       | ((uint32_t)LED_Data[i][2] << 8)
                       |  (uint32_t)LED_Data[i][3];
#endif
        for (int bit = 23; bit >= 0; bit--)
            pwmData[indx++] = (color & (1u << bit)) ? 140 : 70;
    }

    for (int i = 0; i < 50; i++)
        pwmData[indx++] = 0;

    datasentflag = 0;


    _htim->hdma[TIM_DMA_ID_CC1]->State = HAL_DMA_STATE_READY;


    if (HAL_TIM_PWM_Start_DMA(_htim, _channel, (uint32_t *)pwmData, indx) != HAL_OK)
        return; 


    uint32_t timeout = HAL_GetTick();
    while (!datasentflag)
    {
        if ((HAL_GetTick() - timeout) > 10)
        {
            HAL_TIM_PWM_Stop_DMA(_htim, _channel);
            break;
        }
    }

    HAL_Delay(1); 
}


void WS2812_Pattern_SolidColor(uint8_t r, uint8_t g, uint8_t b)
{
	WS2812_Clear();
    for (int i = 0; i < MAX_LED; i++)
        WS2812_SetLED(i, r, g, b);
    WS2812_SetBrightness(45);
    WS2812_Send();
}


static void HSV_to_RGB(uint16_t h, uint8_t *r, uint8_t *g, uint8_t *b)
{
    h = h % 360;
    uint8_t region = h / 60;
    uint8_t rem    = (h % 60) * 255 / 60;
    switch (region) {
        case 0: *r = 255;       *g = rem;       *b = 0;         break;
        case 1: *r = 255 - rem; *g = 255;       *b = 0;         break;
        case 2: *r = 0;         *g = 255;       *b = rem;       break;
        case 3: *r = 0;         *g = 255 - rem; *b = 255;       break;
        case 4: *r = rem;       *g = 0;         *b = 255;       break;
        default:*r = 255;       *g = 0;         *b = 255 - rem; break;
    }
}
void WS2812_Pattern_Rainbow(void)
{
    uint8_t r, g, b;
    for (int i = 0; i < MAX_LED; i++) {
        HSV_to_RGB((i * 360) / MAX_LED, &r, &g, &b);
        WS2812_SetLED(i, r, g, b);
    }
    WS2812_SetBrightness(45); 
    WS2812_Send();
    WS2812_Clear();
}

void WS2812_Pattern_Breathe(uint8_t r, uint8_t g, uint8_t b)
{

    for (int i = 0; i < MAX_LED; i++)
        WS2812_SetLED(i, r, g, b);

  
    for (int br = 0; br <= 100; br += 2) {
        WS2812_SetBrightness(br);
        WS2812_Send();
        HAL_Delay(20);
    }

    for (int br = 100; br >= 0; br -= 2) {
        WS2812_SetBrightness(br);
        WS2812_Send();
        HAL_Delay(20);
        WS2812_Clear();
    }
}

void WS2812_Pattern_Blink(uint8_t r, uint8_t g, uint8_t b,
                           uint8_t times, uint16_t on_ms, uint16_t off_ms)
{

    for (uint8_t i = 0; i < times; i++) {
        for (int led = 0; led < MAX_LED; led++)
            WS2812_SetLED(led, r, g, b);
        WS2812_SetBrightness(45);
        WS2812_Send();
        HAL_Delay(on_ms);

        for (int led = 0; led < MAX_LED; led++)
            WS2812_SetLED(led, 0, 0, 0);
        WS2812_SetBrightness(45);
        WS2812_Send();
        HAL_Delay(off_ms);
        WS2812_Clear();
    }
}

void WS2812_Pattern_Chase(uint8_t r, uint8_t g, uint8_t b, uint16_t delay_ms)
{

    for (int pos = 0; pos < MAX_LED; pos++)
    {
        for (int i = 0; i < MAX_LED; i++)
            WS2812_SetLED(i, 0, 0, 0);

        WS2812_SetLED(pos, r, g, b);
        WS2812_SetBrightness(45);
        WS2812_Send();
        HAL_Delay(delay_ms);
    }


    for (int i = 0; i < MAX_LED; i++)
        WS2812_SetLED(i, 0, 0, 0);
    WS2812_SetBrightness(255);
    WS2812_Send();
    WS2812_Clear();
}

void WS2812_SetZone(uint8_t start, uint8_t end, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = start; i <= end; i++)
        WS2812_SetLED(i, r, g, b);
}

void WS2812_Pattern_DualSolid(uint8_t r1, uint8_t g1, uint8_t b1,
                                uint8_t r2, uint8_t g2, uint8_t b2)
{
    WS2812_SetZone(LED_ZONE_A_START, LED_ZONE_A_END, r1, g1, b1);
    WS2812_SetZone(LED_ZONE_B_START, LED_ZONE_B_END, r2, g2, b2);
    WS2812_SetBrightness(45);
    WS2812_Send();
    WS2812_Clear();
}


void WS2812_Pattern_ChaseZoneA_SolidZoneB(uint8_t r1, uint8_t g1, uint8_t b1,uint8_t r2, uint8_t g2, uint8_t b2,uint16_t delay_ms)
{
    for (int pos = LED_ZONE_A_START; pos <= LED_ZONE_A_END; pos++)
    {
        // Clear zone A
        WS2812_SetZone(LED_ZONE_A_START, LED_ZONE_A_END, 0, 0, 0);
        // Set chase position in zone A
        WS2812_SetLED(pos, r1, g1, b1);
        // Zone B stays solid
        WS2812_SetZone(LED_ZONE_B_START, LED_ZONE_B_END, r2, g2, b2);

        WS2812_SetBrightness(45);
        WS2812_Send();
        HAL_Delay(delay_ms);
    }
    WS2812_Clear();
}

// Zone A blinks, Zone B stays solid
void WS2812_Pattern_BlinkZoneA_SolidZoneB(uint8_t r1, uint8_t g1, uint8_t b1,uint8_t r2, uint8_t g2, uint8_t b2,uint8_t times, uint16_t on_ms, uint16_t off_ms)
{
    for (uint8_t t = 0; t < times; t++)
    {
        WS2812_SetZone(LED_ZONE_A_START, LED_ZONE_A_END, r1, g1, b1);
        WS2812_SetZone(LED_ZONE_B_START, LED_ZONE_B_END, r2, g2, b2);
        WS2812_SetBrightness(45);
        WS2812_Send();
        HAL_Delay(on_ms);

        WS2812_SetZone(LED_ZONE_A_START, LED_ZONE_A_END, 0, 0, 0);
        WS2812_SetZone(LED_ZONE_B_START, LED_ZONE_B_END, r2, g2, b2);
        WS2812_SetBrightness(45);
        WS2812_Send();
        HAL_Delay(off_ms);
    }
    WS2812_Clear();
}

// Both zones chase independently in opposite directions
void WS2812_Pattern_DualChase(uint8_t r1, uint8_t g1, uint8_t b1,uint8_t r2, uint8_t g2, uint8_t b2,uint16_t delay_ms)
{
    int zone_a_size = LED_ZONE_A_END - LED_ZONE_A_START + 1; // 3
    int zone_b_size = LED_ZONE_B_END - LED_ZONE_B_START + 1; // 3
    int steps = (zone_a_size > zone_b_size) ? zone_a_size : zone_b_size;

    for (int step = 0; step < steps; step++)
    {
        for (int i = 0; i < MAX_LED; i++)
            WS2812_SetLED(i, 0, 0, 0);

        // Zone A: chase forward (0→1→2)
        int posA = LED_ZONE_A_START + (step % zone_a_size);
        WS2812_SetLED(posA, r1, g1, b1);

        // Zone B: chase backward (5→4→3)
        int posB = LED_ZONE_B_END - (step % zone_b_size);
        WS2812_SetLED(posB, r2, g2, b2);

        WS2812_SetBrightness(45);
        WS2812_Send();
        HAL_Delay(delay_ms);
    }
    WS2812_Clear();
}



void WS2812_Clear(void)
{

    memset(LED_Data, 0, sizeof(LED_Data));
    memset(LED_Mod,  0, sizeof(LED_Mod));

    for (int i = 0; i < MAX_LED; i++)
        LED_Data[i][0] = i;

    uint32_t indx = 0;
    memset(pwmData, 0, sizeof(pwmData));
    for (int i = 0; i < (24 * MAX_LED); i++)
        pwmData[indx++] = 0;
    for (int i = 0; i < 50; i++)
        pwmData[indx++] = 0;

    datasentflag = 0;
    _htim->hdma[TIM_DMA_ID_CC1]->State = HAL_DMA_STATE_READY;
    HAL_TIM_PWM_Start_DMA(_htim, _channel, (uint32_t *)pwmData, indx);

    uint32_t timeout = HAL_GetTick();
    while (!datasentflag)
    {
        if ((HAL_GetTick() - timeout) > 10)
        {
            HAL_TIM_PWM_Stop_DMA(_htim, _channel);
            break;
        }
    }
    HAL_Delay(1);
}

void WS2812_Test_Individual(void)
{
    // Light each LED one at a time with a 1 second hold
    // Watch which LEDs fail or show wrong color
    for (int target = 0; target < MAX_LED; target++)
    {
        memset(LED_Data, 0, sizeof(LED_Data));
        memset(LED_Mod,  0, sizeof(LED_Mod));
        for (int i = 0; i < MAX_LED; i++)
            LED_Data[i][0] = i;

        LED_Mod[target][0] = target;
        LED_Mod[target][1] = 0;    // G
        LED_Mod[target][2] = 255;  // R  
        LED_Mod[target][3] = 0;    // B

        // Build and send manually
        uint32_t indx = 0;
        memset(pwmData, 0, sizeof(pwmData));
        for (int i = 0; i < MAX_LED; i++) {
            uint32_t color = ((uint32_t)LED_Mod[i][1] << 16)
                           | ((uint32_t)LED_Mod[i][2] << 8)
                           |  (uint32_t)LED_Mod[i][3];
            for (int bit = 23; bit >= 0; bit--)
                pwmData[indx++] = (color & (1u << bit)) ? 140 : 70;
        }
        for (int i = 0; i < 50; i++) pwmData[indx++] = 0;

        datasentflag = 0;
        _htim->hdma[TIM_DMA_ID_CC1]->State = HAL_DMA_STATE_READY;
        HAL_TIM_PWM_Start_DMA(_htim, _channel, (uint32_t *)pwmData, indx);
        uint32_t t = HAL_GetTick();
        while (!datasentflag)
            if ((HAL_GetTick() - t) > 10) { HAL_TIM_PWM_Stop_DMA(_htim, _channel); break; }

        HAL_Delay(1000); 
    }
}



