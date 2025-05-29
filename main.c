#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#define DEVICE "/dev/input/event4" // キーボード入力元

int setup_uinput_device()
{
  int uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (uinput_fd < 0)
  {
    perror("open /dev/uinput");
    exit(1);
  }

  // 対応するイベント種別とキーをセット
  ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
  ioctl(uinput_fd, UI_SET_EVBIT, EV_SYN);

  for (int i = 0; i < 256; i++)
  {
    ioctl(uinput_fd, UI_SET_KEYBIT, i);
  }

  struct uinput_user_dev uidev = {};
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "remap_keyboard");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor = 0x1234;
  uidev.id.product = 0x5678;
  uidev.id.version = 1;

  write(uinput_fd, &uidev, sizeof(uidev));
  ioctl(uinput_fd, UI_DEV_CREATE);

  sleep(1); // デバイスが有効になるまで待つ

  return uinput_fd;
}

void emit(int fd, int type, int code, int value)
{
  struct input_event ev;
  gettimeofday(&ev.time, NULL);
  ev.type = type;
  ev.code = code;
  ev.value = value;
  write(fd, &ev, sizeof(ev));
}

int main()
{
  int input_fd = open(DEVICE, O_RDONLY);
  if (input_fd < 0)
  {
    perror("open input device");
    return 1;
  }

  int uinput_fd = setup_uinput_device();

  struct input_event ev;
  while (1)
  {
    ssize_t n = read(input_fd, &ev, sizeof(ev));
    if (n != sizeof(ev))
      continue;

    if (ev.type == EV_KEY && ev.code == KEY_I)
    {
      ev.code = KEY_O;
    }

    emit(uinput_fd, ev.type, ev.code, ev.value);

    if (ev.type == EV_KEY || ev.type == EV_SYN)
    {
      emit(uinput_fd, EV_SYN, SYN_REPORT, 0);
    }
  }

  // 通常ここに来ないが、終了処理を書くなら以下
  ioctl(uinput_fd, UI_DEV_DESTROY);
  close(uinput_fd);
  close(input_fd);
  return 0;
}
