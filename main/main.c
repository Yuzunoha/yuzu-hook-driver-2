#include <fcntl.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define MAX_KEYBOARD_DEVICES 8

// 外部関数
void event_modifier_in_loop(struct input_event *ev);
int search_keyboard_event_paths(char keyboard_paths[MAX_KEYBOARD_DEVICES][32]);

// 本物の入力デバイスのリスト
int g_input_fds[MAX_KEYBOARD_DEVICES];

// 本物の入力デバイスの数
int g_actual_keyboard_num;

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

// 指定した入力デバイスに、リリースされていないキーがあれば1を返す関数
int has_unreleased_keys(int fd)
{
  unsigned char key_states[KEY_MAX / 8 + 1];
  memset(key_states, 0, sizeof(key_states));

  // このサイズ分のキー情報を取得する
  if (ioctl(fd, EVIOCGKEY(sizeof(key_states)), key_states) < 0)
  {
    perror("ioctl EVIOCGKEY");
    return -1;
  }

  // 押されているキーがないか確認するループ
  for (int i = 0; i < 255; i++)
  {
    if (key_states[i / 8] & (1 << (i % 8)))
    {
      fprintf(stderr, "Key %d is still pressed.\n", i);
      return 1;
    }
  }

  // 押されているキーはなかった
  return 0;
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
  for (int i = 0; i < g_actual_keyboard_num; i++)
  {
    ioctl(g_input_fds[i], EVIOCGRAB, 0);
    close(g_input_fds[i]);
  }
  ioctl(g_uinput_fd, UI_DEV_DESTROY);
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
  // キーボードイベントをサーチする
  char keyboard_paths[MAX_KEYBOARD_DEVICES][32];
  g_actual_keyboard_num = search_keyboard_event_paths(keyboard_paths);
  if (0 == g_actual_keyboard_num)
  {
    printf("no keyboard found\n");
    return 0;
  }

  // 実キーボードのファイルを読み込み専用で開く
  for (int i = 0; i < g_actual_keyboard_num; i++)
  {
    g_input_fds[i] = open(keyboard_paths[i], O_RDONLY);
    // キーボードを "つかむ"
    sleep(1);
    if (has_unreleased_keys(g_input_fds[i]) > 0)
    {
      fprintf(stderr, "Unreleased keys detected. Exiting.\n");
      close(g_input_fds[i]);
      return 1;
    }
    ioctl(g_input_fds[i], EVIOCGRAB, 1);
  }

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
  fd_set readfds;
  int max_fd = -1;
  struct input_event ev;
  while (1)
  {
    FD_ZERO(&readfds);
    // 全てのキーボードをセット
    for (int i = 0; i < g_actual_keyboard_num; i++)
    {
      FD_SET(g_input_fds[i], &readfds);
      if (g_input_fds[i] > max_fd)
      {
        max_fd = g_input_fds[i];
      }
    }

    // イベントを待機
    if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
    {
      perror("select");
      break;
    }

    // どのキーボードからイベントが来たかチェック
    for (int i = 0; i < g_actual_keyboard_num; i++)
    {
      if (FD_ISSET(g_input_fds[i], &readfds))
      {
        if (read(g_input_fds[i], &ev, sizeof(ev)) == sizeof(ev))
        {
          // イベントに変換をかける
          event_modifier_in_loop(&ev);
          // イベントを書く
          write(g_uinput_fd, &ev, sizeof(ev));
        }
      }
    }
  }

  // 念の為クリーンアップ。通常このアプリは停止信号を受信する。
  // その場合ここには来ないが、シグナルハンドラによってcleanは実行される。
  cleanup_and_exit();
  return 0;
}
