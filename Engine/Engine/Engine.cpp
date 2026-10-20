#include "../Utils/pch.h"
#include "Engine.h"


void Engine::Init(const WindowInfo& info)
{
	_window = info;
	ResizeWindow(info.width, info.height);
	//그려질 화면 크기를 설정
	_viewport = { 0,0, static_cast<FLOAT>(info.width),static_cast<FLOAT>(info.height),0.0f, 1.0f };
	_scissorRect = CD3DX12_RECT(0, 0, info.width, info.height);

	_device = std::make_shared<Device>();
	_cmdQueue = std::make_shared<CommandQueue>();
	_swapChain = std::make_shared<SwapChain>();
	//_descHeap = std::make_shared<DescriptorHeap>();
	_rootSignature = std::make_shared<RootSignature>();
	_constantBuffer = std::make_shared<ConstantBuffer>();

	_device->Init();
	_cmdQueue->Init(_device->GetDevice(), _swapChain, _descHeap);
	_swapChain->Init(_window, _device->GetDevice(),_device->GetDXGI(), _cmdQueue->GetCmdQueue());
	//_descHeap->Init(_device->GetDevice(), _swapChain);
	_rootSignature->Init(_device->GetDevice());		
	_constantBuffer->Init(sizeof(Transform), 256); //256개까지 상수버퍼를 쪼개서 사용가능
}

void Engine::Render()
{
	RenderBegin();
	RenderEnd();
}

void Engine::RenderBegin()
{
	_cmdQueue->RenderBegin(&_viewport, &_scissorRect);
}

void Engine::RenderEnd()
{
	_cmdQueue->RenderEnd();
}

void Engine::ResizeWindow(int32 width, int32 height)
{
	_window.width = width;
	_window.height = height;
	RECT rect = {0,0,width, height};
	::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	::SetWindowPos(_window.hwnd, 0, 100, 100, width, height, 0);
}
