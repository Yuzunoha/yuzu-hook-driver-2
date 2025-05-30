#include <stdio.h>
#include <stdlib.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main()
{
    int fd;
    struct uinput_user_dev uidev;
    struct input_event ev;

    // uinputデバイスを開く
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Opening /dev/uinput");
        return 1;
    }

    // uinputデバイスの設定
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Virtual Keyboard");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1;
    uidev.id.product = 0x1;
    uidev.id.version = 1;

    // キーボードイベントを設定
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, KEY_A);
    ioctl(fd, UI_SET_KEYBIT, KEY_B);
    ioctl(fd, UI_SET_KEYBIT, KEY_C);

    // デバイスを作成
    if (write(fd, &uidev, sizeof(uidev)) < 0) {
        perror("Writing uinput user device data");
        return 1;
    }

    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        perror("Creating uinput device");
        return 1;
    }

    // キーを押すイベントを送信
    ev.type = EV_KEY;
    ev.code = KEY_A;
    ev.value = 1;  // 押す
    write(fd, &ev, sizeof(ev));

    // 同期イベント
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));

    // キーを離すイベントを送信
    ev.type = EV_KEY;
    ev.code = KEY_A;
    ev.value = 0;  // 離す
    write(fd, &ev, sizeof(ev));

    // 同期イベント
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));

    // デバイスを削除
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);

    return 0;
}
