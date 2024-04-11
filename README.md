# picoAM

A simple but capable AM transmitter for the Raspberry Pi pico!

Note: Use the https://github.com/earlephilhower/arduino-pico core and install
the `RP2040_PWM` library in your Arduino IDE.

## Important Information

The FCC link for unlicensed educational transmissions is here:

https://www.fcc.gov/media/radio/low-power-radio-general-information

This simplest summation is:

1. Never interfere with anything.

2. Move your frequency if you are being interfered with, DO NOT COMPLAIN.

3. Do not transmit more that 200 feet.

4. Use the broadcast AM and FM bands (COMMON SENSE RULES!)

Unlicensed operation on the AM (535 to 1705 kHz) and FM radio broadcast bands
(88 to 108 MHz) is permitted for some extremely low powered devices covered
under Part 15 of the FCC's rules.

Unlicensed operation on the AM and FM radio broadcast bands is permitted for some extremely low
powered devices covered under Part 15 of the FCC's rules. On FM frequencies, these devices are
limited to an effective service range of approximately 200 feet (61 meters). See 47 CFR (Code of
Federal Regulations) Section 15.239, and the July 24, 1991 Public Notice (still in effect). On the AM
broadcast band, these devices are limited to an effective service range of approximately 200 feet (61
meters). See 47 CFR Sections 15.207, 15.209, 15.219, and 15.221. These devices must accept any
interference caused by any other operation, which may further limit the effective service range.

## CHATHAM MARCONI MUSEUM STEM Class

https://www.chathammarconi.org/

picoAMSine is being taught in an after school STEM program at the Museum in
Chatham Mass mid June 2024 by the Buzzards Bay Radio Club WB2TEV with
instructor Bradshaw Lupton, with the gracious continued assistance of VU3CER
Dhiru Kholia, KL7TF, Tom Farrington, N2EMU Mark Dionne, W1HD
William Way, Eben Franks and Scott Haigh, David Weikel each of whom cheerfully
tested WSPR and/or FT\* communication from the beginning of the COVID pandemic.
The program will be detailed here for other STEM teachers, hams and STEM
students on Cape Cod and throughout the State.

## How it works

The Pi generates a 1557 kHz PWM signal, which will be the carrier wave. The
frequency is not changed, only the pulse width. The pico does pulse width
changes according to the `signal`, and outputs the AM signal on pin 15.

A sine wave signal is available on the `GP12` pin! This `GP12` pin can be
connected to the ADC pin in order to send a RF sine signal out.

Update: The Für Elise melody is available on the `GP13` pin now - enjoy
responsibly! Connect `GP13` pin directly to the ADC pin using a jumper cable.

Update 2: Morse message tune is available on GP16 pin!

## Reproducible compilation setup

```
export PATH=$PATH:$HOME/.local/bin
sudo apt install make python3-pip -y

make install_arduino_cli
make install_platform
make install_deps

make default
```

Once these steps are executed, the resulting firmware can be found at the
`build/rp2040.rp2040.rpipico/picoAMSine.ino.uf2` path.

## How to use it

- Flash the .ino file using the Arduino IDE
- Tune in to 1557 kHz AM on the radio

## Voice transmission

Do you want to transmit an actual voice signal instead of the (possibly
annoying) sine wave?

Just connect the `MAX4466 Electret Microphone Amplifier with Adjustable Gain
Module` to the Pico, and there you go!

## Custom tunes

```
pip3 install -r requirements.txt
```

From https://github.com/rgrosset/pico-pwm-audio?tab=readme-ov-file#usage-guide,

Launch the notebook user interface by issuing the command below in the same
folder as this project. This will open the Jupyter user interface. Open the
notebook file in the user interface.

```
jupyter notebook
```

The notebook itself is fairly self explanatory. Run each cell in order using
the run buttons in the UI. The final cell will create a data array that you can
copy and paste into your project. The notebook is configured to convert just
about any WAV file to a mono 11 kHz data which you can then use in your
projects!

## References

- https://github.com/rgrosset/pico-pwm-audio

- https://github.com/ktauchathuranga/MorseEncoder
