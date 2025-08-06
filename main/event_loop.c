#include <linux/input.h>
#include <stdio.h>

// Linuxにおけるvalueが表すキーの状態 (0,1,2) = (離された,押された,押し続け)
int g_value_muhenkan;

// 無変換+Jキー同時押し中に無変換だけリリースされたときなどの対策のためのフラグ。
int muhenkan_modification_is_active;

// 無変換押下中の各キーの振る舞いを決める関数。defaultに入ったときのみ0を返す
int event_modifier_for_muhenkan_pressed(struct input_event *ev)
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
    case KEY_P:
        ev->code = KEY_DELETE;
        break;
    case KEY_M:
        ev->code = KEY_ENTER;
        break;
    default:
        return 0;
    }
    return 1;
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

    if (ev->code == KEY_KATAKANAHIRAGANA)
    {
        // カタカナキーは右Altにリマップする
        ev->code = KEY_RIGHTALT;
        return;
    }

    if ((0 < g_value_muhenkan) || muhenkan_modification_is_active)
    {
        // 無変換が押されているときの振る舞いをさせる
        muhenkan_modification_is_active = event_modifier_for_muhenkan_pressed(ev);
    }
}
