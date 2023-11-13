#pragma once
#include <Windows.h>

#pragma comment (lib, "User32.lib")
#pragma comment (lib, "Gdi32.lib") // settextcolor

/*
* -------------------------------------
* x86 Architecture Register Information
* -------------------------------------
*
* // 32, 16, 8, 8
* @ General Registers:
*     eax, ax, ah, al (accumulator)  ->   I/O port access, arithmetic, interrupt calls
*     ebx, bx, bh, bl (base)         ->   base pointer for memory access, recieves interrupt return values
*     ecx, cx, ch, cl (counter)      ->   contains 'this ptr', loop counter, recieves interrupt values
*     edx, dx, dh, dl (data)         ->   i/o port access, arithmetic, interrupt calls
*
* @ Index / Pointer Registers:
*     esi (source index)                ->    string, memory array copying
*     edi (destination index)           ->    string, memory array copying / setting, far pointer addressing with es
*     ebp (stack base pointer)          ->    holds base address of the stack
*     esp (stack pointer)               ->    holds top address of the stack
*     eip (index pointer - read only)   ->    holds offset of the next instruction
*/

class window_asm_x86 {
public:
   __thiscall window_asm_x86(char* name, const int width, const int height)
   {
      __asm {
         // initialize members
         mov eax, [name]
         mov ebx, [width]
         mov edx, [height]

         mov [ecx + 0x0], eax ; m_name
         mov [ecx + 0x4], ebx ; m_width
         mov [ecx + 0x8], edx ; m_height
                     
         xor edx, edx

         mov [ecx + 0xC],  edx ; m_hwnd
         mov [ecx + 0x10], edx ; m_win_class

         // allocate window class pointer
         push ecx                            ; save this ptr
                                             
         push 0x30                           ; push size of window structure (0x30 -> 40 = (4 x 12))
         call malloc                         ; _cdecl, stack alignment required
         xor edx, edx
         mov dl, 0x4
         add esp, edx                        ; re-align stack
                                             
         pop ecx                             ; retrieve this ptr
         mov dword ptr [ecx + 0x10], eax     ; point our window structure to the allocated memory

         // retrieve hinstance
         xor edx, edx
         push edx
         call GetModuleHandleA   ; _stdcall, no stack alignment required

         // initialize window class
         mov ebx, dword ptr [ecx + 0x10]
         
         xor edx, edx
         mov dl,  0x30

         mov dword ptr [ebx + 0x0],  edx          ; size of winclass structure
         mov dword ptr [ebx + 0x4],  CS_CLASSDC 
         
         lea edx, window_procedure                 ; store win_proc in edx
         mov dword ptr [ebx + 0x8], edx            ; copy edx to win_class ptrs win_proc
         xor edx, edx                              ; zero out edx

         mov dword ptr [ebx + 0xC],  edx             
         mov dword ptr [ebx + 0x10], edx             
         mov dword ptr [ebx + 0x14], eax           ; hinstance
         mov dword ptr [ebx + 0x18], edx
         mov dword ptr [ebx + 0x1C], edx

         mov dword ptr [ebx + 0x20], 0
         xor edx, edx

         mov eax, dword ptr [ecx + 0x0]            ; load m_name into eax

         mov dword ptr [ebx + 0x24], eax           ; 
         mov dword ptr [ebx + 0x28], eax           ; m_name
         mov dword ptr [ebx + 0x2C], edx

         // save this ptr with a push and pop
         push ecx

         // push window_class and register it
         push ebx
         call RegisterClassExA            ; _stdcall, no stack alignment required

         // copy this ptr inside of ecx into eax, and push ecx
         pop ecx
         mov esi, ecx ; save this ptr in esi, should not change.

         xor edx, edx

         // create window
         push edx
         push dword ptr [ebx + 0x14]      ; hinstance
         push edx
         push edx
         push dword ptr [ecx + 0x8]       ; m_height
         push dword ptr [ecx + 0x4]       ; m_width

         mov dl, 100
         push edx                         ; y
         push edx                         ; x
         xor edx, edx

         push WS_OVERLAPPEDWINDOW
         push dword ptr [ecx + 0x0]
         push dword ptr [ebx + 0x24]
         push edx
         call CreateWindowExA

         mov dword ptr [esi + 0xC], eax // store hwnd in this->m_hwnd

         // allocate message struct
         xor edx, edx
         mov dl, 0x1C

         push edx
         call malloc

         xor edx, edx
         mov dl,  0x4
         add esp, edx

         mov dword ptr [esi + 0x14], eax ; set msg pointer to allocation

         push SW_SHOWDEFAULT
         push dword ptr [esi + 0xC]
         call ShowWindow

         push dword ptr [esi + 0xC]
         call UpdateWindow

      handle_messages:
         xor edx, edx
         mov dl, 1
         push edx
         call Sleep
         
         xor edx, edx
         push PM_REMOVE
         push edx
         push edx
         push edx
         push dword ptr [esi + 0x14]
         call PeekMessageA

         xor edx, edx
         cmp eax, edx
         je main_loop

         push dword ptr [esi + 0x14]
         call TranslateMessage

         push dword ptr [esi + 0x14]
         call DispatchMessageA

         mov edx, dword ptr [esi + 0x14]        ; set edx to contain the message struct ptr
         cmp dword ptr [edx + 0x4], WM_QUIT     ; check if message contains WM_QUIT
         je end_main                            ; exit application if true

         jmp handle_messages

      main_loop:
         jmp handle_messages

      goodbye_message:
         _emit 'b'
         _emit 'y'
         _emit 'e'
         _emit '!'
         _emit  0
      
      ok_message:
         _emit 'o'
         _emit 'k'
         _emit  0

      end_main:
         push MB_OK
         mov  edx, offset ok_message
         push edx
         mov  edx, offset goodbye_message
         push edx
         push dword ptr 0
         call MessageBoxA

         push dword ptr [esi + 0xC]
         call DestroyWindow

         mov eax, dword ptr [esi + 0x10]
         push [eax + 0x14]
         push [eax + 0x28]
         call UnregisterClassA
      }     
   }

