#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

// DirectX objects
ID3D11Device* device = NULL;
ID3D11DeviceContext* deviceContext = NULL;
IDXGISwapChain* swapChain = NULL;
ID3D11RenderTargetView* renderTargetView = NULL;
ID3D11Buffer* vertexBuffer = NULL;
ID3D11InputLayout* inputLayout = NULL;
ID3D11VertexShader* vertexShader = NULL;
ID3D11PixelShader* pixelShader = NULL;

// Window dimensions
const int WIDTH = 800;
const int HEIGHT = 600;

void CleanupD3D() {
    if (vertexBuffer) vertexBuffer->lpVtbl->Release(vertexBuffer);
    if (inputLayout) inputLayout->lpVtbl->Release(inputLayout);
    if (vertexShader) vertexShader->lpVtbl->Release(vertexShader);
    if (pixelShader) pixelShader->lpVtbl->Release(pixelShader);
    if (renderTargetView) renderTargetView->lpVtbl->Release(renderTargetView);
    if (swapChain) swapChain->lpVtbl->Release(swapChain);
    if (deviceContext) deviceContext->lpVtbl->Release(deviceContext);
    if (device) device->lpVtbl->Release(device);
}

int InitD3D(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC scd = { 0 };
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.BufferDesc.Width = WIDTH;
    scd.BufferDesc.Height = HEIGHT;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    // Create device, device context, and swap chain
    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0,
                      D3D11_SDK_VERSION, &scd, &swapChain, &device, NULL, &deviceContext);
    if (FAILED(hr)) {
        MessageBox(hwnd, L"Failed to create device and swap chain", L"Error", MB_OK);
        return 0;
    }

    // Get the back buffer and create render target view
    ID3D11Texture2D* backBuffer = NULL;
    hr = swapChain->lpVtbl->GetBuffer(swapChain, 0, &IID_ID3D11Texture2D, (LPVOID*)&backBuffer);
    if (FAILED(hr)) {
        MessageBox(hwnd, L"Failed to get back buffer", L"Error", MB_OK);
        return 0;
    }

    hr = device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)backBuffer, NULL, &renderTargetView);
    backBuffer->lpVtbl->Release(backBuffer);
    if (FAILED(hr)) {
        MessageBox(hwnd, L"Failed to create render target view", L"Error", MB_OK);
        return 0;
    }

    // Set the render target and viewport
    deviceContext->lpVtbl->OMSetRenderTargets(deviceContext, 1, &renderTargetView, NULL);

    D3D11_VIEWPORT viewport = { 0 };
    viewport.Width = (float)WIDTH;
    viewport.Height = (float)HEIGHT;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    deviceContext->lpVtbl->RSSetViewports(deviceContext, 1, &viewport);

    return 1;
}

struct Vertex {
    float position[3];
    float color[4];
};

int InitTriangle() {
    struct Vertex vertices[] = {
        { { 0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
    };

    D3D11_BUFFER_DESC bufferDesc = { 0 };
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = { 0 };
    initData.pSysMem = vertices;

    if (FAILED(device->lpVtbl->CreateBuffer(device, &bufferDesc, &initData, &vertexBuffer))) {
        MessageBox(NULL, L"Failed to create vertex buffer", L"Error", MB_OK);
        return 0;
    }

    return 1;
}

const char* vertexShaderSource = 
    "struct VS_INPUT { float4 Pos : POSITION; float4 Color : COLOR; };"
    "struct PS_INPUT { float4 Pos : SV_POSITION; float4 Color : COLOR; };"
    "PS_INPUT VSMain(VS_INPUT input) {"
    "    PS_INPUT output;"
    "    output.Pos = input.Pos;"
    "    output.Color = input.Color;"
    "    return output;"
    "}";

const char* pixelShaderSource = 
    "struct PS_INPUT { float4 Pos : SV_POSITION; float4 Color : COLOR; };"
    "float4 PSMain(PS_INPUT input) : SV_TARGET {"
    "    return input.Color;"
    "};";

int InitShaders() {
    ID3DBlob* vsBlob = NULL;
    ID3DBlob* psBlob = NULL;
    ID3DBlob* errorBlob = NULL;

    // Compile vertex shader
    HRESULT hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), NULL, NULL, NULL, 
                            "VSMain", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            MessageBoxA(NULL, (char*)errorBlob->lpVtbl->GetBufferPointer(errorBlob), "Vertex Shader Error", MB_OK);
            errorBlob->lpVtbl->Release(errorBlob);
        }
        return 0;
    }

    hr = device->lpVtbl->CreateVertexShader(device, vsBlob->lpVtbl->GetBufferPointer(vsBlob),
                                            vsBlob->lpVtbl->GetBufferSize(vsBlob), NULL, &vertexShader);
    if (FAILED(hr)) {
        vsBlob->lpVtbl->Release(vsBlob);
        return 0;
    }

    // Compile pixel shader
    hr = D3DCompile(pixelShaderSource, strlen(pixelShaderSource), NULL, NULL, NULL,
                    "PSMain", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            MessageBoxA(NULL, (char*)errorBlob->lpVtbl->GetBufferPointer(errorBlob), "Pixel Shader Error", MB_OK);
            errorBlob->lpVtbl->Release(errorBlob);
        }
        return 0;
    }

    hr = device->lpVtbl->CreatePixelShader(device, psBlob->lpVtbl->GetBufferPointer(psBlob),
                                           psBlob->lpVtbl->GetBufferSize(psBlob), NULL, &pixelShader);
    if (FAILED(hr)) {
        psBlob->lpVtbl->Release(psBlob);
        return 0;
    }

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = device->lpVtbl->CreateInputLayout(device, layout, 2, vsBlob->lpVtbl->GetBufferPointer(vsBlob),
                                           vsBlob->lpVtbl->GetBufferSize(vsBlob), &inputLayout);
    
    vsBlob->lpVtbl->Release(vsBlob);
    psBlob->lpVtbl->Release(psBlob);
    
    if (FAILED(hr)) {
        return 0;
    }

    return 1;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"DirectX Window Class";

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Calculate window size based on desired client area
    RECT wr = { 0, 0, WIDTH, HEIGHT };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"DirectX Triangle",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Initialize DirectX
    if (!InitD3D(hwnd)) {
        MessageBox(hwnd, L"DirectX Initialization Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Initialize triangle and shaders
    if (!InitTriangle() || !InitShaders()) {
        MessageBox(hwnd, L"Failed to initialize triangle or shaders!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        CleanupD3D();
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CleanupD3D();
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
            if (deviceContext && renderTargetView) {
                float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
                deviceContext->lpVtbl->ClearRenderTargetView(deviceContext, renderTargetView, clearColor);

                UINT stride = sizeof(struct Vertex);
                UINT offset = 0;
                deviceContext->lpVtbl->IASetVertexBuffers(deviceContext, 0, 1, &vertexBuffer, &stride, &offset);
                deviceContext->lpVtbl->IASetInputLayout(deviceContext, inputLayout);
                deviceContext->lpVtbl->IASetPrimitiveTopology(deviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                deviceContext->lpVtbl->VSSetShader(deviceContext, vertexShader, NULL, 0);
                deviceContext->lpVtbl->PSSetShader(deviceContext, pixelShader, NULL, 0);

                deviceContext->lpVtbl->Draw(deviceContext, 3, 0);

                swapChain->lpVtbl->Present(swapChain, 0, 0);
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
