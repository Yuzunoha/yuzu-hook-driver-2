#include <linux/input.h>
#include <stdio.h>

void event_modifier_in_loop(struct input_event *ev)
{
    if (ev->code == KEY_I)
    {
        ev->code = KEY_O;
    }
    else if (ev->code == KEY_O)
    {
        ev->code = KEY_A;
    }
}