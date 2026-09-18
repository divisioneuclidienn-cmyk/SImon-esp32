#include <Arduino.h>
#include "driver/ledc.h"
#include "buzzer.h"

#define INPUT_TIMEOUT_MS 15000
#define LED_BLINK_TIME 500

#define MAX_DEBOUNCE 10
#define DEBOUNCE_INTERVAL 5

#define PWM_RESOLUTION LEDC_TIMER_12_BIT
#define DUTY_MAX 4096

//états tous utiles ?
typedef enum STATE{
  STATE_START = 0,
  STATE_GAME_LOGIC = 1,
  STATE_USER = 2,
  STATE_GAME_OVER = 3
}STATE;
STATE state;

const gpio_num_t led_gpio[4] = {
  GPIO_NUM_14,
  GPIO_NUM_27,
  GPIO_NUM_26,
  GPIO_NUM_25
};

const gpio_num_t button_gpio[4] = {
  GPIO_NUM_33,
  GPIO_NUM_32,
  GPIO_NUM_35,
  GPIO_NUM_34
};

//13, 12, 
const gpio_num_t buzzer = GPIO_NUM_13;

//configuration de l'horloge
ledc_timer_config_t ledc_timer = {
  .speed_mode = LEDC_LOW_SPEED_MODE,
  .duty_resolution = PWM_RESOLUTION,
  .timer_num = LEDC_TIMER_0,
  .freq_hz = 523,
  .clk_cfg = LEDC_USE_APB_CLK
};

ledc_channel_config_t ledc_channel = {
  .gpio_num = buzzer,
  .speed_mode = LEDC_LOW_SPEED_MODE,
  .channel = LEDC_CHANNEL_0,
  .timer_sel = LEDC_TIMER_0,
  .duty = 0,
  .hpoint = 0,
};

typedef enum button_event {
  EVENT_NONE = 0,
  EVENT_PRESSED = 1,
  EVENT_RELEASED = 2
}button_event;

typedef struct led_struct {
  gpio_num_t gpio;
  bool state;
}led_t;

//structure bouton comprenant sa propre led
typedef struct button_struct{
  gpio_num_t gpio;
  uint8_t debounce_count;
  bool current_stable_state;
  bool last_stable_state;
  button_event event;
  int buzzer_frequency;
  uint8_t ID;

  led_t led;
}button_t;
//tableau scope global
button_t button[4];

uint8_t sequence[100] = {0*100};
uint8_t game_index = 0;
uint8_t user_index = 0;

uint64_t input_time = 0;

//button AND led
void init_hardware(button_t *button, gpio_num_t button_gpio, gpio_num_t led_gpio, int buzzer_frequency, uint8_t ID){
  button->gpio = button_gpio;
  button->debounce_count = 0;
  button->current_stable_state = false;
  button->last_stable_state = false;
  button->event = EVENT_NONE;
  button->buzzer_frequency = buzzer_frequency;
  button->ID = ID;

  button->led.gpio = led_gpio;
  button->led.state = false;

  //INPUT_PULLUP
  gpio_config_t button_config = {
    .pin_bit_mask = (1ULL << button_gpio),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };

  gpio_config_t led_config = {
  .pin_bit_mask = (1ULL << led_gpio),
  .mode = GPIO_MODE_OUTPUT,
  .pull_up_en = GPIO_PULLUP_DISABLE,
  .pull_down_en = GPIO_PULLDOWN_DISABLE,
  .intr_type = GPIO_INTR_DISABLE
  };

  esp_err_t err1 = gpio_config(&button_config);
  esp_err_t err2 = gpio_config(&led_config);

  printf("err1 : %d | err2 : %d\n", err1, err2);
}

//Acquisition + Filtrage + Interpretation (event)
void read_button(button_t *button){
  bool raw_state = !gpio_get_level(button->gpio);

  //debounce section
  if(raw_state){
    if(button->debounce_count < MAX_DEBOUNCE) { button->debounce_count++; }
  }
  else {
    if(button->debounce_count > 0) { button->debounce_count--; }
  }

  //stable_state update section
  if(button->debounce_count == MAX_DEBOUNCE) { button->current_stable_state = true; }
  else if(button->debounce_count == 0) { button->current_stable_state = false; }

  //event occured
  if(button->current_stable_state != button->last_stable_state){
    //réinitialise last stable state pour permettre la détection des prochains event
    button->last_stable_state = button->current_stable_state;

    //button currently pressed
    if(button->current_stable_state){ button->event = EVENT_PRESSED; }

    //button currently released
    else{ button->event = EVENT_RELEASED; }
  }
}

