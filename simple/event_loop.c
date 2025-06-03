#include <linux/input.h>
#include <stdio.h>

void event_modifier_in_loop(struct input_event *ev)
{
    if (ev->code == KEY_MUHENKAN)
    {
        // 無変換単体は無効化する。code 0をwriteしても無視されるだけで害はない。
        ev->code = 0;
        return;
    }
    if (ev->code == KEY_I)
    {
        ev->code = KEY_O;
    }
    else if (ev->code == KEY_O)
    {
        ev->code = KEY_A;
    }
}