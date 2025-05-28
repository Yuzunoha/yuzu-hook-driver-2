#include <stdio.h>

// 他のファイルで定義された関数の宣言
int add(int a, int b);

// 実行コマンド
// gcc -o main *.c; chmod +x main; ./main; rm ./main;
int main() {
  printf("こんにちは!!\n");
  int result = add(5, 3);
  printf("Result: %d\n", result);
  result = add(5, -4);
  printf("Result: %d\n", result);
  return 0;
}
