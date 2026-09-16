#include "../Utils/pch.h"
#include "SwapChain.h"

void SwapChain::Init(const WindowInfo& info, ComPtr<ID3D12Device> device, ComPtr<IDXGIFactory> dxgi, ComPtr<ID3D12CommandQueue> cmdQueue)
{

	CreateSwapChain(info, dxgi, cmdQueue);
	CreateRTV(device);
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetBackBufferView()
{
	return _rtvHandle[GetCurrentBackBufferIndex()];
}
void SwapChain::CreateSwapChain(const WindowInfo& info, ComPtr<IDXGIFactory> dxgi, ComPtr<ID3D12CommandQueue> cmdQueue)
{
	_swapChain.Reset();
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = static_cast<uint32>(info.width);
	sd.BufferDesc.Height = static_cast<uint32>(info.height);
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	sd.OutputWindow = info.hwnd;
	sd.Windowed = info.windowed;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	dxgi->CreateSwapChain(cmdQueue.Get(), &sd, &_swapChain);
	for (int32 i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		_swapChain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i]));
	}
}

void SwapChain::CreateRTV(ComPtr<ID3D12Device> device)
{


	// descriptor = view
	// d11에서는 render target view, depth stencil view, constant buffer view, shader resource view,,

	int32 _rtvHeapSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); //render target view 사이즈 계산


	// descriptor heap 같은 종류의 데이터끼리 배열로 처리
	// rtv 목록 : [] [] (현재 2개)

	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc;
	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
	rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDesc.NodeMask = 0;


	device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&_rtvHeap));
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = _rtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
	{
		_rtvHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, i * _rtvHeapSize); //다음 주소만큼 이동
		device->CreateRenderTargetView(_renderTargets[i].Get(), nullptr, _rtvHandle[i]);
	}
}

void SwapChain::Present()
{
	_swapChain->Present(0, 0); //
}

void SwapChain::SwapIndex()
{
	_backBufferIndex = (_backBufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT; //0,1,0,1,0
}
