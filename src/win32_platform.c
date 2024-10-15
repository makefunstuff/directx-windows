#include <d3dcommon.h>
#include <windows.h>
#include <d3d11.h>
#include <stdbool.h>
#include <d3dcompiler.h>

#define ENGINE_NAME "dx11engine"

typedef struct {
    ID3D11Device* pd3dDevice;
    ID3D11DeviceContext* pImmediateContext;
    IDXGISwapChain* pSwapChain;
    ID3D11RenderTargetView* pRenderTargetView;
    ID3D11Buffer* pVertexBuffer;
    ID3D11InputLayout* pInputLayout;
    ID3D11VertexShader* pVertexShader;
    ID3D11PixelShader* pPixelShader;
} SDX11State;

typedef struct {
    float position[3];
    float color[4];
} SVertex;

HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut) 
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
            pErrorBlob->lpVtbl->Release(pErrorBlob);
        }
        return hr;
    }

    if(pErrorBlob) pErrorBlob->lpVtbl->Release(pErrorBlob);

    return S_OK;
}

HRESULT InitializeTriangle(SDX11State* pState)
{
    HRESULT hr = S_OK;

    SVertex vertices[] = {
        {{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}
    };

    D3D11_BUFFER_DESC bufferDesc = {0};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(SVertex)*3;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {0};
    initData.pSysMem = vertices;

    hr = pState->pd3dDevice->lpVtbl->CreateBuffer(
        pState->pd3dDevice,
        &bufferDesc,
        &initData,
        &pState->pVertexBuffer
    );
    if (FAILED(hr)) return hr;

    ID3DBlob* psVSBlob = NULL;
    hr = CompileShaderFromFile(
        L"res/shaders/basic_vs.hlsl",
        "VSMain",
        "vs_5_0",
        &psVSBlob
    );
    if (FAILED(hr)) {
        if(pErrorBlob) {
            OutputDebugStringA((char*)pErrorBlob->lpVtbl->GetBufferPointer(pErrorBlob));
            pErrorBlob->lpVtbl->Release(pErrorBlob);
        }
        return hr;
    }

    hr = pState->pd3dDevice->lpVtbl->CreateVertexShader(
        pState->pd3dDevice,
        psVSBlob->lpVtbl->GetBufferPointer(psVSBlob),
        psVSBlob->lpVtbl->GetBufferSize(psVSBlob),
        NULL,
        &pState->pVertexShader
    );
    if (FAILED(hr)) {
        psVSBlob->lpVtbl->Release(psVSBlob);
        return hr;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    UINT numElements = ARRAYSIZE(layout);

    hr = pState->pd3dDevice->lpVtbl->CreateInputLayout(
        pState->pd3dDevice,
        layout,
        numElements,
        psVSBlob->lpVtbl->GetBufferPointer(psVSBlob),
        psVSBlob->lpVtbl->GetBufferSize(psVSBlob),
        &pState->pInputLayout
    );
    psVSBlob->lpVtbl->Release(psVSBlob);
    
    if (FAILED(hr)) return hr;

    ID3DBlob* pPSBlob = NULL;
    hr = CompileShaderFromFile(L"res/shaders/basic_ps.hlsl", "PS_Main", "ps_5_0", &pPSBlob);

    if (FAILED(hr)) {
        MessageBoxW(NULL, L"Could not compile pixel shader", L"Error", MB_OK);
        return hr;
    }

    hr = pState->pd3dDevice->lpVtbl->CreatePixelShader(
        pState->pd3dDevice,
        pPSBlob->lpVtbl->GetBufferPointer(pPSBlob),
        pPSBlob->lpVtbl->GetBufferSize(pPSBlob),
        NULL,
        &pState->pPixelShader
    );
    pPSBlob->lpVtbl->Release(pPSBlob);

    if (FAILED(hr)) return hr;

    return S_OK;
}

void RenderTriangle(SDX11State* pState)
{
    pState->pImmediateContext->lpVtbl->IASetInputLayout(pState->pImmediateContext, pState->pInputLayout);

    UINT uStride = sizeof(SVertex);
    UINT uOffset = 0;

    pState->pImmediateContext->lpVtbl->IASetVertexBuffers(
        pState->pImmediateContext,
        0,
        1, 
        &pState->pVertexBuffer,
        &uStride,
        &uOffset
    );

    pState->pImmediateContext->lpVtbl->IASetPrimitiveTopology(
        pState->pImmediateContext,
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );

    pState->pImmediateContext->lpVtbl->VSSetShader(pState->pImmediateContext, pState->pVertexShader, NULL, 0);
    pState->pImmediateContext->lpVtbl->PSSetShader(pState->pImmediateContext, pState->pPixelShader, NULL, 0);

    pState->pImmediateContext->lpVtbl->Draw(pState->pImmediateContext, 3, 0);
}

void Render(SDX11State* pState)
{
    float fClearColor[4] = {0.0f, 0.125f, 0.3f, 1.0f};

    pState->pImmediateContext->lpVtbl->ClearRenderTargetView(pState->pImmediateContext, pState->pRenderTargetView, fClearColor);

    RenderTriangle(pState);

    pState->pSwapChain->lpVtbl->Present(pState->pSwapChain, 0, 0);
}


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
    hr = g_dx11State.pSwapChain->lpVtbl->GetBuffer(
        g_dx11State.pSwapChain, 
        0, 
        &IID_ID3D11Texture2D, 
        (LPVOID*)&pBackBuffer
    );

    if (FAILED(hr))
        return hr;

    hr = g_dx11State.pd3dDevice->lpVtbl->CreateRenderTargetView(
        g_dx11State.pd3dDevice, 
        (ID3D11Resource*)pBackBuffer, 
        NULL, 
        &g_dx11State.pRenderTargetView
    );
    pBackBuffer->lpVtbl->Release(pBackBuffer);
    if (FAILED(hr))
        return hr;

    g_dx11State.pImmediateContext->lpVtbl->OMSetRenderTargets(
        g_dx11State.pImmediateContext, 
        1, 
        &g_dx11State.pRenderTargetView, 
        NULL
    );

    D3D11_VIEWPORT vp;
    vp.Width = (float)uWidth;
    vp.Height = (float)uHeight;
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

    hr = InitializeTriangle(&g_dx11State);
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
        
        Render(&g_dx11State);
    }

    // Cleanup

    if (g_dx11State.pVertexBuffer) g_dx11State.pVertexBuffer->lpVtbl->Release(g_dx11State.pVertexBuffer);
    if (g_dx11State.pInputLayout) g_dx11State.pInputLayout->lpVtbl->Release(g_dx11State.pInputLayout);
    if (g_dx11State.pVertexShader) g_dx11State.pVertexShader->lpVtbl->Release(g_dx11State.pVertexShader);
    if (g_dx11State.pPixelShader) g_dx11State.pPixelShader->lpVtbl->Release(g_dx11State.pPixelShader);
    if (g_dx11State.pRenderTargetView) g_dx11State.pRenderTargetView->lpVtbl->Release(g_dx11State.pRenderTargetView);
    if (g_dx11State.pSwapChain) g_dx11State.pSwapChain->lpVtbl->Release(g_dx11State.pSwapChain);
    if (g_dx11State.pImmediateContext) g_dx11State.pImmediateContext->lpVtbl->Release(g_dx11State.pImmediateContext);
    if (g_dx11State.pd3dDevice) g_dx11State.pd3dDevice->lpVtbl->Release(g_dx11State.pd3dDevice);

    return 0;
}
