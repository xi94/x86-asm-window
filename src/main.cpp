#include <iostream>
#include <Windows.h>
#include <memory>

#include "x86_asm_window.hpp"

#if 1

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
   std::unique_ptr<char[]> name(new char[] {"hello, world!"});
   window_asm_x86 w(name.get(), 720, 580);

   return 0;
}

#else

#include "c/window.h"

int main(int argc, char** argv)
{
   Window *w = NULL;
   int r = ht2_initialize_window(L"my title", 0, w);
   return 0;
}

#endif
