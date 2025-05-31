#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

int main()
{
    const char *device = "/dev/input/event4"; // キーボードのイベントデバイス
    struct input_event ev;
    int fd;

    // デバイスファイルを読み取り専用でオープン
    fd = open(device, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return EXIT_FAILURE;
    }

    printf("Reading from %s (press Ctrl+C to exit)\n", device);

    while (1)
    {
        ssize_t n = read(fd, &ev, sizeof(struct input_event));
        if (n == (ssize_t)sizeof(struct input_event))
        {
            if (ev.type == EV_KEY)
            {
                printf("Key %d %s\n", ev.code,
                       ev.value == 0 ? "RELEASED" : ev.value == 1 ? "PRESSED"
                                                : ev.value == 2   ? "REPEATED"
                                                                  : "UNKNOWN");
            }
        }
    }

    close(fd);
    return 0;
}
