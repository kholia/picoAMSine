// You will need to install RP2040_PWM library in the Arduino IDE first!

// https://github.com/earlephilhower/arduino-pico/blob/master/libraries/rp2040/examples/Siren/Siren.ino

#define F_CPU 204000000
#define ENABLE_SERIAL

#include "RP2040_PWM.h"

#include "hardware/pll.h"
#include "hardware/clocks.h"
#include <hardware/pio.h>
#include "hardware/irq.h"   // interrupts
#include "hardware/pwm.h"   // pwm
#include "hardware/sync.h"  // wait for interrupt

// Our assembled program:
#include "hello.pio.h"

#include "MorseEncoder.h"

PIO pio = pio0;
uint offset;
uint sm;

// begin clock code

void set_sys_clock_pll(uint32_t vco_freq, uint post_div1, uint post_div2) {
  if (!running_on_fpga()) {
    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    48 * MHZ,
                    48 * MHZ);

    pll_init(pll_sys, 1, vco_freq, post_div1, post_div2);
    uint32_t freq = vco_freq / (post_div1 * post_div2);

    // Configure clocks
    // CLK_REF = XOSC (12MHz) / 1 = 12MHz
    clock_configure(clk_ref,
                    CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC,
                    0,  // No aux mux
                    12 * MHZ,
                    12 * MHZ);

    // CLK SYS = PLL SYS (125MHz) / 1 = 125MHz
    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                    freq, freq);

    clock_configure(clk_peri,
                    0,  // Only AUX mux on ADC
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    48 * MHZ,
                    48 * MHZ);
  }
}

bool check_sys_clock_khz(uint32_t freq_khz, uint *vco_out, uint *postdiv1_out, uint *postdiv_out) {
  uint crystal_freq_khz = clock_get_hz(clk_ref) / 1000;
  for (uint fbdiv = 320; fbdiv >= 16; fbdiv--) {
    uint vco = fbdiv * crystal_freq_khz;
    if (vco < 400000 || vco > 1600000) continue;
    for (uint postdiv1 = 7; postdiv1 >= 1; postdiv1--) {
      for (uint postdiv2 = postdiv1; postdiv2 >= 1; postdiv2--) {
        uint out = vco / (postdiv1 * postdiv2);
        if (out == freq_khz && !(vco % (postdiv1 * postdiv2))) {
          *vco_out = vco * 1000;
          *postdiv1_out = postdiv1;
          *postdiv_out = postdiv2;
          return true;
        }
      }
    }
  }
  return false;
}

static inline bool set_sys_clock_khz(uint32_t freq_khz, bool required) {
  uint vco, postdiv1, postdiv2;
  if (check_sys_clock_khz(freq_khz, &vco, &postdiv1, &postdiv2)) {
    set_sys_clock_pll(vco, postdiv1, postdiv2);
    return true;
  } else if (required) {
    panic("System clock of %u kHz cannot be exactly achieved", freq_khz);
  }
  return false;
}
// end clock code

// https://wokwi.com/tools/pioasm
// https://www.instructables.com/Using-RP2040-PIO-in-Arduino-IDE-on-Windows/
// https://parthssharma.github.io/Pico/PIOSquareWave.html
// https://github.com/rgrosset/pico-pwm-audio

RP2040_PWM *pwm;
RP2040_PWM *led;

// Audio PIN is to match some of the design guide shields.
#define AUDIO_PIN 13  // you can change this to whatever you like

/*
   This include brings in static arrays which contain audio samples.
   if you want to know how to make these please see the python code
   for converting audio samples into static arrays.
*/
#include "sample.h"
int wav_position = 0;

/*
   PWM Interrupt Handler which outputs PWM level and advances the
   current sample.

   We repeat the same value for 8 cycles this means sample rate etc
   adjust by factor of 8   (this is what bitshifting <<3 is doing)

*/
void pwm_interrupt_handler() {
  pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN));
  if (wav_position < (WAV_DATA_LENGTH << 3) - 1) {
    // set pwm level
    // allow the pwm value to repeat for 8 cycles this is >>3
    pwm_set_gpio_level(AUDIO_PIN, WAV_DATA[wav_position >> 3]);
    wav_position++;
  } else {
    // reset to start
    wav_position = 0;
  }
}

