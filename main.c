#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdlib.h>

#define EVDEV_PATH "/dev/input/event4" // 適宜変更
// #define KEY_MUHENKAN 129  // KEY_COMPOSE や KEY_MUHENKAN は環境によって異なる（evtestで確認）
#define KEY_J 36 // 通常は 'j' のスキャンコード

int setup_uinput()
{
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0)
    {
        perror("uinput open");
        exit(1);
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, KEY_DOWN);

    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "custom-virtual-kbd");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x1234;
    uidev.id.product = 0xfedc;
    uidev.id.version = 1;

    write(fd, &uidev, sizeof(uidev));
    ioctl(fd, UI_DEV_CREATE);

    return fd;
}

void send_key(int uinput_fd, int keycode)
{
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    gettimeofday(&ev.time, NULL);
    ev.type = EV_KEY;
    ev.code = keycode;
    ev.value = 1; // key press
    write(uinput_fd, &ev, sizeof(ev));

    ev.value = 0; // key release
    write(uinput_fd, &ev, sizeof(ev));

    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(uinput_fd, &ev, sizeof(ev));
}

int main()
{
    int fd = open(EVDEV_PATH, O_RDONLY);
    if (fd < 0)
    {
        perror("event device open");
        return 1;
    }

    int uinput_fd = setup_uinput();

    bool muhenkan_pressed = false;
    bool j_pressed = false;

    struct input_event ev;

    while (read(fd, &ev, sizeof(ev)) > 0)
    {
        if (ev.type == EV_KEY)
        {
            printf("ev.code: %d\n", ev.code);

            // if (muhenkan_pressed && j_pressed) {
            if (j_pressed)
            {
                printf("無変換+J detected: Sending KEY_DOWN\n");
                send_key(uinput_fd, KEY_DOWN);
                // 押しっぱなしの間連打したくないならどちらかのフラグをfalseに
                muhenkan_pressed = false;
                j_pressed = false;
            }
        }
    }

    ioctl(uinput_fd, UI_DEV_DESTROY);
    close(uinput_fd);
    close(fd);
    return 0;
}
