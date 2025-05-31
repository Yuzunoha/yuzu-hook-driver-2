#include <fcntl.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main()
{
  struct input_event ev;

  // 実キーボードのファイル（event4）を読み込み専用で開く
  int input_fd = open("/dev/input/event4", O_RDONLY);
  if (input_fd < 0)
  {
    perror("open input");
    return 1;
  }

  // キーボードを "つかむ"
  ioctl(input_fd, EVIOCGRAB, 1);

  // uinput の準備
  int uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (uinput_fd < 0)
  {
    perror("open uinput");
    return 1;
  }

  // 必要なイベント種別とコードを登録（例: EV_KEY 全体）
  ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
  for (int i = 0; i < 256; ++i)
  {
    ioctl(uinput_fd, UI_SET_KEYBIT, i);
  }

  struct uinput_user_dev uidev;
  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "my-virtual-keyboard");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor = 0x1234;
  uidev.id.product = 0xfedc;
  uidev.id.version = 1;

  write(uinput_fd, &uidev, sizeof(uidev));
  ioctl(uinput_fd, UI_DEV_CREATE);

  // バイパスループ
  while (1)
  {
    ssize_t n = read(input_fd, &ev, sizeof(ev));
    if (n != sizeof(ev))
    {
      perror("read input");
      break;
    }

    // そのままuinputに渡す（time含めてOK）
    write(uinput_fd, &ev, sizeof(ev));
  }

  // キーボードを "離す"
  ioctl(input_fd, EVIOCGRAB, 0);
  // クリーンアップ
  ioctl(uinput_fd, UI_DEV_DESTROY);
  close(uinput_fd);
  close(input_fd);
  return 0;
}
