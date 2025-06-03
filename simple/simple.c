#include <fcntl.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

// 外部関数
void event_modifier_in_loop(struct input_event *ev);

// 本物の入力デバイス
int g_input_fd;

// 仮想の入力デバイス
int g_uinput_fd;

// write関数のwrapper
void write_wrap(int fd, unsigned short type, unsigned short code, signed int value)
{
  struct input_event ev;
  ev.type = type;
  ev.code = code;
  ev.value = value;
  write(fd, &ev, sizeof(ev));
}

// 同期イベントを実行する関数。これがないと直前のイベントが処理されない
void ev_syn_wrap(int fd)
{
  write_wrap(fd, EV_SYN, SYN_REPORT, 0);
}

// 指定されたデバイスのすべてのキーを上げる
void release_all_keys(int fd)
{
  if (fd <= 0)
  {
    return;
  }
  for (int j = 0; j < 255; j++)
  {
    write_wrap(fd, EV_KEY, j, 0);
  }
  ev_syn_wrap(fd);
}

// uidevを作成して設定して返却する関数
struct uinput_user_dev create_uidev()
{
  struct uinput_user_dev uidev;
  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "my-virtual-keyboard");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor = 0x1234;
  uidev.id.product = 0xfedc;
  uidev.id.version = 1;
  return uidev;
}

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

// 後始末関数をシグナルハンドラに登録する関数
void setup_all_signal_handler_for_cleanup()
{
  signal(SIGINT, cleanup_and_exit);
  signal(SIGTERM, cleanup_and_exit);
  signal(SIGHUP, cleanup_and_exit);
  signal(SIGQUIT, cleanup_and_exit);
}

int main()
{
  // 実キーボードのファイル（event4）を読み込み専用で開く
  g_input_fd = open("/dev/input/event4", O_RDONLY);
  if (g_input_fd < 0)
  {
    perror("open input");
    return 1;
  }

  // キーボードを "つかむ"
  sleep(1);
  release_all_keys(g_input_fd);
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

  // uidevを作成する
  struct uinput_user_dev uidev = create_uidev();

  // uidevをuinputに紐付ける
  write(g_uinput_fd, &uidev, sizeof(uidev));
  ioctl(g_uinput_fd, UI_DEV_CREATE);

  // シグナルハンドラを設定する
  setup_all_signal_handler_for_cleanup();

  // ループ
  struct input_event ev;
  while (1)
  {
    // イベントを読む
    read(g_input_fd, &ev, sizeof(ev));
    // イベントに変換をかける
    event_modifier_in_loop(&ev);
    // イベントを書く
    write(g_uinput_fd, &ev, sizeof(ev));
  }

  // 念の為クリーンアップ。通常このアプリは停止信号を受信する。
  // その場合ここには来ないが、シグナルハンドラによってcleanは実行される。
  cleanup_and_exit();
  return 0;
}
