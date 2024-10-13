#include <d3dcommon.h>
#include <windows.h>
#include <d3d11.h>
#include <stdbool.h>

#define ENGINE_NAME "dx11engine"

typedef struct {
    ID3D11Device* pd3dDevice;
    ID3D11DeviceContext* pImmediateContext;
    IDXGISwapChain* pSwapChain;
    ID3D11RenderTargetView* pRenderTargetView;
} SDX11State;

bool g_IsRunning = true;
SDX11State g_dx11State = {0};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;
    
    switch (uMsg) {
        case WM_CLOSE:
            g_IsRunning = false;
            break;
        default:
            Result = DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return Result;
}

HRESULT InitializeDX11(HWND hwnd)
{
    HRESULT hr = S_OK;
    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT uWidth = rc.right - rc.left;
    UINT uHeight = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef ENABLE_DXDEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = uWidth;
    sd.BufferDesc.Height = uHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;

    hr = D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        createDeviceFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &sd,
        &g_dx11State.pSwapChain,
        &g_dx11State.pd3dDevice,
        &featureLevel,
        &g_dx11State.pImmediateContext
    );

    // TODO(jurip) logging
    if (FAILED(hr)) {
        return hr;
    }
    
    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_dx11State.pSwapChain->lpVtbl->GetBuffer(g_dx11State.pSwapChain, 0, &IID_ID3D11Texture2D, (LPVOID*)&pBackBuffer);
    if (FAILED(hr))
        return hr;

    hr = g_dx11State.pd3dDevice->lpVtbl->CreateRenderTargetView(g_dx11State.pd3dDevice, (ID3D11Resource*)pBackBuffer, NULL, &g_dx11State.pRenderTargetView);
    pBackBuffer->lpVtbl->Release(pBackBuffer);
    if (FAILED(hr))
        return hr;

    g_dx11State.pImmediateContext->lpVtbl->OMSetRenderTargets(g_dx11State.pImmediateContext, 1, &g_dx11State.pRenderTargetView, NULL);

    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)uWidth;
    vp.Height = (FLOAT)uHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_dx11State.pImmediateContext->lpVtbl->RSSetViewports(g_dx11State.pImmediateContext, 1, &vp);

    return S_OK; 
}

int CALLBACK 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    WNDCLASS wc = {0};
    wc.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = ENGINE_NAME;
    
    if (!RegisterClassA(&wc)) {
        // TODO(jurip) add error handling and debug output
        return 1;
    }

    HWND hwnd = CreateWindowExA(
        0,
        ENGINE_NAME,
        ENGINE_NAME,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        // TODO(jurip) add error handling and debug output
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);

    HRESULT hr = InitializeDX11(hwnd);
    if (FAILED(hr)) {
        // TODO(jurip) add error handling and debug output
        return 1;
    }

    while(g_IsRunning) {
        MSG msg = {0};
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_IsRunning = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Clear the back buffer
        float ClearColor[4] = {0.0f, 0.125f, 0.3f, 1.0f}; // RGBA
        g_dx11State.pImmediateContext->lpVtbl->ClearRenderTargetView(g_dx11State.pImmediateContext, g_dx11State.pRenderTargetView, ClearColor);

        // Present the back buffer to the screen
        g_dx11State.pSwapChain->lpVtbl->Present(g_dx11State.pSwapChain, 0, 0);
    }

    // Cleanup
    if (g_dx11State.pRenderTargetView) g_dx11State.pRenderTargetView->lpVtbl->Release(g_dx11State.pRenderTargetView);
    if (g_dx11State.pSwapChain) g_dx11State.pSwapChain->lpVtbl->Release(g_dx11State.pSwapChain);
    if (g_dx11State.pImmediateContext) g_dx11State.pImmediateContext->lpVtbl->Release(g_dx11State.pImmediateContext);
    if (g_dx11State.pd3dDevice) g_dx11State.pd3dDevice->lpVtbl->Release(g_dx11State.pd3dDevice);

    return 0;
}
