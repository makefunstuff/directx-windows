#include <sal.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")

#include <assert.h>

#define global static
#define internal static

global bool g_bWindowDidResize = false;

typedef struct {
    ID3D11Device1* pd3d11Device;
    ID3D11DeviceContext1* pd3d11DeviceContext;
    IDXGISwapChain1* pd3d11SwapChain;
    ID3D11RenderTargetView* pd3d11FrameBufferView;
} SD3D11_State;

typedef struct {
    ID3D11PixelShader* ppixelShader;
    ID3D11VertexShader* pvertexShader;
    ID3D11InputLayout* pinputLayout;
} SD3D11_ShaderState;


global SD3D11_State g_d3d11State;
global SD3D11_ShaderState g_d3d11TriangleShaderState;

HRESULT
CompileShaderFromFile(
    WCHAR* szFileName, LPCSTR szEntryPoint,
    LPCSTR szShaderModel, ID3DBlob** ppBlobOut
)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef ENABLE_DXDEBUG
    dwShaderFlags |= D3DCOMPILE_DEBUG;
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = NULL;
    hr = D3DCompileFromFile(
        szFileName,
        NULL, NULL,
        szEntryPoint,
        szShaderModel,
        dwShaderFlags,
        0,
        ppBlobOut,
        &pErrorBlob
    );


    if (FAILED(hr)) {
        if(pErrorBlob) {
            pErrorBlob->Release();
        }
        return hr;
    }

    if(pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

void
D3D11InitState(SD3D11_State* pd3d11State)
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
    }

    hResult = baseDevice->QueryInterface(
        __uuidof(ID3D11Device1),
        (void**)&pd3d11State->pd3d11Device
    );

    assert(SUCCEEDED(hResult));
    baseDevice->Release();


    hResult = baseDeviceContext->QueryInterface(
        __uuidof(ID3D11DeviceContext1),
        (void**) &pd3d11State->pd3d11DeviceContext
    );

    assert(SUCCEEDED(hResult));
    baseDeviceContext->Release();
}


void
D3D11InitSwapChain(SD3D11_State* pd3d11State, HWND hWnd)
{
    IDXGIFactory2 *dxgiFactory;
    {
        IDXGIDevice1* dxgiDevice;
        HRESULT hResult = pd3d11State->pd3d11Device->QueryInterface(
            __uuidof(IDXGIDevice1), (void**)&dxgiDevice
        );

        assert(SUCCEEDED(hResult));

        IDXGIAdapter* dxgiAdapter;
        hResult = dxgiDevice->GetAdapter(&dxgiAdapter);
        assert(SUCCEEDED(hResult));
        dxgiDevice->Release();

        DXGI_ADAPTER_DESC adapterDesc;
        dxgiAdapter->GetDesc(&adapterDesc);

        OutputDebugStringA("Graphics Device: ");
        OutputDebugStringW(adapterDesc.Description);

        hResult = dxgiAdapter->GetParent(
            __uuidof(IDXGIFactory2), (void**)&dxgiFactory
        );
        assert(SUCCEEDED(hResult));
        dxgiAdapter->Release();
    }

    DXGI_SWAP_CHAIN_DESC1 d3d11SwapChainDesc = {};
    d3d11SwapChainDesc.Width = 0;
    d3d11SwapChainDesc.Height = 0;
    d3d11SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    d3d11SwapChainDesc.SampleDesc.Count = 1;
    d3d11SwapChainDesc.SampleDesc.Quality = 0;
    d3d11SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    d3d11SwapChainDesc.BufferCount = 2;
    d3d11SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    d3d11SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    d3d11SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    d3d11SwapChainDesc.Flags = 0;

    HRESULT hResult = dxgiFactory->CreateSwapChainForHwnd(
        pd3d11State->pd3d11Device,
        hWnd,
        &d3d11SwapChainDesc,
        0, 0, &pd3d11State->pd3d11SwapChain
    );

    assert(SUCCEEDED(hResult));
    dxgiFactory->Release();
}

void
D3D11InitFrameBuffer(SD3D11_State* pd3d11State)
{
    ID3D11Texture2D* d3d11FrameBuffer;
    HRESULT hResult = pd3d11State->pd3d11SwapChain->GetBuffer(
        0, __uuidof(ID3D11Texture2D),
        (void**)&d3d11FrameBuffer
    );

    assert(SUCCEEDED(hResult));

    pd3d11State->pd3d11Device->CreateRenderTargetView(
        d3d11FrameBuffer, 0,
        &pd3d11State->pd3d11FrameBufferView
    );

    assert(SUCCEEDED(hResult));
    d3d11FrameBuffer->Release();
}

HRESULT
InitTriangleShaderState(
    SD3D11_State* pState, SD3D11_ShaderState* pShaderState,
    ID3DBlob*vsBlob
)
{
    HRESULT hr = S_OK;

    ID3DBlob* psBlob;

    // vertex shader
    hr = CompileShaderFromFile(
        L"res/shaders/basic_vs.hlsl", "VSMain",
        "vs_5_0", &vsBlob
    );

    if (FAILED(hr)) {
        return hr;
    }

    hr = pState->pd3d11Device->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        NULL,
        &pShaderState->pvertexShader
    );

    if (FAILED(hr)) {
        vsBlob->Release();
        return hr;
    }

    // pixel shader
    hr = CompileShaderFromFile(
        L"res/shaders/basic_ps.hlsl",
        "PS_Main", "ps_5_0", &psBlob
    );

    pState->pd3d11Device->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        NULL, &pShaderState->ppixelShader
    );

    if (FAILED(hr)) {
        psBlob->Release();
        return hr;
    }
    psBlob->Release();

    return S_OK;
}

void
CreateTriangleInputLayout(SD3D11_State* pState, SD3D11_ShaderState* pShaderState, ID3DBlob* vsBlob)
{
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };

    HRESULT hr = pState->pd3d11Device->CreateInputLayout(
        inputElementDesc,
        ARRAYSIZE(inputElementDesc),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &pShaderState->pinputLayout
    );

    assert(SUCCEEDED(hr));
}

void
CreateTriangleVertexBuffer()
{
  // TODO
}

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
        wc.lpszClassName = L"MyWindowClass";
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

    D3D11InitState(&g_d3d11State);
    D3D11InitSwapChain(&g_d3d11State, hWnd);
    D3D11InitFrameBuffer(&g_d3d11State);

    bool isRunning = true;
    while(isRunning)
    {
        MSG msg = {};
        while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) {
                isRunning = false;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        FLOAT backgroundColor[4] = { 0.1f, 0.2f, 0.6f, 1.0f };
        g_d3d11State.pd3d11DeviceContext->ClearRenderTargetView(g_d3d11State.pd3d11FrameBufferView, backgroundColor);
        g_d3d11State.pd3d11SwapChain->Present(1, 0);
    }
}

