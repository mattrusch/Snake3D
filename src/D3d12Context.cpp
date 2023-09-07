// D3d12Context.cpp

#include "D3d12Context.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include <wrl.h>
#include "Window.h"
#include <cassert>

constexpr size_t ALIGN_256(size_t in)
{
    return (in + 0xff) & ~0xff;
}

class SceneConstantBuffer
{
public:
    DirectX::XMMATRIX mWorldViewProj;
};

class D3dContext
{
public:
    static const UINT   kFrameCount      = 2;
    static const size_t kConstBufferSize = 1024 * 64;

    Microsoft::WRL::ComPtr<IDXGISwapChain3>           mSwapChain;
    Microsoft::WRL::ComPtr<ID3D12Device>              mDevice;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    mCommandAllocator;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>        mCommandQueue;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>       mRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>       mPipelineState;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>      mCbvSrvHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource>            mConstantBuffer;
    uint8_t* mpCbvDataBegin;
    SceneConstantBuffer                               mConstantBufferData;
    Microsoft::WRL::ComPtr<ID3D12Resource>            mTexture;

    Microsoft::WRL::ComPtr<ID3D12Resource>            mVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW                          mVertexBufferView;

    Microsoft::WRL::ComPtr<ID3D12Resource>            mIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW                           mIndexBufferView;

    Microsoft::WRL::ComPtr<ID3D12Fence>               mFence;
    HANDLE                                            mFenceEvent;
    UINT64                                            mFenceValue;
    unsigned int                                      mFrameIndex = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource>            mRenderTargets[kFrameCount];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>      mRtvHeap;
    unsigned int                                      mRtvDescriptorSize = 0;
    CD3DX12_VIEWPORT                                  mViewport;
    CD3DX12_RECT                                      mScissorRect;

    Microsoft::WRL::ComPtr<ID3D12Resource>            mDepthStencil;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>      mDsvHeap;
};

D3dContext gDevice;
const int gX = 100;
const int gY = 100;
const int gWidth = 1024;
const int gHeight = 1024;

class Vertex
{
public:
    float pos[3];
    float color[4];
    float texcoord[2];
};

const uint32_t gTexWidth = 64;
const uint32_t gTexHeight = 64;
const uint32_t gTexBpp = 4;
char gTexData[gTexWidth][gTexHeight][gTexBpp];

void InitDevice(HWND hwnd);

void Init(HWND hwnd)
{
    InitTexture(&gTexData[0][0][0], gTexWidth, gTexHeight, gTexBpp);
    InitDevice(hwnd);
    InitAssets();
}

using Microsoft::WRL::ComPtr;

inline void D3D_CHECK(HRESULT hr)
{
    assert(SUCCEEDED(hr));
}

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
_Use_decl_annotations_
void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
    ComPtr<IDXGIAdapter1> adapter;
    *ppAdapter = nullptr;

    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            // If you want a software adapter, pass in "/warp" on the command line.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

    *ppAdapter = adapter.Detach();
}

void WaitForPreviousFrame()
{
    // Signal and increment fence value
    const UINT64 fence = gDevice.mFenceValue;
    D3D_CHECK(gDevice.mCommandQueue->Signal(gDevice.mFence.Get(), fence));
    gDevice.mFenceValue++;

    // Wait until the previous frame is finished
    if (gDevice.mFence->GetCompletedValue() < fence)
    {
        D3D_CHECK(gDevice.mFence->SetEventOnCompletion(fence, gDevice.mFenceEvent));
        WaitForSingleObject(gDevice.mFenceEvent, INFINITE);
    }

    gDevice.mFrameIndex = gDevice.mSwapChain->GetCurrentBackBufferIndex();
}

