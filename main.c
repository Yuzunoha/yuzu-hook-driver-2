// remap_i_to_o.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <errno.h>
#include <signal.h>

#define DEVICE "/dev/input/event4"

int uinput_fd = -1;
int input_fd = -1;

void cleanup(int signo)
{
  if (uinput_fd >= 0)
  {
    ioctl(uinput_fd, UI_DEV_DESTROY);
    close(uinput_fd);
  }

  if (input_fd >= 0)
  {
    ioctl(input_fd, EVIOCGRAB, 0); // グラブ解除
    close(input_fd);
  }

  printf("\n[INFO] Exit cleanly\n");
  exit(0);
}

int setup_uinput_device()
{
  struct uinput_user_dev uidev;

  uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (uinput_fd < 0)
  {
    perror("open /dev/uinput");
    exit(1);
  }

  ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
  ioctl(uinput_fd, UI_SET_EVBIT, EV_SYN);
  ioctl(uinput_fd, UI_SET_KEYBIT, KEY_O);

  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "i_to_o_remap_dev");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor = 0x1;
  uidev.id.product = 0x1;
  uidev.id.version = 1;

  write(uinput_fd, &uidev, sizeof(uidev));
  if (ioctl(uinput_fd, UI_DEV_CREATE) < 0)
  {
    perror("UI_DEV_CREATE");
    exit(1);
  }

  sleep(1); // デバイス準備待ち
  return uinput_fd;
}

void send_key_event(int fd, int keycode, int value)
{
  struct input_event ev;

  memset(&ev, 0, sizeof(ev));
  gettimeofday(&ev.time, NULL);
  ev.type = EV_KEY;
  ev.code = keycode;
  ev.value = value;
  write(fd, &ev, sizeof(ev));

  memset(&ev, 0, sizeof(ev));
  gettimeofday(&ev.time, NULL);
  ev.type = EV_SYN;
  ev.code = SYN_REPORT;
  ev.value = 0;
  write(fd, &ev, sizeof(ev));
}

int main()
{
  struct input_event ev;

  // SIGINTなどを受けたらクリーンアップする
  signal(SIGINT, cleanup);
  signal(SIGTERM, cleanup);

  // 入力デバイスオープン
  input_fd = open(DEVICE, O_RDONLY);
  if (input_fd < 0)
  {
    perror("open input device");
    return 1;
  }

  // 入力デバイスを「グラブ」して他のプロセスに渡さない
  if (ioctl(input_fd, EVIOCGRAB, 1) < 0)
  {
    perror("EVIOCGRAB");
    return 1;
  }

  // 仮想出力デバイス作成
  setup_uinput_device();

  printf("[INFO] Remapping 'i' to 'o'...\n");

  // イベントループ
  while (1)
  {
    if (read(input_fd, &ev, sizeof(ev)) != sizeof(ev))
    {
      continue;
    }

    if (ev.type == EV_KEY && ev.code == KEY_I)
    {
      // 'i' を 'o' に置き換える
      send_key_event(uinput_fd, KEY_O, ev.value);
    }
  }

  return 0;
}
