#include <fcntl.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

// 本物の入力デバイス
int g_input_fd;
// 仮想の入力デバイス
int g_uinput_fd;

// プログラム終了時に呼ばれるべき後始末関数
void cleanup_and_exit()
{
  ioctl(g_input_fd, EVIOCGRAB, 0);
  ioctl(g_uinput_fd, UI_DEV_DESTROY);
  close(g_input_fd);
  close(g_uinput_fd);
  printf("Clean exit on signal. シグナルハンドラが動いたよ。\n");
  exit(0);
}

int main()
{
  struct input_event ev;

  // 実キーボードのファイル（event4）を読み込み専用で開く
  g_input_fd = open("/dev/input/event4", O_RDONLY);
  if (g_input_fd < 0)
  {
    perror("open input");
    return 1;
  }

  // キーボードを "つかむ"
  ioctl(g_input_fd, EVIOCGRAB, 1);

  // uinput の準備
  g_uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (g_uinput_fd < 0)
  {
    perror("open uinput");
    return 1;
  }

  // 必要なイベント種別とコードを登録（例: EV_KEY 全体）
  ioctl(g_uinput_fd, UI_SET_EVBIT, EV_KEY);
  for (int i = 0; i < 256; ++i)
  {
    ioctl(g_uinput_fd, UI_SET_KEYBIT, i);
  }

  struct uinput_user_dev uidev;
  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "my-virtual-keyboard");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor = 0x1234;
  uidev.id.product = 0xfedc;
  uidev.id.version = 1;

  write(g_uinput_fd, &uidev, sizeof(uidev));
  ioctl(g_uinput_fd, UI_DEV_CREATE);

  // シグナルハンドラ設定
  signal(SIGINT, cleanup_and_exit);
  signal(SIGTERM, cleanup_and_exit);
  signal(SIGHUP, cleanup_and_exit);
  signal(SIGQUIT, cleanup_and_exit);

  // バイパスループ
  while (1)
  {
    ssize_t n = read(g_input_fd, &ev, sizeof(ev));
    if (n != sizeof(ev))
    {
      perror("read input");
      break;
    }

    // そのままuinputに渡す（time含めてOK）
    write(g_uinput_fd, &ev, sizeof(ev));
  }

  return 0;
}
