#include <linux/input.h>
#include <stdio.h>

// Linuxにおけるvalueが表すキーの状態 (0,1,2) = (離された,押された,押し続けている)
int g_value_muhenkan;

// 無変換押下中の各キーの振る舞いを決める関数
void event_modifier_for_muhenkan_pressed(struct input_event *ev)
{
    switch (ev->code)
    {
    case KEY_H:
        ev->code = KEY_LEFT;
        break;
    case KEY_J:
        ev->code = KEY_DOWN;
        break;
    case KEY_K:
        ev->code = KEY_UP;
        break;
    case KEY_L:
        ev->code = KEY_RIGHT;
        break;
    case KEY_O:
        ev->code = KEY_BACKSPACE;
        break;
    default:
        break;
    }
}

void event_modifier_in_loop(struct input_event *ev)
{
    if (ev->code == KEY_MUHENKAN)
    {
        // 保持している無変換キーの状態を更新する
        g_value_muhenkan = ev->value;
        // 無変換単体は無効化する。code 0をwriteしても無視されるだけで害はない。
        ev->code = 0;
        return;
    }

    if (ev->code == KEY_HENKAN)
    {
        // 変換キーは右ctrlにリマップする
        ev->code = KEY_RIGHTCTRL;
        return;
    }

    if (0 < g_value_muhenkan)
    {
        // 無変換が押されているときの振る舞いをさせる
        event_modifier_for_muhenkan_pressed(ev);
        return;
    }
}