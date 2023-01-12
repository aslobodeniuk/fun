#include <SDKDDKVer.h>
#include <windows.h>

#include <gst/gst.h>
#include <gst/video/videooverlay.h>

static struct {
    HWND hwnd;
    GstElement* pipe;
    GThread* thr;
    gboolean stopping;
} fixture;

gpointer fixture_thread(gpointer)
{
    int length = 0;
    int a = 0;
    while (!fixture.stopping) {
        if ((length += 10) > 40) {
            length = 0;
            PostMessageA(fixture.hwnd, SW_SHOW, 0, 0);
            if (++a > 1) {
                gst_element_set_state(fixture.pipe, GST_STATE_NULL);
                gst_element_set_state(fixture.pipe, GST_STATE_PLAYING);
                a = 0;
            }
        }

        if (!fixture.stopping)
          MoveWindow(
            fixture.hwnd,
            0,
            0,
            length,
            length,
            1
        );
        g_usleep(100);
    }

    return NULL;
}

#define IDC_D3D11VIDEOSINKTEST			109

// Global Variables:
HINSTANCE hInst;                                // current instance

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_D3D11VIDEOSINKTEST));

    MSG msg;
    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = NULL;
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = L"something";
    wcex.lpszClassName  = L"mywindowclass";
    wcex.hIconSm        = NULL;

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   gst_init(NULL, NULL);
   GstBus* bus;
   fixture.pipe = gst_parse_launch("videotestsrc ! queue ! d3d11videosink", NULL);
   bus = gst_element_get_bus(fixture.pipe);
   

   gst_bus_set_sync_handler(bus,
       (GstBusSyncHandler)+[](GstBus* bus, GstMessage* message, gpointer) -> GstBusSyncReply
       {
           if (GST_MESSAGE_TYPE(message) != GST_MESSAGE_ELEMENT ||
               !gst_is_video_overlay_prepare_window_handle_message(message)) {
               return GST_BUS_PASS;
           }

           gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(message)), (guintptr)fixture.hwnd);

           return GST_BUS_DROP;
       },
       NULL, NULL);
   gst_object_unref(bus);

   HWND hWnd = CreateWindowW(L"mywindowclass", L"d3d11videosink test", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   fixture.hwnd = hWnd;
   gst_element_set_state(fixture.pipe, GST_STATE_PLAYING);

   fixture.thr = g_thread_new(NULL, fixture_thread, NULL);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        fixture.stopping = TRUE;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    g_thread_join(fixture.thr);
    gst_object_unref(fixture.pipe);
    return 0;
}
