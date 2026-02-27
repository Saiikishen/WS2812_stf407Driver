/*
 * ws2812.h
 *
 *  Created on: Feb 26, 2026
 *      Author: saiikishen
 */

#ifndef WS2812_H
#define WS2812_H

#include "stm32f4xx_hal.h"

#define MAX_LED       6
#define USE_BRIGHTNESS 1
#define LED_ZONE_A_START  0
#define LED_ZONE_A_END    2
#define LED_ZONE_B_START  3
#define LED_ZONE_B_END    5


void WS2812_Init(TIM_HandleTypeDef *htim, uint32_t channel);
void WS2812_SetLED(uint8_t led, uint8_t red, uint8_t green, uint8_t blue);
void WS2812_SetBrightness(uint8_t brightness);
void WS2812_Send(void);
void WS2812_Clear(void);

// Pattern functions
void WS2812_Pattern_SolidColor(uint8_t r, uint8_t g, uint8_t b);
void WS2812_Pattern_Rainbow(void);
void WS2812_Pattern_Chase(uint8_t r, uint8_t g, uint8_t b, uint16_t delay_ms);
void WS2812_Pattern_Breathe(uint8_t r, uint8_t g, uint8_t b);
void WS2812_Pattern_Blink(uint8_t r, uint8_t g, uint8_t b, uint8_t times, uint16_t on_ms, uint16_t off_ms);
void WS2812_SetZone(uint8_t start, uint8_t end, uint8_t r, uint8_t g, uint8_t b);
void WS2812_Pattern_DualSolid(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2);
void WS2812_Pattern_ChaseZoneA_SolidZoneB(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2, uint16_t delay_ms);
void WS2812_Pattern_BlinkZoneA_SolidZoneB(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2, uint8_t times, uint16_t on_ms, uint16_t off_ms);
void WS2812_Pattern_DualChase(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2, uint16_t delay_ms);


#endif
