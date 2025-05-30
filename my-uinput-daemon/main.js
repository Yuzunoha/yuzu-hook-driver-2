const ref = require('ref-napi');
const int = ref.types.int;
const intPtr = ref.refType(int);

const buf = ref.alloc(int); // 整数用のバッファ
buf.writeInt32LE(42); // 値を書き込む

console.log(buf.deref()); // 42が出力される