#define TONEPIN 16  // GP16

MorseEncoder morseA(TONEPIN);

void setup() {
#ifdef ENABLE_SERIAL
  Serial.begin(9600);
#endif
  analogReadResolution(12);
  set_sys_clock_khz(204000, true);

  gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

  int audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_PIN);

  // Setup PWM interrupt to fire when PWM cycle is complete
  pwm_clear_irq(audio_pin_slice);
  pwm_set_irq_enabled(audio_pin_slice, true);
  // set the handle function above
  irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_interrupt_handler);
  irq_set_enabled(PWM_IRQ_WRAP, true);

  // Setup PWM for audio output
  pwm_config config = pwm_get_default_config();
  /* Base clock 176,000,000 Hz divide by wrap 250 then the clock divider further divides
     to set the interrupt rate.

     11 KHz is fine for speech. Phone lines generally sample at 8 KHz


     So clkdiv should be as follows for given sample rate
      8.0f for 11 KHz
      4.0f for 22 KHz
      2.0f for 44 KHz etc
  */
  pwm_config_set_clkdiv(&config, 8.0f);
  pwm_config_set_wrap(&config, 250);
  pwm_init(audio_pin_slice, &config, true);

  pwm_set_gpio_level(AUDIO_PIN, 0);
  pwm = new RP2040_PWM(15, 1557000, 0);
  pwm->enablePWM();
  uint16_t PWM_TOP = pwm->get_TOP();
  uint16_t PWM_DIV = pwm->get_DIV();
  uint16_t PWM_Level = 0;
  // setPWM_manual(uint8_t pin, uint16_t top, uint8_t div, uint16_t level, bool phaseCorrect = false)
  pwm->setPWM_manual(15, PWM_TOP, PWM_DIV, PWM_Level, true);
  led = new RP2040_PWM(LED_BUILTIN, 1557000, 100);
  led->enablePWM();
  PWM_TOP = led->get_TOP();
  PWM_DIV = led->get_DIV();
  PWM_Level = 65535;
  // setPWM_manual(uint8_t pin, uint16_t top, uint8_t div, uint16_t level, bool phaseCorrect = false)
  led->setPWM_manual(LED_BUILTIN, PWM_TOP, PWM_DIV, PWM_Level, false);

  Serial.println(pwm->get_freq_CPU());
  Serial.println(pwm->getActualFreq());
  delay(200);

  // PIO program stuff
  offset = pio_add_program(pio, &PIOMaxSquareWave_program);
  sm = pio_claim_unused_sm(pio, true);
  PIOMaxSquareWave_program_init(pio, sm, offset, 12, 600000.0f);

  morseA.beginAudio(20, 700);  // WPM and frequency
}

long t = millis();
long n = 0;
float m = 0;

void loop() {
  float v = ((float)analogRead(26) / 4096.0 * 200.0 + 10);
  pwm->setPWM_DCPercentage_manual(15, v);
  led->setPWM_DCPercentage_manual(LED_BUILTIN, v);
#ifdef ENABLE_SERIAL
  n = n + 1;
  if (m < v) {
    m = v;
  }
  if (n % 100000 == 0) {
    Serial.print((float)n / (((float)(millis() - t)) / 1000) / 1000);
    Serial.print("kHz (max=");
    Serial.print(m);
    Serial.println(")");
    t = millis();
    n = 0;
    m = 0;
  }
#endif
}

void loop1() {
  // Siren code
  /* for (int i = 100; i < 10000; i += 1) {
    tone(TONEPIN, i);
    delayMicroseconds(20);
    } */

  String message = "CQ CQ CQ DE WQ6W";
  morseA.print(message);
  delay(2000);
}
