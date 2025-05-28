#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* find_keyboard_event_simple() {
    FILE *fp = fopen("/proc/bus/input/devices", "r");
    if (!fp) {
        perror("Failed to open /proc/bus/input/devices");
        return NULL;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "kbd") && strstr(line, "event")) {
            // "event" 文字列の直後に数字が続いていると仮定してパース
            char *start = strstr(line, "event");
            if (start) {
                char event_path[64];
                snprintf(event_path, sizeof(event_path), "/dev/input/%.*s", 10, start); // eventXまで
                fclose(fp);
                return strdup(event_path); // 呼び出し元で free() 必須
            }
        }
    }

    fclose(fp);
    return NULL;
}

int main() {
    char *keyboard_event = find_keyboard_event_simple();
    if (keyboard_event) {
        printf("Detected keyboard event: %s\n", keyboard_event);
        free(keyboard_event);
    } else {
        printf("Keyboard device not found.\n");
    }
    return 0;
}
