# Which pio to run: $PIO if set, else PlatformIO's own ~/.platformio/penv copy if it exists
# (the VS Code extension's install), else whatever `pio` is on PATH (e.g. the cloud setup).
penv_pio := home_directory() / ".platformio/penv/bin/pio"
default_pio := if path_exists(penv_pio) == "true" { penv_pio } else { "pio" }
pio := env("PIO", default_pio)

default:
    @just --list

# Flash the real board (if the port isn't found: hold BOOT, tap RESET, release BOOT, retry, then press RESET)
flash:
    {{pio}} run -e matrix -t upload

# Serial monitor, 115200 baud
monitor:
    {{pio}} device monitor

# Flash, then attach the serial monitor
flash-monitor: flash monitor

# Build for Wokwi, then run F1 > "Wokwi: Start Simulator" in VS Code
sim:
    {{pio}} run -e sim

# Interactive terminal preview (needs a TTY; arrows/WASD tilt, space next, q quit)
tui:
    {{pio}} run -e preview -t exec

# Browser preview on http://localhost:8000 with live reload (needs em++)
web:
    node web/serve.mjs

# Compile every host without flashing; the first ESP32 build downloads the toolchain
check:
    {{pio}} run -e matrix
    {{pio}} run -e sim
    {{pio}} run -e preview
    node web/serve.mjs build
