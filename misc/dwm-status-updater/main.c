#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

const static time_t MU_SLEEP_TIME = 5 * 1000000;

/* unicode glyphs */
const static char CALENDAR = 0;
const static char CLOCK = 0x33;

void engrave_date(char* _buffer, size_t _buff_size);
void engrave_wifi(char* _buffer, size_t _buff_size);
void engrave_batt(char* _buffer, size_t _buff_size);

int main(void) {
  void (*engravers[])(char*, size_t) =
    {
      &engrave_wifi,
      &engrave_batt,
      &engrave_date
    };

  size_t engraver_size = sizeof(engravers) / sizeof(&engrave_date);

  size_t buff_size = 255;
  char buffer[buff_size];
  while (1) {
    buffer[0] = 0;

    for (size_t x = 0; x < engraver_size; ++x) {
      engravers[x](buffer, buff_size);
    }

    printf("%s\n", buffer);
    usleep(MU_SLEEP_TIME);
  }
 
  return 0;
}

void
engrave_date(char* _buffer, size_t _buff_size) {
  const time_t now = time(NULL);
  const struct tm time = *localtime(&now);
  const char* date_fmt = "[%d:%d][%d/%d/%d]";
  char store[32];

  sprintf(store, date_fmt,
	  time.tm_hour, time.tm_min,
	  time.tm_mday, time.tm_mon, time.tm_year);

  strncat(_buffer, store, _buff_size);
}

void
engrave_wifi(char* _buffer, size_t _buff_size) {
  const char* label = "Wifi:";
  const char* fmt = "[%s%d]";
  char wifi[16];

  sprintf(wifi, fmt, label, "somplace");
  strncpy(_buffer, wifi, _buff_size);
}

void
engrave_batt(char* _buffer, size_t _buff_size) {
  const char* label = "BAT:";
  const char* fmt = "[%s%d]";
  char battery[16];

  sprintf(battery, fmt, label, 76);
  strncpy(_buffer, battery, _buff_size);
}