   static inline LRESULT WINAPI window_procedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
   {
      PAINTSTRUCT ps;
      RECT rect;

      ZeroMemory(&ps, sizeof(ps));
      ZeroMemory(&rect, sizeof(rect));

      __asm {
         cmp msg, WM_SIZE
         je handle_size

         cmp msg, WM_DESTROY
         je destroy_window

         cmp msg, WM_PAINT
         je paint_window

         jmp default_condition

      paint_window:
         lea ebx, ps
         push ebx
         push hwnd
         call BeginPaint
         
         mov edx, eax               ; store display handle so we can use it for drawing

         xor ebx, ebx
         cmp eax, ebx
         je default_condition

         // get client area
         push edx                   ; push edx to maintain it after GetClientRect
         
         lea eax, rect
         push eax
         push hwnd
         call GetClientRect

         pop edx                    ; retrieve edx from stack

         mov ebx, edx               ; copy edx into ebx and push it to maintain it after SetTextColor
         push ebx

         // set text color
         xor ebx, ebx
         mov bl, COLOR_INFOTEXT
         push ebx
         push edx
         call SetTextColor

         pop ebx                    ; retrieve edx in ebx
         mov edx, ebx               ; copy it back into edx
         push ebx

         // remove text background color
         xor ebx, ebx
         mov bl, TRANSPARENT
         push ebx
         push edx
         call SetBkMode

         pop ebx
         mov edx, ebx
         push ebx

         // set background color
         xor ebx, ebx
         mov bl, COLOR_BACKGROUND
         push ebx
         lea eax, rect
         push eax
         push edx
         call FillRect

         pop ebx                    ; handle
         mov edx, ebx  
         push edx

         // draw some text
         mov ebx, offset render_text_message
         push ebx
         call strlen
         xor ebx, ebx
         mov bl, 0x4
         add esp, ebx
         mov ebx, eax
         inc ebx                    ; null terminator

         pop edx                    ; handle 

         mov ecx, offset render_text_message
         lea eax, rect
         push DT_CENTER | DT_VCENTER | DT_SINGLELINE
         push eax
         push ebx
         push ecx
         push edx
         call DrawTextA

         lea eax, ps
         push eax
         push hwnd
         call EndPaint

         jmp default_condition

      handle_size:
         cmp wParam, SIZE_MINIMIZED
         je return_zero

         xor eax, eax
         mov al, 0x1
         push eax
         
         xor eax, eax
         push eax

         push hwnd
         call InvalidateRect  ; _stdcall 
         jmp default_condition

      destroy_window:
         xor eax, eax
         push eax
         call PostQuitMessage
         jmp return_zero

      render_text_message :
         _emit 'a'
         _emit 's'
         _emit 'm'
         _emit ' '
         _emit 'x'
         _emit '8'
         _emit '6'
         _emit  0

      default_condition:
         push lParam
         push wParam
         push msg
         push hwnd
         call DefWindowProcA  // maybe have to use A version
         jmp end

      return_zero:
         xor eax, eax

      end:
      }
     
   }


private:
   char* m_name;               // 0x0
   int  m_width, m_height;     // 0x4, 0x8
   HWND m_hwnd;                // 0xC
   WNDCLASSEXA* m_win_class;   // 0x10
   MSG* m_msg;                 // 0x14, 0x1C in size
};
