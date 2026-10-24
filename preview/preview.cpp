// Desktop preview: runs the same effects as the board and draws them in the
// terminal with 24-bit color. Build and run with:  pio run -e preview -t exec
//
// Keys: arrows / WASD tilt, space or n = next effect, q = quit.

#include <Effects.h>

#include <chrono>
#include <cstdio>
#include <thread>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {

#ifdef _WIN32
struct Terminal {
  Terminal() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }
  int readKey() {
    if (!_kbhit()) return -1;
    int c = _getch();
    if (c == 0 || c == 224) {
      switch (_getch()) {
        case 72: return 'w';
        case 80: return 's';
        case 75: return 'a';
        case 77: return 'd';
      }
      return -1;
    }
    return c;
  }
};
#else
struct Terminal {
  termios saved{};
  Terminal() {
    tcgetattr(STDIN_FILENO, &saved);
    termios raw = saved;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  }
  ~Terminal() { tcsetattr(STDIN_FILENO, TCSANOW, &saved); }
  int readKey() {
    pollfd p{STDIN_FILENO, POLLIN, 0};
    if (poll(&p, 1, 0) <= 0) return -1;
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return -1;
    if (c != 27) return c;
    unsigned char seq[2];
    if (read(STDIN_FILENO, seq, 2) != 2 || seq[0] != '[') return 27;
    switch (seq[1]) {
      case 'A': return 'w';
      case 'B': return 's';
      case 'D': return 'a';
      case 'C': return 'd';
    }
    return -1;
  }
};
#endif

void draw(const Frame& f, const char* name, float tx, float ty) {
  std::printf("\x1b[H");
  for (int y = 0; y < kHeight; y++) {
    for (int x = 0; x < kWidth; x++) {
      Rgb c = f.get(x, y);
      std::printf("\x1b[38;2;%d;%d;%dm\xe2\x96\x88\xe2\x96\x88", c.r, c.g, c.b);
    }
    std::printf("\x1b[0m\n");
  }
  std::printf("\n%-10s tilt x=%+.2f y=%+.2f\x1b[K\n", name, tx, ty);
  std::printf("arrows/WASD tilt  space next  q quit\x1b[K\n");
  std::fflush(stdout);
}

}  // namespace

int main() {
  Terminal term;
  std::printf("\x1b[2J\x1b[?25l");

  Frame frame;
  int current = 0;
  float tiltX = 0, tiltY = 0;
  effectAt(current).reset();
  auto start = std::chrono::steady_clock::now();

  for (;;) {
    for (int k; (k = term.readKey()) != -1;) {
      switch (k) {
        case 'q': case 'Q': case 27:
          std::printf("\x1b[0m\x1b[?25h\n");
          return 0;
        case ' ': case 'n':
          current = (current + 1) % effectCount();
          effectAt(current).reset();
          frame.clear();
          break;
        case 'a': tiltX -= 0.25f; break;
        case 'd': tiltX += 0.25f; break;
        case 'w': tiltY -= 0.25f; break;
        case 's': tiltY += 0.25f; break;
      }
    }
    if (tiltX > 1) tiltX = 1;
    if (tiltX < -1) tiltX = -1;
    if (tiltY > 1) tiltY = 1;
    if (tiltY < -1) tiltY = -1;

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - start).count();
    effectAt(current).render(frame, Inputs{(uint32_t)ms, tiltX, tiltY});
    draw(frame, effectAt(current).name(), tiltX, tiltY);
    std::this_thread::sleep_for(std::chrono::milliseconds(33));
  }
}
