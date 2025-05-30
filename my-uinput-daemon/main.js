const fs = require('fs');
const ref = require('ref-napi');
const Struct = require('ref-struct-di')(ref);
const ArrayType = require('ref-array-di')(ref);
const koffi = require('koffi');

// 定数
const EV_KEY = 0x01;
const EV_SYN = 0x00;
const SYN_REPORT = 0;
const UI_SET_EVBIT = 1074025828;
const UI_SET_KEYBIT = 1074025829;
const UI_DEV_CREATE = 21761;
const UI_DEV_DESTROY = 21762;

// キーコード
const KEY_UP = 103;

// `/dev/uinput` を開く
const fd = fs.openSync('/dev/uinput', 'w');

// `input_event` 構造体
const timeval = Struct({
  tv_sec: 'long',
  tv_usec: 'long',
});

const input_event = Struct({
  time: timeval,
  type: 'ushort',
  code: 'ushort',
  value: 'int',
});

// `uinput_user_dev` 構造体
const input_id = Struct({
  bustype: 'ushort',
  vendor: 'ushort',
  product: 'ushort',
  version: 'ushort',
});

const uinput_user_dev = Struct({
  name: ArrayType('char', 80),
  id: input_id,
  ff_effects_max: 'int',
  absmax: ArrayType('int', 64),
  absmin: ArrayType('int', 64),
  absfuzz: ArrayType('int', 64),
  absflat: ArrayType('int', 64),
});

// `ioctl` 呼び出し
const libc = koffi.load('libc.so.6');
libc.func('int ioctl(int fd, ulong request, ...)', 'ioctl');

// イベントを有効化
libc.ioctl(fd, UI_SET_EVBIT, EV_KEY);
libc.ioctl(fd, UI_SET_KEYBIT, KEY_UP);

// デバイス情報を書き込み
const dev = new uinput_user_dev();
dev.name[0] = 'n'.charCodeAt(0); // 名前は何でも良い
dev.id.bustype = 0x03; // BUS_USB
dev.id.vendor = 0x1234;
dev.id.product = 0x5678;
dev.id.version = 1;

fs.writeSync(fd, dev.ref());
fs.writeSync(fd, Buffer.alloc(4096 - dev.ref().length)); // パディング

// 仮想キーボード作成
libc.ioctl(fd, UI_DEV_CREATE);
console.log('仮想キーボード作成完了');

// KEY_UP イベント送信（押す）
const ev_press = new input_event();
ev_press.type = EV_KEY;
ev_press.code = KEY_UP;
ev_press.value = 1;
fs.writeSync(fd, ev_press.ref());

// KEY_UP イベント送信（離す）
const ev_release = new input_event();
ev_release.type = EV_KEY;
ev_release.code = KEY_UP;
ev_release.value = 0;
fs.writeSync(fd, ev_release.ref());

// SYN イベントで送信完了
const ev_syn = new input_event();
ev_syn.type = EV_SYN;
ev_syn.code = SYN_REPORT;
ev_syn.value = 0;
fs.writeSync(fd, ev_syn.ref());

console.log('↑キー送信完了！');

// 終了するなら↓も（一定時間後に）
setTimeout(() => {
  libc.ioctl(fd, UI_DEV_DESTROY);
  console.log('仮想デバイス削除');
  fs.closeSync(fd);
}, 3000);