void InitDevice(HWND hwnd)
{
    gDevice.mViewport.Width = static_cast<float>(gWidth);
    gDevice.mViewport.Height = static_cast<float>(gHeight);
    gDevice.mViewport.MaxDepth = 1.0f;
    gDevice.mViewport.MinDepth = 0.0f;
    gDevice.mViewport.TopLeftX = 0.0f;
    gDevice.mViewport.TopLeftY = 0.0f;

    gDevice.mScissorRect.top = 0;
    gDevice.mScissorRect.left = 0;
    gDevice.mScissorRect.bottom = gHeight;
    gDevice.mScissorRect.right = gWidth;

    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    {
        // Enable debug layer
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    D3D_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    bool useWarpDevice = false;
    if (useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        D3D_CHECK(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        D3D_CHECK(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&gDevice.mDevice)));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter);
        D3D_CHECK(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&gDevice.mDevice)));
    }

    // Create command queue
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    D3D_CHECK(gDevice.mDevice->CreateCommandQueue(
        &commandQueueDesc,
        IID_PPV_ARGS(&gDevice.mCommandQueue)));

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = gDevice.kFrameCount;
    swapChainDesc.Width = gWidth;
    swapChainDesc.Height = gHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    D3D_CHECK(factory->CreateSwapChainForHwnd(
        gDevice.mCommandQueue.Get(),
        hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain));

    D3D_CHECK(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
    D3D_CHECK(swapChain.As(&gDevice.mSwapChain));
    gDevice.mFrameIndex = gDevice.mSwapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps

    // RTV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = gDevice.kFrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    D3D_CHECK(gDevice.mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&gDevice.mRtvHeap)));
    gDevice.mRtvDescriptorSize = gDevice.mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // DSV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    D3D_CHECK(gDevice.mDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&gDevice.mDsvHeap)));

    // CBVSRV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
    cbvHeapDesc.NumDescriptors = 2; // TODO: Make this bigger
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    D3D_CHECK(gDevice.mDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&gDevice.mCbvSrvHeap)));

    // Create frame resources

    // Render targets
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(gDevice.mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < gDevice.kFrameCount; ++i)
    {
        D3D_CHECK(gDevice.mSwapChain->GetBuffer(i, IID_PPV_ARGS(&gDevice.mRenderTargets[i])));
        gDevice.mDevice->CreateRenderTargetView(gDevice.mRenderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, gDevice.mRtvDescriptorSize);
    }

    // Depth stencil buffer
    D3D12_CLEAR_VALUE depthOptClearValue = {};
    depthOptClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptClearValue.DepthStencil.Depth = 1.0f;
    depthOptClearValue.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, gWidth, gHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    D3D_CHECK(gDevice.mDevice->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptClearValue,
        IID_PPV_ARGS(&gDevice.mDepthStencil)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(gDevice.mDsvHeap->GetCPUDescriptorHandleForHeapStart());

    gDevice.mDevice->CreateDepthStencilView(gDevice.mDepthStencil.Get(), &dsvDesc, dsvHandle);

    D3D_CHECK(gDevice.mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gDevice.mCommandAllocator)));
}

