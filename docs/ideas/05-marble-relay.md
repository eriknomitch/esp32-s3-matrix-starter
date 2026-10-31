# 5. Pass the marble between boards

When the existing `Marble` rolls off the edge of board A, it shows up on the matching edge of board B over ESP-NOW, Espressif's direct board-to-board radio protocol (no router needed). Line up two boards, or four in a square, and they become one large tilt table you play together.

**Why it's cool:** it's the only idea here that uses the radio, and it looks like real hardware magic.

**Cost:** you need a second board, and the radio part can't be tried in the previews. The prompt keeps the effect in `lib/Effects` platform-free and adds a loopback link in the terminal preview so the hand-off logic can be tested without hardware.

## Prompt

```text
Make two boards share one marble over ESP-NOW: when it rolls off a linked edge of one
board it continues on the neighboring board. Read CLAUDE.md first.

Design:
- lib/Effects stays platform-free. Add a small interface in Effects.h, e.g.

    struct MarbleLink {
      virtual ~MarbleLink() = default;
      // Called when the marble leaves through `edge` (0=left 1=right 2=top 3=bottom)
      // at position `pos` (0..7, along the edge) with velocity (vx, vy).
      // Return false if no neighbor is linked on that edge (then it bounces).
      virtual bool send(int edge, float pos, float vx, float vy) = 0;
      // Returns true and fills the arguments if a marble arrived since the last call.
      virtual bool receive(int& edge, float& pos, float& vx, float& vy) = 0;
    };

  and a way for a host to attach a link to the marble effect (e.g. a free function
  `setMarbleLink(MarbleLink*)`). With no link attached, Marble behaves exactly as it
  does today. Adjust the names and shape if something cleaner fits the code.
- Marble ownership: exactly one board has the marble at a time. When it leaves through
  a linked edge, this board stops drawing it (let the trail fade out) and sends it. The
  receiving board places it just inside the matching opposite edge with the same
  velocity. Ignore duplicate or stale messages (put a sequence number in the packet).
  Wait about a second at power-up before deciding who owns the marble: the board with
  the lower MAC address spawns it, unless a marble arrives first.
- Firmware (src/): add link_espnow.cpp implementing MarbleLink with esp_now (built into
  Arduino-ESP32 2.0.17; don't add libraries). Use WiFi.mode(WIFI_STA), broadcast to
  FF:FF:FF:FF:FF:FF on a fixed channel, and filter incoming packets by a magic byte and
  version. The ESP-NOW receive callback runs in the WiFi task, so hand the data to
  loop() through a small lock-protected or atomic single-slot buffer.
- Layout: two boards side by side, A's right edge linked to B's left edge. Choose the
  role with a compile-time constant in src/config.h (e.g. cfg::kLinkRole = 0 for none,
  1 for left board, 2 for right board), defaulting to 0 so a single board behaves as
  before. Document how to flash the two boards with different roles (e.g. a build flag
  or two extra PlatformIO envs; whichever is simpler) in the README.
- sim env: Wokwi can't link two boards, so keep the link compiled out under SIM.
- Serial: print the board's MAC and link role at boot, and "Marble -> right" /
  "Marble <- left" on each hand-off.
- Terminal preview: add a loopback mode that draws two boards side by side in the
  terminal, linked by an in-process MarbleLink, with the same faked tilt applied to
  both. That's how the hand-off gets tested without hardware.

Constraints:
- No platform headers in lib/Effects. Don't change cfg::kMaxBrightness or the pinned
  library versions. The Effects registry and other effects are unchanged.
- Update CLAUDE.md for the new interface, the config constant, and the preview mode.

Verify:
- Run `just check` (it must still build matrix, sim, preview, and the browser preview).
- Explain how you checked the hand-off logic in the loopback preview (it needs a TTY, so
  if you can't run it interactively, add a non-interactive way to exercise it or say
  what you couldn't test). List what can only be verified on two real boards.

Then commit and push.
```
