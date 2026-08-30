#include "../Utils/pch.h"
#include "Engine.h"
#include "Device.h"
#include "CommandQueue.h"
#include "SwapChain.h"
#include "DescriptorHeap.h"

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
	_descHeap = std::make_shared<DescriptorHeap>();

	_device->Init();
	_cmdQueue->Init(_device->GetDevice(), _swapChain, _descHeap);
}

void Engine::Render()
{
}

void Engine::ResizeWindow(int32 width, int32 height)
{
	_window.width = width;
	_window.height = height;
	RECT rect = {0,0,width, height};
	::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	::SetWindowPos(_window.hwnd, 0, 100, 100, width, height, 0);
}