void InitAssets()
{
    // Create root signature
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(gDevice.mDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
    //CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    CD3DX12_ROOT_PARAMETER1 rootParameters[2];

    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE);
    rootParameters[0].InitAsDescriptorTable(2, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
    //rootParameters[1].InitAsConstants(1, 1, 0);
    rootParameters[1].InitAsConstantBufferView(1);

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    D3D_CHECK(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
    D3D_CHECK(gDevice.mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&gDevice.mRootSignature)));

    // Create pipeline state
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;

    UINT compileFlags = 0;
#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif//_DEBUG

    D3D_CHECK(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VsMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
    D3D_CHECK(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PsMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

    // Input layout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // Create pipeline state object
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = gDevice.mRootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    D3D_CHECK(gDevice.mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&gDevice.mPipelineState)));

    // Create command list
    D3D_CHECK(gDevice.mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gDevice.mCommandAllocator.Get(), gDevice.mPipelineState.Get(), IID_PPV_ARGS(&gDevice.mCommandList)));

    // Create the vertex buffer
    const float cubeScale = 0.25f;
    Vertex triangleVerts[] =
    {
        // Top
        { { -1.0f * cubeScale,  1.0f * cubeScale,  1.0f * cubeScale }, { 1.0f, 0.25f, 0.25f, 1.0f }, { 0.0f, 1.0f } },
        { {  1.0f * cubeScale,  1.0f * cubeScale,  1.0f * cubeScale }, { 0.25f, 1.0f, 0.25f, 1.0f }, { 1.0f, 1.0f } },
        { {  1.0f * cubeScale,  1.0f * cubeScale, -1.0f * cubeScale }, { 0.25f, 0.25f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
        { { -1.0f * cubeScale,  1.0f * cubeScale, -1.0f * cubeScale }, { 1.0f, 0.25f, 1.0f,  1.0f }, { 0.0f, 0.0f } },
        // Bottom
        { { -1.0f * cubeScale, -1.0f * cubeScale, -1.0f * cubeScale }, { 1.0f, 0.25f, 0.25f, 1.0f }, { 0.0f, 0.0f } },
        { { -1.0f * cubeScale, -1.0f * cubeScale,  1.0f * cubeScale }, { 0.25f, 1.0f, 0.25f, 1.0f }, { 0.0f, 1.0f } },
        { {  1.0f * cubeScale, -1.0f * cubeScale,  1.0f * cubeScale }, { 0.25f, 0.25f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
        { {  1.0f * cubeScale, -1.0f * cubeScale, -1.0f * cubeScale }, { 1.0f, 0.25f, 1.0f,  1.0f }, { 1.0f, 0.0f } },
        // Left                                                  
        { { -1.0f * cubeScale,  1.0f * cubeScale, -1.0f * cubeScale }, { 1.0f, 0.25f, 0.25f, 1.0f }, { 0.0f, 1.0f } },
        { { -1.0f * cubeScale,  1.0f * cubeScale,  1.0f * cubeScale }, { 0.25f, 1.0f, 0.25f, 1.0f }, { 1.0f, 1.0f } },
        { { -1.0f * cubeScale, -1.0f * cubeScale,  1.0f * cubeScale }, { 0.25f, 0.25f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
        { { -1.0f * cubeScale, -1.0f * cubeScale, -1.0f * cubeScale }, { 1.0f, 0.25f, 1.0f,  1.0f }, { 0.0f, 0.0f } },
        // Right                                                 
        { {  1.0f * cubeScale,  1.0f * cubeScale, -1.0f * cubeScale }, { 1.0f, 0.25f, 0.25f, 1.0f }, { 0.0f, 1.0f } },
        { {  1.0f * cubeScale,  1.0f * cubeScale,  1.0f * cubeScale }, { 0.25f, 1.0f, 0.25f, 1.0f }, { 1.0f, 1.0f } },
        { {  1.0f * cubeScale, -1.0f * cubeScale,  1.0f * cubeScale }, { 0.25f, 0.25f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
        { {  1.0f * cubeScale, -1.0f * cubeScale, -1.0f * cubeScale }, { 1.0f, 0.25f, 1.0f,  1.0f }, { 0.0f, 0.0f } },
        // Back                                                  
        { { -1.0f * cubeScale,  1.0f * cubeScale,  1.0f * cubeScale }, { 1.0f, 0.25f, 0.25f, 1.0f }, { 0.0f, 1.0f } },
        { {  1.0f * cubeScale,  1.0f * cubeScale,  1.0f * cubeScale }, { 0.25f, 1.0f, 0.25f, 1.0f }, { 1.0f, 1.0f } },
        { {  1.0f * cubeScale, -1.0f * cubeScale,  1.0f * cubeScale }, { 0.25f, 0.25f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
        { { -1.0f * cubeScale, -1.0f * cubeScale,  1.0f * cubeScale }, { 1.0f, 0.25f, 1.0f,  1.0f }, { 0.0f, 0.0f } },
        // Front                                                 
        { { -1.0f * cubeScale,  1.0f * cubeScale, -1.0f * cubeScale }, { 1.0f, 0.25f, 0.25f, 1.0f }, { 0.0f, 1.0f } },
        { {  1.0f * cubeScale,  1.0f * cubeScale, -1.0f * cubeScale }, { 0.25f, 1.0f, 0.25f, 1.0f }, { 1.0f, 1.0f } },
        { {  1.0f * cubeScale, -1.0f * cubeScale, -1.0f * cubeScale }, { 0.25f, 0.25f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
        { { -1.0f * cubeScale, -1.0f * cubeScale, -1.0f * cubeScale }, { 1.0f, 0.25f, 1.0f,  1.0f }, { 0.0f, 0.0f } },
    };

    const UINT vertexBufferSize = sizeof(triangleVerts);

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC buffer = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    D3D_CHECK(gDevice.mDevice->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &buffer,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&gDevice.mVertexBuffer)));

    // Copy triangle data to vertex buffer
    UINT8* pVertexData;
    CD3DX12_RANGE readRangeVb(0, 0);
    D3D_CHECK(gDevice.mVertexBuffer->Map(0, &readRangeVb, reinterpret_cast<void**>(&pVertexData)));
    memcpy(pVertexData, triangleVerts, sizeof(triangleVerts));
    gDevice.mVertexBuffer->Unmap(0, nullptr);

    // Initialize VB view
    gDevice.mVertexBufferView.BufferLocation = gDevice.mVertexBuffer->GetGPUVirtualAddress();
    gDevice.mVertexBufferView.StrideInBytes = sizeof(Vertex);
    gDevice.mVertexBufferView.SizeInBytes = vertexBufferSize;

    // Create index buffer
    uint32_t indices[] =
    {
        0, 1, 3, 3, 1, 2,
        4, 5, 7, 7, 5, 6,
        8, 9, 11, 11, 9, 10,
        12, 13, 15, 15, 13, 14,
        16, 17, 19, 19, 17, 18,
        20, 21, 23, 23, 21, 22
    };

    const UINT indexBufferSize = sizeof(indices);

    CD3DX12_HEAP_PROPERTIES ibHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
    D3D_CHECK(gDevice.mDevice->CreateCommittedResource(
        &ibHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&gDevice.mIndexBuffer)));

    // Copy data into index buffer
    UINT8* pIndexData;
    CD3DX12_RANGE readRangeIb(0, 0);
    D3D_CHECK(gDevice.mIndexBuffer->Map(0, &readRangeIb, reinterpret_cast<void**>(&pIndexData)));
    memcpy(pIndexData, indices, sizeof(indices));
    gDevice.mIndexBuffer->Unmap(0, nullptr);

    // Initialize IB view
    gDevice.mIndexBufferView.BufferLocation = gDevice.mIndexBuffer->GetGPUVirtualAddress();
    gDevice.mIndexBufferView.SizeInBytes = indexBufferSize;
    gDevice.mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;

    // Create the constant buffer
    CD3DX12_HEAP_PROPERTIES cbHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC cbResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(D3dContext::kConstBufferSize);
    D3D_CHECK(gDevice.mDevice->CreateCommittedResource(
        &cbHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &cbResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&gDevice.mConstantBuffer)));

    // Create constant buffer view
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = gDevice.mConstantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = (UINT)ALIGN_256(sizeof(SceneConstantBuffer));
    gDevice.mDevice->CreateConstantBufferView(&cbvDesc, gDevice.mCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());

    // Map and initialize constant buffer
    CD3DX12_RANGE readRangeCb(0, 0);
    D3D_CHECK(gDevice.mConstantBuffer->Map(0, &readRangeCb, reinterpret_cast<void**>(&gDevice.mpCbvDataBegin)));
    memcpy(gDevice.mpCbvDataBegin, &gDevice.mConstantBufferData, sizeof(gDevice.mConstantBufferData));

    // Create texture
    CD3DX12_HEAP_PROPERTIES texHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    const auto texResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, gTexWidth, gTexHeight, 1, 1);
    D3D_CHECK(gDevice.mDevice->CreateCommittedResource(
        &texHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texResourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&gDevice.mTexture)));

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(gDevice.mTexture.Get(), 0, 1);

    // Create GPU upload buffer
    CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    ComPtr<ID3D12Resource> textureUploadHeap;
    D3D_CHECK(gDevice.mDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &uploadResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&textureUploadHeap)));

    // Copy data to the upload heap and schedule a copy from the upload heap to the texture
    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = &gTexData[0][0][0];
    textureData.RowPitch = gTexWidth * gTexBpp;
    textureData.SlicePitch = textureData.RowPitch * gTexHeight;

    UpdateSubresources(gDevice.mCommandList.Get(), gDevice.mTexture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);

    CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        gDevice.mTexture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    gDevice.mCommandList->ResourceBarrier(1, &resourceBarrier);

    // Create SRV for the texture
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texResourceDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    gDevice.mDevice->CreateShaderResourceView(
        gDevice.mTexture.Get(),
        &srvDesc,
        CD3DX12_CPU_DESCRIPTOR_HANDLE(gDevice.mCbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), 1, gDevice.mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));

    // Close command list and execute to begin initial GPU setup
    D3D_CHECK(gDevice.mCommandList->Close());
    ID3D12CommandList* ppCommandLists[] = { gDevice.mCommandList.Get() };
    gDevice.mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create synchroniztion objects
    D3D_CHECK(gDevice.mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gDevice.mFence)));
    gDevice.mFenceValue = 1;
    gDevice.mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (gDevice.mFenceEvent == nullptr)
    {
        D3D_CHECK(HRESULT_FROM_WIN32(GetLastError()));
    }

    WaitForPreviousFrame();
}

