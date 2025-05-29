#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#define DEVICE_PATH "/dev/input/event4"
#define BIT_LEN (KEY_MAX / 8 + 1)

int this_device_path_is_a_keyboard(char *device_path)
{
  int fd;
  unsigned char evbit[EV_MAX / 8 + 1];
  unsigned char keybit[BIT_LEN];

  memset(evbit, 0, sizeof(evbit));
  memset(keybit, 0, sizeof(keybit));

  fd = open(DEVICE_PATH, O_RDONLY);
  if (fd < 0)
  {
    perror("open");
    return 1;
  }

  // イベントタイプ（EV_KEY など）を取得
  if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0)
  {
    perror("ioctl EVIOCGBIT ev");
    close(fd);
    return 1;
  }

  // キーイベントがサポートされているか確認
  if (!(evbit[EV_KEY / 8] & (1 << (EV_KEY % 8))))
  {
    printf("This is NOT a keyboard: no EV_KEY support.\n");
    close(fd);
    return 0;
  }

  // サポートされているキーの一覧を取得
  if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0)
  {
    perror("ioctl EVIOCGBIT key");
    close(fd);
    return 1;
  }

  // 'A' キーなど、キーボードらしいキーがあるかを確認
  if (keybit[KEY_A / 8] & (1 << (KEY_A % 8)))
  {
    printf("This is a keyboard device.\n");
  }
  else
  {
    printf("This is NOT a keyboard: no KEY_A.\n");
  }

  close(fd);
  return 0;
}

int main()
{
  this_device_path_is_keyboard(DEVICE_PATH);
  return 0;
}
