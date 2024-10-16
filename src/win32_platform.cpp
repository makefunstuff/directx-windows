#include <d3dcommon.h>
#include <windows.h>
#include <d3d11.h>
#include <stdbool.h>
#include <d3dcompiler.h>

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

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
    ID3D11ShaderResourceView* pTextureRV;
    ID3D11SamplerState* pSamplerLinear;
} SDX11State;

typedef struct {
    float fposition[3];
    float color[4];
    float texCoord[2] ;
} SVertex;

HRESULT LoadTexture(SDX11State* pState, const char* filename)
{
    HRESULT hr = S_OK;

    int width, height, channels;

    unsigned char* img = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

    if (!img) {
        return E_FAIL;
    }

    D3D11_TEXTURE2D_DESC desc = {0};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels =  1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = img;
    subResource.SysMemPitch = width * 4;
    subResource.SysMemSlicePitch = 0;

    ID3D11Texture2D* pTexture = NULL;
    hr = pState->pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    if(FAILED(hr)) {
        stbi_image_free(img);
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    hr = pState->pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &pState->pTextureRV);

    pTexture->Release();

    stbi_image_free(img);

    if(FAILED(hr)) return hr;
    
    return S_OK;
}

HRESULT CreateSamplerState(SDX11State* pState)
{
    HRESULT hr = S_OK;

    D3D11_SAMPLER_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;

    hr = pState->pd3dDevice->CreateSamplerState(&sd, &pState->pSamplerLinear);

    if (FAILED(hr)) return hr;

    return S_OK;
}

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
            pErrorBlob->Release();
        }
        return hr;
    }

    if(pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

HRESULT InitializeTriangle(SDX11State* pState)
{
    HRESULT hr = S_OK;

    SVertex vertices[] = {
        {{0.0f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };

    D3D11_BUFFER_DESC bufferDesc = {0};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(SVertex)*3;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {0};
    initData.pSysMem = vertices;

    hr = pState->pd3dDevice->CreateBuffer(
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
        return hr;
    }

    hr = pState->pd3dDevice->CreateVertexShader(
        psVSBlob->GetBufferPointer(),
        psVSBlob->GetBufferSize(),
        NULL,
        &pState->pVertexShader
    );
    if (FAILED(hr)) {
        psVSBlob->Release();
        return hr;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    UINT numElements = ARRAYSIZE(layout);

    hr = pState->pd3dDevice->CreateInputLayout(
        layout,
        numElements,
        psVSBlob->GetBufferPointer(),
        psVSBlob->GetBufferSize(),
        &pState->pInputLayout
    );
    psVSBlob->Release();
    
    if (FAILED(hr)) return hr;

    ID3DBlob* pPSBlob = NULL;
    hr = CompileShaderFromFile(L"res/shaders/basic_ps.hlsl", "PS_Main", "ps_5_0", &pPSBlob);

    if (FAILED(hr)) {
        MessageBoxW(NULL, L"Could not compile pixel shader", L"Error", MB_OK);
        return hr;
    }

    hr = pState->pd3dDevice->CreatePixelShader(
        pPSBlob->GetBufferPointer(),
        pPSBlob->GetBufferSize(),
        NULL,
        &pState->pPixelShader
    );
    pPSBlob->Release();

    if (FAILED(hr)) return hr;

    return S_OK;
}

void RenderTriangle(SDX11State* pState)
{
    pState->pImmediateContext->IASetInputLayout(pState->pInputLayout);

    UINT uStride = sizeof(SVertex);
    UINT uOffset = 0;

    pState->pImmediateContext->IASetVertexBuffers(
        0,
        1, 
        &pState->pVertexBuffer,
        &uStride,
        &uOffset
    );

    pState->pImmediateContext->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );

    pState->pImmediateContext->VSSetShader(pState->pVertexShader, NULL, 0);
    pState->pImmediateContext->PSSetShader(pState->pPixelShader, NULL, 0);

    pState->pImmediateContext->PSSetShaderResources(0, 1, &pState->pTextureRV);
    pState->pImmediateContext->PSSetSamplers(0, 1, &pState->pSamplerLinear);

    pState->pImmediateContext->Draw(3, 0);
}

void Render(SDX11State* pState)
{
    float fClearColor[4] = {0.0f, 0.125f, 0.3f, 1.0f};

    pState->pImmediateContext->ClearRenderTargetView(pState->pRenderTargetView, fClearColor);

    RenderTriangle(pState);

    pState->pSwapChain->Present(0, 0);
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
    hr = g_dx11State.pSwapChain->GetBuffer(
        0, 
        __uuidof(ID3D11Texture2D), 
        (LPVOID*)&pBackBuffer
    );

    if (FAILED(hr))
        return hr;

    hr = g_dx11State.pd3dDevice->CreateRenderTargetView(
        (ID3D11Resource*)pBackBuffer, 
        NULL, 
        &g_dx11State.pRenderTargetView
    );
    pBackBuffer->Release();
    if (FAILED(hr))
        return hr;

    g_dx11State.pImmediateContext->OMSetRenderTargets(
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
    g_dx11State.pImmediateContext->RSSetViewports(1, &vp);

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

    hr = LoadTexture(&g_dx11State, "res/textures/wall.jpg");

    if (FAILED(hr)) {
        return 1;
    }

    hr = CreateSamplerState(&g_dx11State);

    if (FAILED(hr)) {
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
    if (g_dx11State.pTextureRV) g_dx11State.pTextureRV->Release();
    if (g_dx11State.pSamplerLinear) g_dx11State.pSamplerLinear->Release();
    if (g_dx11State.pVertexBuffer) g_dx11State.pVertexBuffer->Release();
    if (g_dx11State.pInputLayout) g_dx11State.pInputLayout->Release();
    if (g_dx11State.pVertexShader) g_dx11State.pVertexShader->Release();
    if (g_dx11State.pPixelShader) g_dx11State.pPixelShader->Release();
    if (g_dx11State.pRenderTargetView) g_dx11State.pRenderTargetView->Release();
    if (g_dx11State.pSwapChain) g_dx11State.pSwapChain->Release();
    if (g_dx11State.pImmediateContext) g_dx11State.pImmediateContext->Release();
    if (g_dx11State.pd3dDevice) g_dx11State.pd3dDevice->Release();

    return 0;
}
