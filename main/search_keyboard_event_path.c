#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#define BIT_LEN (KEY_MAX / 8 + 1)

/**
 * openされたfdを使って判定だけする。closeは呼び出し元でやる
 */
int is_keyboard_without_closing(int fd)
{
  unsigned char evbit[EV_MAX / 8 + 1];
  unsigned char keybit[BIT_LEN];
  memset(evbit, 0, sizeof(evbit));
  memset(keybit, 0, sizeof(keybit));

  // イベントタイプ（EV_KEY など）を取得する
  if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0)
  {
    // 取得できなかった
    return 0;
  }

  // キーイベントがサポートされているか確認する
  if (!(evbit[EV_KEY / 8] & (1 << (EV_KEY % 8))))
  {
    // サポートされていなかった
    return 0;
  }

  // サポートされているキーの一覧を取得する
  if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0)
  {
    // 取得できなかった
    return 0;
  }

  // 'A' キーなど、キーボードらしいキーがあるかを確認する
  if (keybit[KEY_A / 8] & (1 << (KEY_A % 8)))
  {
    // あった！キーボードであるとみなす
    return 1;
  }

  // ここまで来たらキーボードではないとみなす
  return 0;
}

int this_device_path_is_keyboard(char *device_path)
{
  // 指定されたフルパスのデバイスを開く
  int fd = open(device_path, O_RDONLY);
  if (fd < 0)
  {
    // 開けなかった
    return 0;
  }
  // 判定だけする
  int result = is_keyboard_without_closing(fd);
  // fdを必ず閉じる
  close(fd);
  // 結果を返却する
  return result;
}

void search_keyboard_event_path(char *arg)
{
  char path[32];
  for (int i = 0; i <= 30; i++)
  {
    snprintf(path, sizeof(path), "/dev/input/event%d", i);
    if (this_device_path_is_keyboard(path))
    {
      strcpy(arg, path);
      return;
    }
    // printf("%sはキーボードで%s\n", path, (result ? "ある" : "ない"));
  }
}

/**
 * 最大8個のキーボードのパスを返却する関数
 */
int search_keyboard_event_paths(char paths[][32])
{
  const int max_count = 8;
  char path[32];
  int found = 0; // 見つかったキーボードの数
  for (int i = 0; i <= 30 && found < max_count; i++)
  {
    snprintf(path, sizeof(path), "/dev/input/event%d", i);
    if (this_device_path_is_keyboard(path))
    {
      strcpy(paths[found], path);
      found++;
    }
  }
  return found;
}
