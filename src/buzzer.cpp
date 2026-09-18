#include "driver/ledc.h"
#include "freertos/task.h"
#include <math.h>

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

//bloquant
void play_tone(uint32_t freq, uint32_t duration_ms, ledc_channel_config_t *ledc_channel, ledc_timer_config_t *ledc_timer) {
  if (freq == 0) {
    // Si la note est un silence (0), on coupe le son (duty = 0)
    ledc_set_duty(ledc_channel->speed_mode, ledc_channel->channel, 0);
    ledc_update_duty(ledc_channel->speed_mode, ledc_channel->channel);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
  } else {
    // 1. On change la fréquence du Timer
    ledc_timer->freq_hz = freq;
    ledc_timer_config(ledc_timer);
    
    // 2. On active le son (50% de volume = DUTY_MAX / 2)
    ledc_set_duty(ledc_channel->speed_mode, ledc_channel->channel, pow(2, (int)(ledc_timer->duty_resolution)) / 2);
    ledc_update_duty(ledc_channel->speed_mode, ledc_channel->channel);
    
    // 3. Temps de jeu de la note
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
  }
  
  // Petite coupure silencieuse indispensable entre chaque note pour bien les détacher
  ledc_set_duty(ledc_channel->speed_mode, ledc_channel->channel, 0);
  ledc_update_duty(ledc_channel->speed_mode, ledc_channel->channel);
  vTaskDelay(pdMS_TO_TICKS(30)); 
}

// OUTRO : Mort de Pac-Man
void play_pacman_outro(ledc_channel_config_t *ledc_channel, ledc_timer_config_t *ledc_timer) {
  int melody[] = { NOTE_C5, NOTE_C4, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_F5, NOTE_F5, NOTE_D5, NOTE_C5 };
  int durations[] = { 130, 130, 130, 130, 130, 130, 130, 180, 350 };

  for (int i = 0; i < 9; i++) {
    play_tone(melody[i], durations[i], ledc_channel, ledc_timer);
  }
}

void play_mario_intro(ledc_channel_config_t *ledc_channel, ledc_timer_config_t *ledc_timer) {
  int melody[] = { NOTE_E5, NOTE_E5, 0, NOTE_E5, 0, NOTE_C5, NOTE_E5, 0, NOTE_G5, 0, 0, NOTE_G4 };
  int durations[] = { 100, 100, 80, 100, 80, 100, 100, 80, 100, 200, 80, 200 };

  for (int i = 0; i < 12; i++) {
    play_tone(melody[i], durations[i], ledc_channel, ledc_timer);
  }
}