void dump_button(const button_t *b){
  printf("[DUMP] gpio : %d | debounce_count : %d | current_stable_state : %d | last_stable_state : %d | event : %d | led->gpio : %d | led->state : %d\n",
  b->gpio, 
  b->debounce_count, 
  b->current_stable_state, 
  b->last_stable_state, 
  b->event, 
  b->led.gpio, 
  b->led.state);
}

void play_led(button_t *button){
  gpio_set_level(button->led.gpio, 1);

  //wake up the buzzer
  ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, DUTY_MAX / 2);
  ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
  ledc_timer.freq_hz = button->buzzer_frequency;
  ledc_timer_config(&ledc_timer);

  vTaskDelay(pdMS_TO_TICKS(LED_BLINK_TIME));
  gpio_set_level(button->led.gpio, 0);

  //shut up the buzzer
  ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
  ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);

  vTaskDelay(pdMS_TO_TICKS(LED_BLINK_TIME));
}

void play_sequence(){
  //délai entre led utilisateur et nouvelle séquence
  vTaskDelay(pdMS_TO_TICKS(1000));
  for(int i = 0; i < game_index; i++){
    uint8_t button_id = sequence[i];
    play_led(&button[button_id]);
  }
}

void add_sequence(void){
  sequence[game_index] = esp_random() % 4;
  game_index++;
}

void game_logic(void){
  add_sequence();
  play_sequence();
  state = STATE_USER;
}

//jouer son de fin
//reset variables du jeu
//rentrer en hibernation OU wait(5s puis lacement du jeu)
void game_over(){
  play_pacman_outro(&ledc_channel, &ledc_timer);

  game_index = 0;
  user_index = 0;

  vTaskDelay(pdMS_TO_TICKS(5000));

  state = STATE_START;
}

//state possible : ADD_SEQUENCE
//mise à jour de user_index
//exploit : boutons appuyés en meme temps = vérif de tous les boutons avant GAME_OVER car super loop switch quand fonction finie
void match_sequence(button_t *button){
  if(button->ID == sequence[user_index]){
    user_index++;
  }
  else {
    state = STATE_GAME_OVER;
  }

  if(user_index == game_index) {
    user_index = 0;
    state = STATE_GAME_LOGIC;
  }
}

//APPEL match_sequence
//nom trop mince pour ce que fait la fonction
//context du bouton actuel
void handle_input(button_t *button){
  button_event event = button->event;

  if(event == EVENT_PRESSED){
    printf("[user_logic] : button %d pressed\n", button->ID);
    play_led(button);

    match_sequence(button);
    button->event = EVENT_NONE;
  }
}

//appels des fonctions de logique bouton + gère le timeout utilisateur
void user_logic(){

  //ISSUE : timer esp continu meme si la fonction ne se joue pas donc décalage = mort instantannée
  /*
  if(input_time - (esp_timer_get_time() / 1000) >= INPUT_TIMEOUT_MS){
    input_time = 0;
    state = STATE_GAME_OVER;
  }
  */

  read_button(&button[0]);
  handle_input(&button[0]);

  read_button(&button[1]);
  handle_input(&button[1]);

  read_button(&button[2]);
  handle_input(&button[2]);

  read_button(&button[3]);
  handle_input(&button[3]);
}

void start(void){
  play_mario_intro(&ledc_channel, &ledc_timer);
  state = STATE_GAME_LOGIC;
}

void setup(){
  state = STATE_START;

  init_hardware(&button[0], button_gpio[0], led_gpio[0], NOTE_C5, 0);
  init_hardware(&button[1], button_gpio[1], led_gpio[1], NOTE_D5, 1);
  init_hardware(&button[2], button_gpio[2], led_gpio[2], NOTE_E5, 2);
  init_hardware(&button[3], button_gpio[3], led_gpio[3], NOTE_F5, 3);

  ledc_timer_config(&ledc_timer);
  ledc_channel_config(&ledc_channel);
}

void loop(){
  switch(state){
    case STATE_START:
      //printf("[DEBUG] : start()\n");
      start();
      break;
    
    case STATE_GAME_LOGIC:
      //printf("[DEBUG] : game_logic()\n");
      game_logic();
      break;
    
    case STATE_USER:
      //printf("[DEBUG] : user_logic()\n");
      user_logic();
      break;
    
    case STATE_GAME_OVER:
      //printf("[DEBUG] : game_over()\n");
      game_over();
      break;
  }

  vTaskDelay(pdMS_TO_TICKS(1));
}


//réecrire avec du non bloquant