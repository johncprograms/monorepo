// build:console_x64_debug

typedef unsigned char u8;
#define _countof(x)   ( sizeof(x) / sizeof(x[0]) )

u8 c_zeros[10] = {0};

u8 g_data[4] = { 1, 2, 3, 4 };

int mainCRTStartup()
{
  int sum = 0;
  for (int i = 0; i < _countof(g_data); ++i) {
    g_data[i] += 1;
    sum += g_data[i] + c_zeros[i];
  }
  return sum;
}