void Update(const DirectX::XMMATRIX& lookAt, float elapsedSeconds)
{
    static float totalRotation = 0.0f;
    totalRotation += elapsedSeconds * 0.5f;

    DirectX::XMMATRIX matRotation = DirectX::XMMatrixRotationY(totalRotation);
    DirectX::XMMATRIX matLookAt = lookAt; // DirectX::XMMatrixLookAtLH({ 0.0f, 1.0f, -4.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f });
    DirectX::XMMATRIX matPerspective = DirectX::XMMatrixPerspectiveFovLH(1.0f, static_cast<float>(gWidth) / static_cast<float>(gHeight), 0.1f, 10.0f);

    // CB for first instance
    DirectX::XMMATRIX worldViewProj = matRotation * matLookAt * matPerspective;
    memcpy(gDevice.mpCbvDataBegin, &worldViewProj, sizeof(worldViewProj));

    // CB for second instance
    size_t offset = ALIGN_256(sizeof(worldViewProj));
    worldViewProj = matRotation * DirectX::XMMatrixTranslation(0.75f, 0.0f, 0.0f) * matLookAt * matPerspective;
    memcpy(gDevice.mpCbvDataBegin + offset, &worldViewProj, sizeof(worldViewProj));

    // CB for third instance
    offset += ALIGN_256(sizeof(worldViewProj));
    worldViewProj = matRotation * DirectX::XMMatrixTranslation(-0.75f, 0.0f, 0.0f) * matLookAt * matPerspective;
    memcpy(gDevice.mpCbvDataBegin + offset, &worldViewProj, sizeof(worldViewProj));
}

