/*
  rgb_led_m150.c - plugin for M150, MODIFIED!

   Usage: M150 [I<pixel>] [S<strip>] [W<intensity>]

    I<pixel>     - NeoPixel index, available if number of pixels > 1
    S<strip>     - strip index if 2 LED strips are used
    W<intensity> - RGB component, 0 - 255 (R=G=B) -> will give white light

  $536 - length of strip 1.
  $537 - length of strip 2.

*/

#include "driver.h"

#if RGB_LED_ENABLE == 2

#include <math.h>
#include <string.h>

#include "rgb_led_strips.c"

static bool is_neopixels;
static user_mcode_ptrs_t user_mcode;
static on_report_options_ptr on_report_options;

static user_mcode_type_t mcode_check (user_mcode_t mcode)
{
    return mcode == RGB_WriteLEDs
                     ? UserMCode_NoValueWords
                     : (user_mcode.check ? user_mcode.check(mcode) : UserMCode_Unsupported);
}

static status_code_t parameter_validate (float *value)
{
    status_code_t state = Status_OK;

    if(!isintf(*value))
        state = Status_BadNumberFormat;

    if(*value < -0.0f || *value > 255.0f)
        state = Status_GcodeValueOutOfRange;

    return state;
}

static status_code_t mcode_validate (parser_block_t *gc_block)
{
    status_code_t state = Status_OK;

    switch(gc_block->user_mcode) {

        case RGB_WriteLEDs:

            if(gc_block->words.w && (state = parameter_validate(&gc_block->values.w)) != Status_OK)    //white intensity 
                return state;

            if(!(gc_block->words.w)) //required parameter
                return Status_GcodeValueWordMissing;

            if(gc_block->words.s && !(gc_block->values.s == 0.0f || (gc_block->values.s == 1.0f && !!hal.rgb1.out)))    //strip number out of range
                return Status_GcodeValueOutOfRange;

            rgb_ptr_t *strip = gc_block->words.s && gc_block->values.s == 1.0f ? &hal.rgb1 : &hal.rgb0;

            if(gc_block->words.i && strip->num_devices > 1) {                                                           //led number out of range
                if(gc_block->values.ijk[0] < -0.0f || gc_block->values.ijk[0] > (float)(strip->num_devices - 1))
                    state = Status_GcodeValueOutOfRange;
                else
                    gc_block->words.i = Off;
            }

            gc_block->words.k = gc_block->words.r = gc_block->words.u = gc_block->words.w = gc_block->words.s = Off;    //?
            break;

        default:
            state = Status_Unhandled;
            break;
    }

    return state == Status_Unhandled && user_mcode.validate ? user_mcode.validate(gc_block) : state;
}

static void mcode_execute (uint_fast16_t state, parser_block_t *gc_block)
{
    static rgb_color_t color = {0}; // TODO: allocate for all leds?

    bool handled = true;

    if(state != STATE_CHECK_MODE) {

        rgb_ptr_t *strip = gc_block->words.s && gc_block->values.s == 1.0f ? &hal.rgb1 : &hal.rgb0;

        switch(gc_block->user_mcode) {

            case RGB_WriteLEDs:;
                uint16_t device = gc_block->words.i ? (uint16_t)gc_block->values.ijk[0] : 0;       //led number or 0
                rgb_color_t new_color;

                if(gc_block->words.w) {
                    color.R = color.G = color.B = gc_block->values.w;
                }

                new_color.value = color.value;

                if(!gc_block->words.i && strip->num_devices > 1) {          //if I is not specified in command then set all leds
                    for(device = 0; device < strip->num_devices; device++) {
                            strip->out(device, new_color);
                    }
                } else {            //I is specified in command
                    strip->out(device, new_color);
                }

                if(strip->num_devices > 1 && strip->write)
                    strip->write();

                break;

            default:
                handled = false;
                break;
        }
    }

    if(!handled && user_mcode.execute)
        user_mcode.execute(state, gc_block);
}

static void onReportOptions (bool newopt)
{
    on_report_options(newopt);

    if(!newopt)
        report_plugin(hal.rgb0.out
                       ? "RGB LED strips (M150)"
                       : "RGB LED strips (N/A)", "0.05");
}

void rgb_led_init (void)
{
    if(hal.rgb0.out) {

        memcpy(&user_mcode, &grbl.user_mcode, sizeof(user_mcode_ptrs_t));

        grbl.user_mcode.check = mcode_check;
        grbl.user_mcode.validate = mcode_validate;
        grbl.user_mcode.execute = mcode_execute;

        is_neopixels = rgb_led_settings_register();
    }

    on_report_options = grbl.on_report_options;
    grbl.on_report_options = onReportOptions;
}

#endif // RGB_LED_ENABLE
