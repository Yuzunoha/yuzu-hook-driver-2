#include <linux/input.h>
#include <stdio.h>

// Linuxにおけるvalueが表すキーの状態 (0,1,2) = (離された,押された,押し続けている)
int g_value_muhenkan;

// 無変換押下中の各キーの振る舞いを決める関数
void event_modifier_for_muhenkan_pressed(struct input_event *ev)
{
    switch (ev->code)
    {
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
        /* 無変換 */
        // 保持している無変換キーの状態を更新する
        g_value_muhenkan = ev->value;
        // 無変換単体は無効化する。code 0をwriteしても無視されるだけで害はない。
        ev->code = 0;
        return;
    }
    if (0 < g_value_muhenkan)
    {
        event_modifier_for_muhenkan_pressed(ev);
    }
}