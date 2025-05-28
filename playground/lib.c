#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <string.h>

void print_keyboard_device() {
  for (int i = 0; i < 10; i++) { // 例として0から9までのeventファイルをチェック
    char path[20];
    snprintf(path, sizeof(path), "/dev/input/event%d", i);
    
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
      continue; // 開けなかった場合はスキップ
    }

    struct input_id id;
    if (ioctl(fd, EVIOCGID, &id) == -1) {
      perror("ioctl");
      close(fd);
      continue;
    }

    // デバイスタイプの確認
    if (id.bustype == BUS_USB || id.bustype == BUS_HOST) {
      struct input_event ev;
      if (read(fd, &ev, sizeof(ev)) > 0) {
        if (ev.type == EV_KEY) {
          printf("%s is a keyboard device\n", path);
        }
      }
    }

    close(fd);
  }
}

int add(int x, int y) {
  return x + y;
}
