#include "platform/Event.h"
#if defined(_WIN32)
#include <windows.h>
#include "Platform_Win32.h"
namespace Euclase {


     KeyCode Win32KeyToKeyCode(WPARAM wParam, LPARAM lParam) {
        switch (wParam) {
            case VK_SPACE: return KeyCode::Space;
            case VK_OEM_7: return KeyCode::Apostrophe;
            case VK_OEM_COMMA: return KeyCode::Comma;
            case VK_OEM_MINUS: return KeyCode::Minus;
            case VK_OEM_PERIOD: return KeyCode::Period;
            case VK_OEM_2: return KeyCode::Slash;

            case '0': return KeyCode::Num0;
            case '1': return KeyCode::Num1;
            case '2': return KeyCode::Num2;
            case '3': return KeyCode::Num3;
            case '4': return KeyCode::Num4;
            case '5': return KeyCode::Num5;
            case '6': return KeyCode::Num6;
            case '7': return KeyCode::Num7;
            case '8': return KeyCode::Num8;
            case '9': return KeyCode::Num9;

            case VK_OEM_1: return KeyCode::Semicolon;
            case VK_OEM_PLUS: return KeyCode::Equal;

            case 'A': return KeyCode::A;
            case 'B': return KeyCode::B;
            case 'C': return KeyCode::C;
            case 'D': return KeyCode::D;
            case 'E': return KeyCode::E;
            case 'F': return KeyCode::F;
            case 'G': return KeyCode::G;
            case 'H': return KeyCode::H;
            case 'I': return KeyCode::I;
            case 'J': return KeyCode::J;
            case 'K': return KeyCode::K;
            case 'L': return KeyCode::L;
            case 'M': return KeyCode::M;
            case 'N': return KeyCode::N;
            case 'O': return KeyCode::O;
            case 'P': return KeyCode::P;
            case 'Q': return KeyCode::Q;
            case 'R': return KeyCode::R;
            case 'S': return KeyCode::S;
            case 'T': return KeyCode::T;
            case 'U': return KeyCode::U;
            case 'V': return KeyCode::V;
            case 'W': return KeyCode::W;
            case 'X': return KeyCode::X;
            case 'Y': return KeyCode::Y;
            case 'Z': return KeyCode::Z;

            case VK_OEM_4: return KeyCode::LeftBracket;
            case VK_OEM_5: return KeyCode::Backslash;
            case VK_OEM_6: return KeyCode::RightBracket;
            case VK_OEM_3: return KeyCode::GraveAccent;

            case VK_ESCAPE: return KeyCode::Escape;
            case VK_RETURN: return KeyCode::Enter;
            case VK_TAB: return KeyCode::Tab;
            case VK_BACK: return KeyCode::Backspace;
            case VK_INSERT: return KeyCode::Insert;
            case VK_DELETE: return KeyCode::Delete;

            case VK_RIGHT: return KeyCode::Right;
            case VK_LEFT: return KeyCode::Left;
            case VK_DOWN: return KeyCode::Down;
            case VK_UP: return KeyCode::Up;
            case VK_PRIOR: return KeyCode::PageUp;
            case VK_NEXT: return KeyCode::PageDown;
            case VK_HOME: return KeyCode::Home;
            case VK_END: return KeyCode::End;

            case VK_CAPITAL: return KeyCode::CapsLock;
            case VK_SCROLL: return KeyCode::ScrollLock;
            case VK_NUMLOCK: return KeyCode::NumLock;
            case VK_SNAPSHOT: return KeyCode::PrintScreen;
            case VK_PAUSE: return KeyCode::Pause;

            case VK_F1: return KeyCode::F1;
            case VK_F2: return KeyCode::F2;
            case VK_F3: return KeyCode::F3;
            case VK_F4: return KeyCode::F4;
            case VK_F5: return KeyCode::F5;
            case VK_F6: return KeyCode::F6;
            case VK_F7: return KeyCode::F7;
            case VK_F8: return KeyCode::F8;
            case VK_F9: return KeyCode::F9;
            case VK_F10: return KeyCode::F10;
            case VK_F11: return KeyCode::F11;
            case VK_F12: return KeyCode::F12;

            case VK_SHIFT: {
                // Bit 16 of lParam contains the scan code.
                const UINT scanCode = (lParam >> 16) & 0xff;

                if (scanCode == MapVirtualKey(VK_RSHIFT, MAPVK_VK_TO_VSC))
                    return KeyCode::RightShift;

                return KeyCode::LeftShift;
            }

            case VK_CONTROL: {
                // Extended bit distinguishes right Ctrl.
                if (lParam & (1LL << 24))
                    return KeyCode::RightControl;

                return KeyCode::LeftControl;
            }

            case VK_MENU: {
                // Extended bit distinguishes right Alt.
                if (lParam & (1LL << 24))
                    return KeyCode::RightAlt;

                return KeyCode::LeftAlt;
            }

            case VK_LWIN: return KeyCode::LeftSuper;
            case VK_RWIN: return KeyCode::RightSuper;

            case VK_APPS: return KeyCode::Menu;

            default:
                return KeyCode::Unknown;
        }
    }

     MouseButton Win32MouseButton(UINT message) {
        switch (message) {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
                return MouseButton::Left;

            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
                return MouseButton::Right;

            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
                return MouseButton::Middle;

            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP: {
                // XBUTTON1/XBUTTON2 are stored in the high word of wParam.
                return MouseButton::X1;
            }

            default:
                return MouseButton::Left;
        }
    }
}
#endif
