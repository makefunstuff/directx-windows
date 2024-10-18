#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE

#include <windows.h>
#include <d3d11_1.h>

#pragma comment(lib, "d3d11.lib")

#include <assert.h>


LRESULT CALLBACK
WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch(uMsg)
    {
        case WM_KEYDOWN:
        {
            if (wParam == VK_ESCAPE)
                DestroyWindow(hWnd);
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            break;
        }
        default:
            result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    return result;
}

int WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nShowCmd
)
{
    HWND hWnd;
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &WndProc;
        wc.hInstance = hInstance;
        wc.hIcon = LoadIconW(0, IDI_APPLICATION);
        wc.hCursor = LoadCursorW(0, IDC_ARROW);
        wc.lpszClassName = L"DxSmplEngine";
        wc.hIconSm = LoadIconW(0, IDI_APPLICATION);

        if (!RegisterClassExW(&wc)) {
            MessageBoxA(0, "RegisterClassEx failed", "Fatal Error", MB_OK);
            return GetLastError();
        }

        RECT rInitialRect = { 0, 0, 1024, 768 };
        AdjustWindowRectEx(
            &rInitialRect,
            WS_OVERLAPPEDWINDOW,
            FALSE, WS_EX_OVERLAPPEDWINDOW
        );

        LONG initialWidth = rInitialRect.right - rInitialRect.left;
        LONG initialHeight = rInitialRect.bottom - rInitialRect.top;

        hWnd = CreateWindowExW(
            WS_EX_OVERLAPPEDWINDOW,
            wc.lpszClassName,
            L"SMPLDX11",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT,
            initialWidth,
            initialHeight,
            0, 0, hInstance, 0
        );

        if (!hWnd) {
            MessageBoxA(0, "CreateWindowEx failed", "Fatal Error", MB_OK);
            return GetLastError();
        }
    }

    ID3D11Device1* d3d11Device;
    ID3D11DeviceContext1* d3d11DeviceContext;
    {
        ID3D11Device* baseDevice;
        ID3D11DeviceContext* baseDeviceContext;

        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(DEBUG_BUILD)
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        HRESULT hResult = D3D11CreateDevice(
            0, D3D_DRIVER_TYPE_HARDWARE,
            0, creationFlags,
            featureLevels, ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION, &baseDevice,
            0, &baseDeviceContext
        );


        if (FAILED(hResult)) {
            MessageBoxA(0, "D3D11CreateDevice() failed", "Fatal Error", MB_OK);
            return GetLastError();
        }

        hResult = baseDevice->QueryInterface(
            __uuidof(ID3D11Device1),
            (void**)&d3d11Device
        );

        assert(SUCCEEDED(hResult));
        baseDevice->Release();


        hResult = baseDeviceContext->QueryInterface(
            __uuidof(ID3D11DeviceContext1),
            (void**) &d3d11DeviceContext
        );

        assert(SUCCEEDED(hResult));
        baseDeviceContext->Release();
    }

#ifdef DEBUG_BUILD
    ID3D11Debug *d3dDebug = nullptr;
    d3d11Device->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug);
    if (d3dDebug)
    {
        ID3D11InfoQueue *d3dInfoQueue = nullptr;
        if (SUCCEEDED(d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue)))
        {
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
            d3dInfoQueue->Release();
        }
        d3dDebug->Release();
    }
#endif
    IDXGISwapChain1* d3d11SwapChain;
    {
        // TODO
    }
}

