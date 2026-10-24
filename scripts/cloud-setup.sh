#!/bin/bash
# Claude cloud environment setup: PlatformIO (matrix/sim/preview) and em++ (web/serve.mjs).
# Run it as the environment's setup script: bash scripts/cloud-setup.sh
set -u

# Emscripten for the browser preview
(
  apt-get update -qq && apt-get install -y -qq emscripten &&
  # the first em++ run builds libc/libc++ into its cache; do it here so the snapshot keeps it
  echo 'int main(){}' > /tmp/em-warm.cpp && em++ -std=c++17 /tmp/em-warm.cpp -o /tmp/em-warm.js
) > /tmp/setup-emscripten.log 2>&1 &

# PlatformIO plus the toolchains the envs use
(
  uv tool install platformio &&
  ln -sf /root/.local/bin/pio /root/.local/bin/platformio /usr/local/bin/ &&
  pio settings set enable_telemetry No &&
  mkdir -p /tmp/pio-warm && cat > /tmp/pio-warm/platformio.ini <<'INI'
[env:esp32s3]
platform = espressif32@6.13.0
board = esp32-s3-devkitc-1
framework = arduino

[env:native]
platform = native
INI
  pio pkg install -d /tmp/pio-warm
) > /tmp/setup-pio.log 2>&1 &

wait
exit 0