void PopulateCommandList()
{
    // Command list allocators can only be reset when the associated command lists have finished execution on the GPU; use fences to determine GPU execution progress
    D3D_CHECK(gDevice.mCommandAllocator->Reset());

    // When ExecuteCommandList() is called on a particular command list, that command list can then be reset at any time and must be before re-recording
    D3D_CHECK(gDevice.mCommandList->Reset(gDevice.mCommandAllocator.Get(), gDevice.mPipelineState.Get()));

    // Set necessary state
    gDevice.mCommandList->SetGraphicsRootSignature(gDevice.mRootSignature.Get());

    // Set descriptor heaps
    ID3D12DescriptorHeap* ppHeaps[] = { gDevice.mCbvSrvHeap.Get() };
    gDevice.mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // Set root descriptor table
    gDevice.mCommandList->SetGraphicsRootDescriptorTable(0, gDevice.mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());

    // Set root command buffer view for first instance
    gDevice.mCommandList->SetGraphicsRootConstantBufferView(1, gDevice.mConstantBuffer->GetGPUVirtualAddress());

    gDevice.mCommandList->RSSetViewports(1, &gDevice.mViewport);
    gDevice.mCommandList->RSSetScissorRects(1, &gDevice.mScissorRect);

    // Indicate that the back buffer will be used as a render target
    CD3DX12_RESOURCE_BARRIER rtResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        gDevice.mRenderTargets[gDevice.mFrameIndex].Get(), 
        D3D12_RESOURCE_STATE_PRESENT, 
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    gDevice.mCommandList->ResourceBarrier(1, &rtResourceBarrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(gDevice.mRtvHeap->GetCPUDescriptorHandleForHeapStart(), gDevice.mFrameIndex, gDevice.mRtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(gDevice.mDsvHeap->GetCPUDescriptorHandleForHeapStart());
    gDevice.mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Record commands
    const float clearColor[] = { 0.65f, 0.65f, 0.85f, 1.0f };
    gDevice.mCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    gDevice.mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    gDevice.mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    gDevice.mCommandList->IASetVertexBuffers(0, 1, &gDevice.mVertexBufferView);
    gDevice.mCommandList->IASetIndexBuffer(&gDevice.mIndexBufferView);
    gDevice.mCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

    // Second instance
    gDevice.mCommandList->SetGraphicsRootConstantBufferView(1, gDevice.mConstantBuffer->GetGPUVirtualAddress() + ALIGN_256(sizeof(SceneConstantBuffer)));
    gDevice.mCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

    // Third instance
    gDevice.mCommandList->SetGraphicsRootConstantBufferView(1, gDevice.mConstantBuffer->GetGPUVirtualAddress() + ALIGN_256(sizeof(SceneConstantBuffer)) * 2);
    gDevice.mCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

    CD3DX12_RESOURCE_BARRIER presentResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(gDevice.mRenderTargets[gDevice.mFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    // Indicate that the back buffer will now be used to present
    gDevice.mCommandList->ResourceBarrier(1, &presentResourceBarrier);

    D3D_CHECK(gDevice.mCommandList->Close());
}

void Render()
{
    PopulateCommandList();

    ID3D12CommandList* ppCommandLists[] = { gDevice.mCommandList.Get() };
    gDevice.mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    D3D_CHECK(gDevice.mSwapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void Destroy()
{
    WaitForPreviousFrame();

    CloseHandle(gDevice.mFenceEvent);
}

void InitTexture(char* dst, uint32_t width, uint32_t height, uint32_t bpp)
{
    uint8_t range = 0x40;

    for (uint32_t j = 0; j < height; ++j)
    {
        for (uint32_t i = 0; i < width; ++i)
        {
            dst[j * (width * bpp) + (i * bpp) + 0] = i % 16 < 8 && j % 8 < 4 ? 0xff : (abs(rand()) % range) + (0xff - range);
            dst[j * (width * bpp) + (i * bpp) + 1] = i % 16 < 8 && j % 8 < 4 ? 0xff : (abs(rand()) % range) + (0xff - range);
            dst[j * (width * bpp) + (i * bpp) + 2] = i % 16 < 8 && j % 8 < 4 ? 0xff : (abs(rand()) % range) + (0xff - range);
            dst[j * (width * bpp) + (i * bpp) + 3] = 0xffu;
        }
    }
}
