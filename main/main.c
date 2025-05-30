#include <stdio.h>
#include <stdlib.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#define KEY_DELAY 1000000 // 1秒の遅延

int main()
{
  int fd;
  struct uinput_user_dev uidev;
  struct input_event ev;
  struct timespec ts;

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
  const int keys[] = {KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_ENTER};
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

  // キーを順番に押す
  for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
  {
    // キーを押す
    ev.type = EV_KEY;
    ev.code = keys[i];
    ev.value = 1; // 押す
    write(fd, &ev, sizeof(ev));

    // 同期イベント
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));

    printf("Pressed key %d\n", keys[i]);

    // 1秒待機
    usleep(KEY_DELAY);

    // キーを離す
    ev.type = EV_KEY;
    ev.code = keys[i];
    ev.value = 0; // 離す
    write(fd, &ev, sizeof(ev));

    // 同期イベント
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));

    printf("Released key %d\n", keys[i]);
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
