#include <stdio.h>

// 他のファイルで定義された関数の宣言
int add(int a, int b);
void print_keyboard_device();

// 実行コマンド
// gcc -o main *.c; chmod +x main; ./main; rm ./main;
int main()
{
  print_keyboard_device();
  return 0;
}
