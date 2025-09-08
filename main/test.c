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
int search_keyboard_event_paths(char keyboard_paths[8][32]);

void test()
{
    // キーボードイベントのパスへの配列
    char keyboard_paths[8][32];
    int num_keyboards = search_keyboard_event_paths(keyboard_paths);
    for (int i = 0; i < num_keyboards; i++)
    {
        printf("Found keyboard at: %s\n", keyboard_paths[i]);
    }
}
