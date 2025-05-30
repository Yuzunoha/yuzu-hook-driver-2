#include <stdio.h>
#include <stdlib.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#define KEY_DELAY 1000000 // 1秒の遅延

/**
 * write関数のwrapper
 */
void write_wrap(int fd, unsigned short type, unsigned short code, signed int value)
{
  struct input_event ev;
  ev.type = type;
  ev.code = code;
  ev.value = value;
  write(fd, &ev, sizeof(ev));
}

/**
 * 同期イベントを実行する関数。これがないと直前のイベントが処理されない
 */
void ev_syn_wrap(int fd)
{
  write_wrap(fd, EV_SYN, SYN_REPORT, 0);
}

/**
 * キーを下げる(value:1)または上げる(value:0)、同期イベント込みの関数
 */
void key_down_or_up_with_sync(int fd, unsigned short code, signed int value)
{
  write_wrap(fd, EV_KEY, code, value);
  ev_syn_wrap(fd);
}

/**
 * キーを下げる、同期イベント込みの関数
 */
void key_down_with_sync(int fd, unsigned short code)
{
  key_down_or_up_with_sync(fd, code, 1);
}

/**
 * キーを上げる、同期イベント込みの関数
 */
void key_up_with_sync(int fd, unsigned short code)
{
  key_down_or_up_with_sync(fd, code, 0);
}

int main()
{
  int fd;
  struct uinput_user_dev uidev;

  printf("Starting virtual keyboard program...\n");

  // uinputデバイスを開く
  fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (fd < 0)
  {
    fprintf(stderr, "Error: Failed to open /dev/uinput\n");
    perror("open");
    return 1;
  }

  printf("Successfully opened /dev/uinput\n");

  // uinputデバイスの設定
  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Virtual Keyboard");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor = 0x1;
  uidev.id.product = 0x1;
  uidev.id.version = 1;

  // キーボードイベントを設定
  if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
  {
    fprintf(stderr, "Error: Failed to set EV_KEY\n");
    perror("ioctl");
    close(fd);
    return 1;
  }

  // 複数のキーを設定
  const int keys[] = {KEY_A, KEY_DOWN, KEY_B, KEY_C, KEY_D, KEY_E, KEY_ENTER};
  for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
  {
    if (ioctl(fd, UI_SET_KEYBIT, keys[i]) < 0)
    {
      fprintf(stderr, "Error: Failed to set key %d\n", keys[i]);
      perror("ioctl");
      close(fd);
      return 1;
    }
  }

  printf("Successfully configured uinput device\n");

  // デバイスを作成
  if (write(fd, &uidev, sizeof(uidev)) < 0)
  {
    fprintf(stderr, "Error: Failed to write uinput device data\n");
    perror("write");
    close(fd);
    return 1;
  }

  if (ioctl(fd, UI_DEV_CREATE) < 0)
  {
    fprintf(stderr, "Error: Failed to create uinput device\n");
    perror("ioctl");
    close(fd);
    return 1;
  }

  printf("Virtual keyboard device created\n");

  // アプリ切り替えの猶予時間
  usleep(KEY_DELAY * 3);

  // キーを順番に押す
  for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
  {
    // キーを押す
    key_down_with_sync(fd, keys[i]);
    // キーを離す
    key_up_with_sync(fd, keys[i]);
    // 1秒待機
    usleep(KEY_DELAY);
  }

  printf("Destroying virtual keyboard device...\n");

  // デバイスを削除
  if (ioctl(fd, UI_DEV_DESTROY) < 0)
  {
    fprintf(stderr, "Error: Failed to destroy uinput device\n");
    perror("ioctl");
  }

  close(fd);
  printf("Program finished\n");

  return 0;
}
