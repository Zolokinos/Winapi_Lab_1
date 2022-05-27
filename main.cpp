#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <cmath>
#include <string>
#include <vector>

// Modes
#define RECTANGLE 1
#define POLYGON 2
#define LINE 3
#define BROKEN_LINE 4
#define ELLIPSE 5
#define CIRCLE 6
#define SECTOR 7

// Messages
#define WM_SUSPEND_ANIMATION (WM_USER + 1)
#define WM_RESUME_ANIMATION (WM_USER + 2)
#define WM_RESET_ANIMATION (WM_USER + 3)

// Timers
#define STEP_TIMER_ID 1
// TimerState
#define NO_ANIMATION 1
#define ANIMATION 2
#define STOPPED_ANIMATION 3
// Menu
#define ID_START 1
#define IDM_STOP 2
#define IDM_QUIT 3
#define IDM_RESET 4


LRESULT MessagesHandler(
    HWND window_handle, UINT message_code, WPARAM w_param, LPARAM l_param);

HINSTANCE instance_handle;

INT WinMain(HINSTANCE instance_handle_arg, HINSTANCE,
            LPSTR /* command_line */, int n_cmd_show) {
  instance_handle = instance_handle_arg;
  // Register the window class.
  const char kClassName[] = "msg_server_class";
  WNDCLASS window_class = {};
  window_class.lpfnWndProc = MessagesHandler;
  window_class.hInstance = instance_handle;
  window_class.lpszClassName = kClassName;
  window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
  window_class.hbrBackground = CreatePatternBrush((HBITMAP)(
      LoadImage(NULL, R"(C:\Users\Zoly\CLionProjects\dz1\The_end.bmp)", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE)));
  RegisterClass(&window_class);

  // Create the window.
  HWND handle_of_window = CreateWindowEx(
      /* dwExStyle =    */ 0,
      /* lpClassName =  */ kClassName,
      /* lpWindowName = */ "WinAPI Example",
      /* dwStyle =      */ WS_OVERLAPPEDWINDOW,
      /* X =            */ CW_USEDEFAULT,
      /* Y =            */ CW_USEDEFAULT,
      /* nWidth =       */ 1000,
      /* nHeight =      */ 500,
      /* hWndParent =   */ nullptr,
      /* hMenu =        */ nullptr,
      /* hInstance =    */ instance_handle,
      /* lpParam =      */ nullptr
  );
  if (handle_of_window == nullptr) {
    return 1;
  }

  // Show the window.
  ShowWindow(handle_of_window, n_cmd_show);

  // Run the message loop.
  MSG message = {};
  while (GetMessage(&message, nullptr, 0, 0)) {
    TranslateMessage(&message);
    DispatchMessage(&message);
  }

  return (int) message.wParam;
}

HMENU NewPopupMenu() {
  HMENU hMenu = CreatePopupMenu();
  AppendMenu(hMenu, MF_STRING, ID_START, "&Resume");
  AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
  AppendMenu(hMenu, MF_STRING, IDM_STOP, "&Stop");
  AppendMenu(hMenu, MF_STRING, IDM_QUIT, "&Quit");
  return hMenu;
}

HMENU NewPopupMenuReset() {
  HMENU hMenu = CreatePopupMenu();
  AppendMenu(hMenu, MF_STRING, ID_START, "&Resume");
  AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
  AppendMenu(hMenu, MF_STRING, IDM_RESET, "&Reset");
  AppendMenu(hMenu, MF_STRING, IDM_QUIT, "&Quit");
  return hMenu;
}

struct SinglePoint {
  int x, y;
};

struct Line {
  SinglePoint points[2];
  bool is_polygon = false;
  // the start and the end for line
  // the left top and right bottom for rectangle and ellipse in that rectangle
};

struct Poly {
  SinglePoint points[5];
};

struct Circle {
  SinglePoint centre_n_rad[2];
};

struct Sector {
  Circle circle;
  SinglePoint grad;
};


