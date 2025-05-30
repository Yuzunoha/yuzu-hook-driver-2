const fs = require('fs');
const ref = require('ref-napi');
const Struct = require('ref-struct-di')(ref);
const ArrayType = require('ref-array-di')(ref);
const koffi = require('koffi');

// 定数定義（linux/input.h、linux/uinput.h などを参照）
const EV_KEY = 0x01;
const EV_SYN = 0x00;
const SYN_REPORT = 0;
const UI_SET_EVBIT = 0x40045564;
const UI_SET_KEYBIT = 0x40045565;
const UI_DEV_CREATE = 0x5501;
const UI_DEV_DESTROY = 0x5502;

// キーコード（linux/input-event-codes.h より）
const KEY_UP = 103;
const KEY_DOWN = 108;
const KEY_LEFT = 105;
const KEY_RIGHT = 106;

// uinput_user_dev構造体定義（一部簡略化）
const input_id = Struct({
  bustype: ref.types.uint16,
  vendor: ref.types.uint16,
  product: ref.types.uint16,
  version: ref.types.uint16,
});

const uinput_user_dev = Struct({
  name: ArrayType(ref.types.char, 80),
  id: input_id,
  ff_effects_max: ref.types.int,
  absmax: ArrayType(ref.types.int, 64),
  absmin: ArrayType(ref.types.int, 64),
  absfuzz: ArrayType(ref.types.int, 64),
  absflat: ArrayType(ref.types.int, 64),
});

// libc.soのioctl関数をkoffiでロード
const libc = koffi.load('libc.so.6');
const ioctl = libc.func('int ioctl(int fd, unsigned long request, int value)');
// 1. /dev/uinput を開く
const fd = fs.openSync('/dev/uinput', 'w');

// 2. イベントタイプとキーを設定
ioctl(fd, UI_SET_EVBIT, EV_KEY);
ioctl(fd, UI_SET_KEYBIT, KEY_UP);
ioctl(fd, UI_SET_KEYBIT, KEY_DOWN);
ioctl(fd, UI_SET_KEYBIT, KEY_LEFT);
ioctl(fd, UI_SET_KEYBIT, KEY_RIGHT);

// 3. uinput_user_dev 構造体を初期化
const dev = new uinput_user_dev();
const devNameBuffer = Buffer.alloc(80);
devNameBuffer.write('My Virtual Keyboard\0', 0, 'utf8'); // null終端含める
for (let i = 0; i < 80; i++) {
  dev.name[i] = devNameBuffer[i];
}

dev.id.bustype = 0x03; // BUS_USB
dev.id.vendor = 0x1234;
dev.id.product = 0x5678;
dev.id.version = 1;
dev.ff_effects_max = 0;
// absmaxなどはゼロ初期化されたままでOK

// 4. 構造体のバッファを4096バイトにパディングして書き込み
const devBuffer = Buffer.alloc(4096);
dev.ref().copy(devBuffer, 0, 0, dev.ref().length);

fs.writeSync(fd, devBuffer);

// 5. デバイス作成
ioctl(fd, UI_DEV_CREATE, 0);

console.log('Virtual keyboard device created.');

// 6. イベント送信関数
function sendKeyEvent(keyCode, keyValue) {
  // input_event構造体（16バイト）作成
  // struct input_event {
  //   struct timeval time;
  //   __u16 type;
  //   __u16 code;
  //   __s32 value;
  // };
  const input_event = Struct({
    tv_sec: ref.types.long,
    tv_usec: ref.types.long,
    type: ref.types.uint16,
    code: ref.types.uint16,
    value: ref.types.int32,
  });
  const ev = new input_event();

  // 時刻は0でOK（省略可能）
  ev.tv_sec = 0;
  ev.tv_usec = 0;
  ev.type = EV_KEY;
  ev.code = keyCode;
  ev.value = keyValue;

  const evBuffer = Buffer.alloc(input_event.size);
  ev.ref().copy(evBuffer);
  ev.ref().copy(evBuffer, 0, 0, input_event.size);

  fs.writeSync(fd, evBuffer);

  // 同期イベントを送る
  ev.type = EV_SYN;
  ev.code = SYN_REPORT;
  ev.value = 0;
  ev.ref().copy(evBuffer);
  ev.ref().copy(evBuffer, 0, 0, input_event.size);

  fs.writeSync(fd, evBuffer);
}

// 7. 使い方例：カーソルキー「上」を押して離す
sendKeyEvent(KEY_UP, 1); // 押す
sendKeyEvent(KEY_UP, 0); // 離す

// 8. 終了時にデバイス破棄
process.on('SIGINT', () => {
  ioctl(fd, UI_DEV_DESTROY, 0);
  fs.closeSync(fd);
  console.log('Virtual keyboard device destroyed.');
  process.exit();
});
