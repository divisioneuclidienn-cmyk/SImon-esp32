#include "driver/ledc.h"
#include "freertos/task.h"
#include <math.h>

#ifndef BUZZER_H
#define BUZZER_H

#define NOTE_C4  262
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E4  330
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G4  392
#define NOTE_G5  784
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_B5  988

void play_tone(uint32_t freq, uint32_t duration_ms, ledc_channel_config_t *ledc_channel, ledc_timer_config_t *ledc_timer);
void play_pacman_outro(ledc_channel_config_t *ledc_channel, ledc_timer_config_t *ledc_timer);
void play_mario_intro(ledc_channel_config_t *ledc_channel, ledc_timer_config_t *ledc_timer);

#endif

//test