// Drawing a grid & handling keyboard events
LRESULT MessagesHandler(
    HWND window_handle, UINT message_code, WPARAM w_param, LPARAM l_param) {
  static int sx, sy;
  static HBITMAP h_background = NULL;

  static std::vector<Line> lines;
  static std::vector<Line> rectangles;
  static std::vector<Poly> polygons;
  static std::vector<Line> ellipses;
  static std::vector<Circle> circles;
  static std::vector<Sector> sectors;
  static auto it_lines = lines.begin();
  static auto it_rectangles = rectangles.begin();
  static auto it_polygons = polygons.begin();
  static auto it_ellipses = ellipses.begin();
  static auto it_circles = circles.begin();
  static auto it_sectors = sectors.begin();

  static std::vector<SinglePoint> buffer;
  static std::vector<int> queue;

  static int prev_x_ = -1;
  static int prev_y_ = -1;
  static int red = 20;
  static int green = 50;
  static int blue = 100;
  static const int Kred = 20;
  static const int Kgreen = 50;
  static const int Kblue = 100;
  static int temp = 0;

  static int mode = LINE;
  static int timer_mode = NO_ANIMATION;

  static HBRUSH h_brush;
  static HPEN h_pen;

  static HMENU hPopupMenu = NULL;
  static HMENU hPopupMenuReset = NULL;

  static double corner_state = 0;

  switch (message_code) {
    case WM_CREATE: {
      h_brush = CreateSolidBrush(RGB(red, green, blue));
      h_pen = CreatePen(PS_SOLID, 1, RGB(red, green, blue));
      h_background = (HBITMAP)(
          LoadImage(NULL, "C:\\The_end.bmp\0", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE));
      LoadImage(NULL, "C:\\The_end.bmp\0", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
      if (!&GetLastError) {
        PostQuitMessage(228);
      }
//            GetObject(h_background,sizeof(BITMAP),&bitmap);
//            HDC hdc=GetDC(window_handle);
//            HDC hdcMem=CreateCompatibleDC(hdc);
//            HBITMAP hOldBitmap=SelectBitmap(hdcMem, h_background);
//            ReleaseDC(window_handle,hdc);

      InitCommonControls();
      hPopupMenu = NewPopupMenu();
      hPopupMenuReset = NewPopupMenuReset();
      break;
    }
    case WM_GETMINMAXINFO: {
      if (timer_mode != NO_ANIMATION) {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO) l_param;

        RECT rcClientArea;
        GetClientRect(window_handle, &rcClientArea);

        lpMMI->ptMinTrackSize.x = rcClientArea.right - rcClientArea.left + 16;
        lpMMI->ptMinTrackSize.y = rcClientArea.bottom - rcClientArea.top + 40;

        lpMMI->ptMaxTrackSize.x = rcClientArea.right - rcClientArea.left + 16;
        lpMMI->ptMaxTrackSize.y = rcClientArea.bottom - rcClientArea.top + 40;
      } else {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO) l_param;

        lpMMI->ptMinTrackSize.x = 0;
        lpMMI->ptMinTrackSize.y = 0;

        lpMMI->ptMaxTrackSize.x = GetSystemMetrics(SM_CXSCREEN);
        lpMMI->ptMaxTrackSize.y = GetSystemMetrics(SM_CYSCREEN);
      }
      break;
    }
    case WM_SIZE: {
      if (timer_mode != NO_ANIMATION) {
        sx = LOWORD(l_param);
        sy = HIWORD(l_param);
        InvalidateRect(window_handle, nullptr, true);
      }
      break;
    }
    case WM_LBUTTONDOWN: {
      if (w_param & MK_SHIFT) {
        lines.clear();
        rectangles.clear();
        polygons.clear();
        ellipses.clear();
        circles.clear();
        sectors.clear();
        buffer.clear();
        queue = {};
      }
      prev_x_ = LOWORD(l_param);
      prev_y_ = HIWORD(l_param);
      buffer.emplace_back(SinglePoint{prev_x_, prev_y_});

      switch(mode) {
        case RECTANGLE: {
          if (buffer.size() >= 2) {
            rectangles.emplace_back(
                Line{buffer[0],
                     buffer[1]});
            buffer.clear();
            queue.push_back(RECTANGLE);
          }
          break;
        }
        case POLYGON: {
          if (buffer.size() >= 5) {
            polygons.emplace_back(
                Poly{buffer[0],
                     buffer[1],
                     buffer[2],
                     buffer[3],
                     buffer[4]});
            queue.push_back(POLYGON);
            buffer.clear();
          }
          if (buffer.size() >= 2) {
            lines.emplace_back(
                Line{buffer[buffer.size() - 2],
                     buffer[buffer.size() - 1],
                     true});
            queue.push_back(LINE);
          }
          break;
        }
        case LINE: {
          if (buffer.size() >= 2) {
            lines.emplace_back(
                Line{buffer[0],
                     buffer[1]});
            buffer.clear();
            queue.push_back(LINE);
          }
          break;
        }
        case BROKEN_LINE: {
          if (buffer.size() >= 2) {
            lines.emplace_back(
                Line{buffer[buffer.size() - 2],
                     buffer[buffer.size() - 1]});
            queue.push_back(LINE);
          }
          break;
        }
        case ELLIPSE: {
          if (buffer.size() >= 2) {
            ellipses.emplace_back(
                Line{buffer[0],
                     buffer[1]});
            queue.push_back(ELLIPSE);
            buffer.clear();
          }
          break;
        }
        case CIRCLE: {
          if (buffer.size() >= 2) {
            circles.emplace_back(
                Circle{buffer[0],
                       buffer[1]});
            queue.push_back(CIRCLE);
            buffer.clear();
          }
          break;
        }
        case SECTOR: {
          if (buffer.size() >= 3) {
            sectors.emplace_back(
                Sector{
                    Circle{
                        buffer[0],
                        buffer[1]
                    },
                    buffer[2]
                });
            queue.push_back(SECTOR);
            buffer.clear();
          }
          break;
        }
      }
      InvalidateRect(window_handle, nullptr, true);
      break;
    }
    case WM_RBUTTONUP: {
      // Get mouse click coordinates.
      POINT point;
      point.x = LOWORD(l_param);
      point.y = HIWORD(l_param);

      // Convert the client coordinates to screen coordinates.
      ClientToScreen(window_handle, &point);

      // Show popup menu.
      if (timer_mode == STOPPED_ANIMATION) {
        TrackPopupMenu(
            hPopupMenuReset, TPM_RIGHTBUTTON, point.x, point.y, 0, window_handle, NULL);

      } else {
        TrackPopupMenu(
            hPopupMenu, TPM_RIGHTBUTTON, point.x, point.y, 0, window_handle, NULL);
      }
      break;
    }
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(window_handle, &ps);

      it_lines = lines.begin();
      it_rectangles = rectangles.begin();
      it_polygons = polygons.begin();
      it_ellipses = ellipses.begin();
      it_circles = circles.begin();
      it_sectors = sectors.begin();
      temp = 0;

      RECT rcClientArea;
      HDC hdcBuffer;
      HBITMAP hbmBuffer, hbmOld;

      // Get the size of the client rectangle.
      GetClientRect(window_handle, &rcClientArea);

      // Create a compatible DC.
      hdcBuffer = CreateCompatibleDC(ps.hdc);

      // Create a bitmap big enough for our client rectangle.
      hbmBuffer = CreateCompatibleBitmap(
          ps.hdc,
          rcClientArea.right - rcClientArea.left,
          rcClientArea.bottom - rcClientArea.top);

      // Select the bitmap into the off-screen DC.
      hbmOld = static_cast<HBITMAP>(SelectObject(hdcBuffer, hbmBuffer));

      // Erase the h_background.
      HBRUSH hbrBkGnd = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
      // HBRUSH hbrBkGnd = CreatePatternBrush(h_background);
      FillRect(hdcBuffer, &rcClientArea, hbrBkGnd);
      DeleteObject(hbrBkGnd);

      //            // loading bitmap from file:
      //            HBITMAP hBmp = (HBITMAP)LoadImage(NULL, "C:\\The_end.bmp\0", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
      //            // creating memory DC for this bitmap:
      //            HDC bmpDC = CreateCompatibleDC(ps.hdc);
      //            SelectObject(bmpDC, hBmp);
      //            // output (wndDC - HDC window):
      //            BitBlt(ps.hdc, 0, 0, rcClientArea.right - rcClientArea.left, rcClientArea.bottom - rcClientArea.top, bmpDC, 0, 0, SRCCOPY);
      //            // releasing resources:
      //            DeleteDC(bmpDC);
      //            DeleteObject(hBmp);

      for(const auto part : queue) {
        switch(part) {
          case RECTANGLE: {
            DeleteObject(h_brush);
            h_brush = CreateHatchBrush(
                HS_FDIAGONAL, RGB(
                    red % 256,
                    (green + temp) % 256,
                    (blue + temp * 2 / 3) % 256));
            DeleteObject(h_pen);
            h_pen = CreatePen(
                PS_SOLID, 1,  RGB(
                    red % 256,
                    (green + temp) % 256,
                    (blue + temp * 2 / 3) % 256));
            temp += 15;
            temp %= 256;

            SelectObject(hdcBuffer, h_brush);
            SelectObject(hdcBuffer, h_pen);

            Rectangle(hdcBuffer, it_rectangles->points[0].x, it_rectangles->points[0].y,
                      it_rectangles->points[1].x, it_rectangles->points[1].y);

            ++it_rectangles;
            break;
          }
          case POLYGON: {
            DeleteObject(h_brush);
            h_brush = CreateSolidBrush(RGB(
                (red + temp) % 256,
                (green + temp * 2 / 3) % 256,
                (blue + temp * 1 / 3) % 256));
            DeleteObject(h_pen);
            h_pen = CreatePen(PS_SOLID, 1, RGB(
                (red + temp) % 256,
                (green + temp * 2 / 3) % 256,
                (blue + temp * 1 / 3) % 256));
            temp += 30;
            temp %= 256;

            BeginPath(hdcBuffer);

            POINT poly[5];
            for(int i = 0; i < 5; ++i) {
              poly[i].x = it_polygons->points[i].x;
              poly[i].y = it_polygons->points[i].y;
            }

            SelectObject(hdcBuffer, h_brush);
            SelectObject(hdcBuffer, h_pen);

            Polyline(hdcBuffer, poly, 5);
            CloseFigure(hdcBuffer); // Closes an open figure in a path.
            EndPath(hdcBuffer);

            SetPolyFillMode(hdcBuffer, WINDING);
            FillPath(hdcBuffer);

            ++it_polygons;
            break;
          }
          case LINE: {
            DeleteObject(h_pen);
            if(it_lines->is_polygon) {
              h_pen = CreatePen(PS_SOLID, 1, RGB(
                  (red + temp) % 256,
                  (green + temp * 2 / 3) % 256,
                  (blue + temp * 1 / 3) % 256));
            } else {
              h_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
            }

            SelectObject(hdcBuffer, h_pen);

            MoveToEx(hdcBuffer, it_lines->points[0].x, it_lines->points[0].y, nullptr);
            LineTo(hdcBuffer, it_lines->points[1].x, it_lines->points[1].y);
            ++it_lines;
            break;
          }
          case ELLIPSE: {
            DeleteObject(h_brush);
            h_brush = CreateHatchBrush(
                HS_CROSS, RGB((red + temp) % 256, (green - temp / 2) % 256, (blue + temp / 2) % 256));
            DeleteObject(h_pen);
            h_pen = CreatePen(PS_SOLID, 2, RGB((red + temp) % 256, (green - temp / 2) % 256, (blue + temp / 2) % 256));
            temp += 20;
            temp %= 256;

            SelectObject(hdcBuffer, h_brush);
            SelectObject(hdcBuffer, h_pen);

            Ellipse(hdcBuffer, it_ellipses->points[0].x, it_ellipses->points[0].y,
                    it_ellipses->points[1].x, it_ellipses->points[1].y);

            ++it_ellipses;
            break;
          }
          case CIRCLE: {
            DeleteObject(h_brush);
            h_brush = CreateHatchBrush(
                HS_CROSS, RGB((red + temp) % 256, (green - temp / 2) % 256, (blue + temp / 2) % 256));
            DeleteObject(h_pen);
            h_pen = CreatePen(PS_SOLID, 2, RGB((red + temp) % 256, (green - temp / 2) % 256, (blue + temp / 2) % 256));
            temp += 20;
            temp %= 256;

            SelectObject(hdcBuffer, h_brush);
            SelectObject(hdcBuffer, h_pen);

            int radius = sqrt((it_circles->centre_n_rad[0].x - it_circles->centre_n_rad[1].x) *
                (it_circles->centre_n_rad[0].x - it_circles->centre_n_rad[1].x) +
                (it_circles->centre_n_rad[0].y - it_circles->centre_n_rad[1].y) *
                    (it_circles->centre_n_rad[0].y - it_circles->centre_n_rad[1].y));

            Ellipse(hdcBuffer, it_circles->centre_n_rad[0].x - radius,
                    it_circles->centre_n_rad[0].y - radius,
                    it_circles->centre_n_rad[0].x + radius,
                    it_circles->centre_n_rad[0].y + radius);

            ++it_circles;
            break;
          }
          case SECTOR: {
            // Draw the Pie.
            DeleteObject(h_brush);
            h_brush = CreateHatchBrush(
                HS_DIAGCROSS, RGB(
                    (red + 4 * temp) % 256,
                    (green - temp / 2) % 256,
                    (blue - temp / 3) % 256));
            DeleteObject(h_pen);
            h_pen = CreatePen(
                PS_SOLID, 2, RGB(
                    (red + 4 * temp) % 256,
                    (green - temp / 2) % 256,
                    (blue - temp / 3) % 256));
            temp += 35;
            temp %= 256;

            SelectObject(hdcBuffer, h_brush);
            SelectObject(hdcBuffer, h_pen);
            int radius = sqrt((it_sectors->circle.centre_n_rad[0].x - it_sectors->circle.centre_n_rad[1].x) *
                (it_sectors->circle.centre_n_rad[0].x - it_sectors->circle.centre_n_rad[1].x) +
                (it_sectors->circle.centre_n_rad[0].y - it_sectors->circle.centre_n_rad[1].y) *
                    (it_sectors->circle.centre_n_rad[0].y - it_sectors->circle.centre_n_rad[1].y));

            double alpha = acos((it_sectors->circle.centre_n_rad[1].x
                - it_sectors->circle.centre_n_rad[0].x) / (double) radius) + corner_state;

            Pie(hdcBuffer, it_sectors->circle.centre_n_rad[0].x - radius,
                it_sectors->circle.centre_n_rad[0].y - radius,
                it_sectors->circle.centre_n_rad[0].x + radius,
                it_sectors->circle.centre_n_rad[0].y + radius,
                cos(alpha) * static_cast<double>(radius) + it_sectors->circle.centre_n_rad[0].x,
                sin(alpha) * static_cast<double>(radius) + it_sectors->circle.centre_n_rad[0].y,
                it_sectors->grad.x,
                it_sectors->grad.y);

            Pie(hdc, it_sectors->circle.centre_n_rad[0].x - radius,
                it_sectors->circle.centre_n_rad[0].y - radius,
                it_sectors->circle.centre_n_rad[0].x + radius,
                it_sectors->circle.centre_n_rad[0].y + radius,
                it_sectors->circle.centre_n_rad[1].x,
                it_sectors->circle.centre_n_rad[1].y,
                it_sectors->grad.x,
                it_sectors->grad.y);

            ++it_sectors;
            break;
          }
          default: {
            PostQuitMessage(42);
          }
        }
      }

      if (temp >= 256 || temp < 0) {
        PostQuitMessage(2);
      }
      SelectBrush(hdcBuffer, GetStockObject(WHITE_BRUSH));
      SelectPen(hdcBuffer, GetStockObject(BLACK_PEN));
      std::wstring label_text_1 =
          L"r- rectangles, p- polygons";
      TextOutW(hdcBuffer, 0, 0, (label_text_1.c_str()), label_text_1.length());

      std::wstring label_text_2 =
          L"l- line segment, b- broken line";
      TextOutW(hdcBuffer, 0, 16, (label_text_2.c_str()), label_text_2.length());

      std::wstring label_text_3 =
          L"e- ellipses, c- circles, s- sectors";
      TextOutW(hdcBuffer, 0, 32, (label_text_3.c_str()), label_text_3.length());

      // draw circles
      if(!buffer.empty()) {
        SelectBrush(hdcBuffer, GetStockObject(WHITE_BRUSH));
        DeleteObject(h_pen);
        h_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        SelectObject(hdcBuffer, h_pen);
        Ellipse(hdcBuffer, prev_x_ - 5, prev_y_ - 5, prev_x_ + 5, prev_y_ + 5);
      }

      // Bit-block transfer to the screen DC.
      BitBlt(ps.hdc,
             rcClientArea.left,
             rcClientArea.top,
             rcClientArea.right - rcClientArea.left,
             rcClientArea.bottom - rcClientArea.top,
             hdcBuffer,
             0,
             0,
             SRCCOPY);

      // Done with off-screen bitmap and DC.
      SelectObject(hdcBuffer, hbmOld);
      DeleteObject(hbmBuffer);
      DeleteDC(hdcBuffer);


      EndPaint(window_handle, &ps);
      break;
    }
    case WM_DESTROY: {
      DeleteObject(h_brush);
      DeleteObject(h_pen);
      DeleteObject(hPopupMenu);
      DeleteObject(hPopupMenuReset);
      PostQuitMessage(0);
      break;
    }
    case WM_KEYDOWN: {
      switch(w_param) {
        case 'R': {
          mode = RECTANGLE;
          buffer.clear();
          InvalidateRect(window_handle, nullptr, true);
          break;
        }
        case 'P': {
          mode = POLYGON;
          buffer.clear();
          InvalidateRect(window_handle, nullptr, true);
          break;
        }
        case 'L': {
          mode = LINE;
          if (!buffer.empty() && !lines.empty() && lines.back().is_polygon) {
            while (!lines.empty() && lines.back().is_polygon) {
              queue.pop_back();
              lines.pop_back();
            }
          }
          buffer.clear();
          InvalidateRect(window_handle, nullptr, true);
          break;
        }
        case 'B': {
          mode = BROKEN_LINE;
          buffer.clear();
          InvalidateRect(window_handle, nullptr, true);
          break;
        }
        case 'E': {
          mode = ELLIPSE;
          buffer.clear();
          InvalidateRect(window_handle, nullptr, true);
          break;
        }
        case 'C': {
          mode = CIRCLE;
          buffer.clear();
          InvalidateRect(window_handle, nullptr, true);
          break;
        }
        case 'S': {
          mode = SECTOR;
          buffer.clear();
          InvalidateRect(window_handle, nullptr, true);
          break;
        }
        default: {
          return DefWindowProc(window_handle, message_code, w_param, l_param);
        }
      }
      break;
    }

    case WM_COMMAND: {
      if (l_param == 0) {
        switch (LOWORD(w_param)) {
          case ID_START: {
            timer_mode = ANIMATION;
            SendMessage(window_handle, WM_RESUME_ANIMATION, 0, 0);
            break;
          }
          case IDM_STOP: {
            timer_mode = STOPPED_ANIMATION;
            SendMessage(window_handle, WM_SUSPEND_ANIMATION, 0, 0);
            break;
          }
          case IDM_RESET: {
            timer_mode = NO_ANIMATION;
            SendMessage(window_handle, WM_RESET_ANIMATION, 0, 0);
            break;
          }
          case IDM_QUIT: {
            PostQuitMessage(0);
            break;
          }
        }
      } else {
        return DefWindowProc(window_handle, message_code, w_param, l_param);
      }
      break;
    }
    case WM_SUSPEND_ANIMATION: {
      KillTimer(window_handle, STEP_TIMER_ID);
      break;
    }
    case WM_RESUME_ANIMATION: {
      SetTimer(window_handle, STEP_TIMER_ID, 30 /* millis */, NULL);
      break;
    }
    case WM_RESET_ANIMATION: {
      red = Kred;
      green = Kgreen;
      blue = Kblue;
      temp = 0;
      corner_state = 0;
      InvalidateRect(window_handle, nullptr, true);
      break;
    }
    case WM_TIMER: {
      static int tick = 0;
      ++tick;
      tick %= 628;
      red = Kred + 15 * tick;
      red = red % 256;
      green = Kgreen -5 * tick;
      green = green % 256;
      while (green < 0) {
        green += 256;
      }
      blue = Kblue -10 * tick;
      blue = blue % 256;
      while (blue < 0) {
        blue += 256;
      }

      corner_state += 0.01;

      // Initiate repainting
      InvalidateRect(window_handle, nullptr, true);
      break;
    }
    default: {
      return DefWindowProc(window_handle, message_code, w_param, l_param);
    }
  }
  return 0;
}
