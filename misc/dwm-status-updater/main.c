#include <sys/types.h>
#include <X11/Xlib.h>

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

const static time_t MU_SLEEP_TIME = 5 * 1000000;

/* unicode glyphs */
const static char CALENDAR = 0;
const static char CLOCK = 0x33;

void engrave_date(char* _buffer, size_t _buff_size);
void engrave_wifi(char* _buffer, size_t _buff_size);
void engrave_batt(char* _buffer, size_t _buff_size);
uint16_t convert_battery_reading();

int main(int argc, char* argv[]) {
  (void) argc, (void) argv;

  void (*engravers[])(char*, size_t) =
    {
      &engrave_wifi,
      &engrave_batt,
      &engrave_date
    };

  size_t engraver_size = sizeof(engravers) / sizeof(&engrave_date);

  static Display* display = NULL;
  static int screen = 0;
  size_t buff_size = 255;
  char buffer[buff_size];

  while (1) {
    buffer[0] = 0;
    display = XOpenDisplay(NULL);

    for (size_t x = 0; x < engraver_size; ++x) {
      engravers[x](buffer, buff_size);
    }

    screen = DefaultScreen(display);
    const Window root = RootWindow(display, screen);

    XStoreName(display, root, buffer);
    XCloseDisplay(display);

    printf("%s\n", buffer);
    usleep(MU_SLEEP_TIME);
  }

  return 0;
}

void engrave_date(char* _buffer, size_t _buff_size) {
  const time_t now = time(NULL);
  const struct tm time = *localtime(&now);
  const char* date_fmt = "[%d:%d][%d/%d/%d]";
  char store[32];

  sprintf(store, date_fmt,
	  time.tm_hour, time.tm_min,
	  time.tm_mday, time.tm_mon, time.tm_year + 1900);

  strncat(_buffer, store, _buff_size);
}

void engrave_wifi(char* _buffer, size_t _buff_size) {
  const char* label = "Wifi:";
  const char* fmt = "[%s%s]";
  char wifi[16];

  sprintf(wifi, fmt, label, "todo");
  strncat(_buffer, wifi, _buff_size);
}

void engrave_batt(char* _buffer, size_t _buff_size) {
  const uint8_t max_batteries = 32;
  const char* label = "BAT:";
  const char* fmt = "[%s%d]";
  char battery[16];

  FILE* bat_file = NULL;
  const char* battery_dev_path = "/sys/class/power_supply/BAT%d/capacity";
  size_t path_buffer_size = 255;
  char path_buffer[255];
  uint16_t cumulative_power = 0;

  uint8_t bat_count = 0;
  for (; bat_count < max_batteries; ++bat_count) {
      path_buffer[0] = 0;
      sprintf(path_buffer, battery_dev_path, bat_count);
      bat_file = fopen(path_buffer, "r");

      if (!bat_file)
          break;

      char fcontents[4];
      fread(fcontents, 1, 3, bat_file);
      fclose(bat_file);

      uint16_t tmp_power = 0;
      for (size_t ix = 0; ix < sizeof(fcontents) - 1; ++ix) {
          const uint8_t curr_char = fcontents[ix];
          if (curr_char == 0x0A)
              break;

          tmp_power = (curr_char - 48) + (tmp_power * 10);
      }

      cumulative_power += tmp_power;
  }

  sprintf(battery, fmt, label, cumulative_power / bat_count);
  strncat(_buffer, battery, _buff_size);
}
