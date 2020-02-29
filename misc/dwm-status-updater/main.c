#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <pthread.h>
#include <X11/Xlib.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

const static time_t MU_SLEEP_TIME = 5 * 1000000;

/* unicode glyphs */
const static char CALENDAR = 0;
const static char CLOCK = 0x33;

/* TODO: this is kind of garbage but I'll figure out a better way to
         do this in the future */
#define LAST_NOTIFICATION_MESSAGE_LEN 50
char LAST_NOTIFICATION_ACC = 0;
char LAST_NOTIFICATION_MESSAGE[2][LAST_NOTIFICATION_MESSAGE_LEN];

void engrave_date(char *_buffer, size_t _buff_size);
void engrave_wifi(char *_buffer, size_t _buff_size);
void engrave_batt(char *_buffer, size_t _buff_size);
void engrave_mess(char *_buffer, size_t _buff_size);
void *notification_service(void *data);
uint16_t convert_battery_reading();

int main(int argc, char* argv[]) {
  (void) argc, (void) argv;

  void (*engravers[])(char*, size_t) = {
    &engrave_mess,
    &engrave_wifi,
    &engrave_batt,
    &engrave_date
  };

  size_t engraver_size = sizeof(engravers) / sizeof(&engrave_date);

  static Display* display = NULL;
  static int screen = 0;
  size_t buff_size = 255;
  char buffer[buff_size];

  // start any other services here
  {
    pthread_t tinfo;

    const int ret =
      pthread_create(&tinfo, NULL, &notification_service, NULL);

    if (ret != 0) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  // main loop
  while (1) {
    buffer[0] = 0;
    display = XOpenDisplay(NULL);

    for (size_t x = 0; x < engraver_size; ++x)
      engravers[x](buffer, buff_size);

    screen = DefaultScreen(display);

    const Window root = RootWindow(display, screen);
    if (!root)
      break;

    XStoreName(display, root, buffer);
    XCloseDisplay(display);

#ifdef DEBUG
    printf("%s\n", buffer);
#endif

    usleep(MU_SLEEP_TIME);
  }

  return 0;
}

void engrave_mess(char* _buffer, size_t _buff_size) {
  strncat(_buffer, LAST_NOTIFICATION_MESSAGE, _buff_size);
}

void engrave_date(char* _buffer, size_t _buff_size) {
  const time_t now = time(NULL);
  const struct tm time = *localtime(&now);
  const char* date_fmt = "[%02d:%02d][%02d/%02d/%02d]";
  char store[32];

  sprintf(store, date_fmt,
	  time.tm_hour, time.tm_min,
	  time.tm_mday, time.tm_mon + 1, time.tm_year + 1900);

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
  const char* fmt = "[BAT:%03d]";
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

  sprintf(battery, fmt, cumulative_power / bat_count);
  strncat(_buffer, battery, _buff_size);
}

void *notification_service(void *data) {
  (void) data;

  const int port = 9001;

  const int domain = AF_INET;
  const int type = SOCK_DGRAM;
  const int protocol = 0;
  int opt = 1;

  int sock_fd = socket(domain, type, protocol);
  if (sock_fd == 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if ((setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                  &opt, sizeof(opt))) != 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (bind(sock_fd, (struct sockaddr*)&address, sizeof(address)) != 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  // Actual listen loop now
  while (1) {
    recvfrom(sock_fd, current, LAST_NOTIFICATION_MESSAGE_LEN, 0, 0, 0);
  }
}
