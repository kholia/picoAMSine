default:
	arduino-cli compile -e --fqbn=rp2040:rp2040:rpipico .

install_platform:
	arduino-cli config init --overwrite
	arduino-cli config add board_manager.additional_urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
	arduino-cli core update-index
	arduino-cli core install rp2040:rp2040

install_deps:
	arduino-cli lib install "RP2040_PWM"
	arduino-cli lib install "MBED_RP2040_PWM"

install_arduino_cli:
	mkdir -p ~/.local/bin
	curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh
