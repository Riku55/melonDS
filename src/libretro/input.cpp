#include "input.h"
#include "libretro_state.h"
#include "utils.h"

#include "NDS.h"

InputState input_state;
u32 input_mask = 0xFFF;
static bool has_touched = false;

// 0 = not pressed, 1 = swipe right, 2 = swipe left
static int gesture_button = 0;
static int swipe_position_x = 130;
bool swipe_right_btn_released = true;
bool swipe_left_btn_released = true;

#define ADD_KEY_TO_MASK(key, i) if (!!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, key)) input_mask &= ~(1 << i); else input_mask |= (1 << i);

bool cursor_enabled(InputState *state)
{
   return state->current_touch_mode == TouchMode::Mouse || state->current_touch_mode == TouchMode::Joystick;
}

void update_input(InputState *state)
{
   input_poll_cb();

   ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_A,      0);
   ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_B,      1);
   ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_SELECT, 2);
   ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_START,  3);
   ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_RIGHT,  4);
   ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_LEFT,   5);
   ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_UP,     6);
   ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_DOWN,   7);
   ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_R,      8);
   ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_L,      9);
   ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_X,      10);
   ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_Y,      11);

   NDS::SetKeyMask(input_mask);

   //bool lid_closed_btn = !!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3);
   bool swipe_left_btn = !!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3);
   bool swipe_right_btn = !!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2);

   if (!swipe_right_btn)
   {
      swipe_right_btn_released = true;
   }

   if (!swipe_left_btn)
   {
      swipe_left_btn_released = true;
   }

   if(swipe_right_btn && gesture_button == 0 && swipe_right_btn_released == true)
   {
      gesture_button = 1;
      swipe_position_x = 130;
      swipe_right_btn_released = false;
   }

   if(swipe_left_btn && gesture_button == 0 && swipe_left_btn_released == true)
   {
      gesture_button = 2;
      swipe_position_x = 205;
      swipe_left_btn_released = false;
   }

   //state->holding_noise_btn = !!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2);
   state->swap_screens_btn = !!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);

   if(current_screen_layout != ScreenLayout::TopOnly)
   {
      switch(state->current_touch_mode)
      {
         case TouchMode::Disabled:
            state->touching = false;
            break;
         case TouchMode::Mouse:
            {
               int16_t mouse_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
               int16_t mouse_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

               state->touching = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);

               state->touch_x = Clamp(state->touch_x + mouse_x, 0, VIDEO_WIDTH - 1);
               state->touch_y = Clamp(state->touch_y + mouse_y, 0, VIDEO_HEIGHT - 1);
            }

            break;
         case TouchMode::Touch:
            if(input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED))
            {
               int16_t pointer_x = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
               int16_t pointer_y = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);

               unsigned int touch_scale = screen_layout_data.displayed_layout == ScreenLayout::HybridBottom ? screen_layout_data.hybrid_ratio : 1;

               unsigned int x = ((int)pointer_x + 0x8000) * screen_layout_data.buffer_width / 0x10000 / touch_scale;
               unsigned int y = ((int)pointer_y + 0x8000) * screen_layout_data.buffer_height / 0x10000 / touch_scale;

               if ((x >= screen_layout_data.touch_offset_x) && (x < screen_layout_data.touch_offset_x + screen_layout_data.screen_width) &&
                     (y >= screen_layout_data.touch_offset_y) && (y < screen_layout_data.touch_offset_y + screen_layout_data.screen_height))
               {
                  state->touching = true;

                  state->touch_x = Clamp((x - screen_layout_data.touch_offset_x) * VIDEO_WIDTH / screen_layout_data.screen_width, 0, VIDEO_WIDTH - 1);
                  state->touch_y = Clamp((y - screen_layout_data.touch_offset_y) * VIDEO_HEIGHT / screen_layout_data.screen_height, 0, VIDEO_HEIGHT - 1);
               }
            }
            else if(state->touching)
            {
               state->touching = false;
            }

            break;
         case TouchMode::Joystick:
            int16_t joystick_x = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X) / 2048;
            int16_t joystick_y = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y) / 2048;

            state->touch_x = Clamp(state->touch_x + joystick_x, 0, VIDEO_WIDTH - 1);
            state->touch_y = Clamp(state->touch_y + joystick_y, 0, VIDEO_HEIGHT - 1);

            state->touching = !!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3);

            break;
      }
   }
   else
   {
      state->touching = false;
   }

   if(state->touching || gesture_button != 0)
   {
         switch(gesture_button)
         {
            case 1:
               log_cb(RETRO_LOG_INFO, std::string("HOTKEY: Touch positions: X = 127, Y = " + std::to_string(swipe_position_x) + "\n").c_str());
               NDS::TouchScreen(127, swipe_position_x);
               swipe_position_x += 25;

               if(swipe_position_x > 205)
               {
                  log_cb(RETRO_LOG_INFO, std::string("HOTKEY: Release Screen\n").c_str());
                  NDS::ReleaseScreen();
                  gesture_button = 0;
               }
               break;

            case 2:
               log_cb(RETRO_LOG_INFO, std::string("HOTKEY: Touch positions: X = 127, Y = " + std::to_string(swipe_position_x) + "\n").c_str());
               NDS::TouchScreen(127, swipe_position_x);
               swipe_position_x -= 25;

               if(swipe_position_x < 130)
               {
                  log_cb(RETRO_LOG_INFO, std::string("HOTKEY: Release Screen\n").c_str());
                  NDS::ReleaseScreen();
                  gesture_button = 0;
               }
               break;

            default:
               NDS::TouchScreen(state->touch_x, state->touch_y);
               log_cb(RETRO_LOG_INFO, std::string("TOUCH: Touch positions: X = " + std::to_string(state->touch_x) + ", Y = " + std::to_string(state->touch_y) + "\n").c_str());
               has_touched = true;
               break;
         }
   }
   else if(has_touched)
   {
      log_cb(RETRO_LOG_INFO, std::string("TOUCH: Release Screen\n").c_str());
      NDS::ReleaseScreen();
      has_touched = false;
   }
}
