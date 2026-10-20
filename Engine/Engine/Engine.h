#pragma once
#include "Device.h"
#include "CommandQueue.h"
#include "SwapChain.h"
#include "DescriptorHeap.h"
#include "RootSignature.h"
#include "ConstantBuffer.h"

#include "../Resource/Mesh.h"
#include "../Resource/Shader.h"

class Engine
{
public:
	void Init(const WindowInfo& info);
	void Render();
	void RenderBegin();
	void RenderEnd();
	void ResizeWindow(int32 width, int32 height);
public: 
	std::shared_ptr<Device> GetDevice() { return _device; }
	std::shared_ptr<CommandQueue> GetCommandQueue() { return _cmdQueue; }
	std::shared_ptr<SwapChain> GetSwapChain() { return _swapChain; }
	std::shared_ptr<RootSignature> GetRootSignature() { return _rootSignature; }
	std::shared_ptr<ConstantBuffer> GetConstantBuffer() { return _constantBuffer; }

private:
	WindowInfo _window;
	D3D12_VIEWPORT _viewport = {};
	D3D12_RECT _scissorRect = {};

	std::shared_ptr<Device> _device; //class Device : 전방선언
	std::shared_ptr<CommandQueue> _cmdQueue; 
	std::shared_ptr<SwapChain> _swapChain; 
	std::shared_ptr<DescriptorHeap> _descHeap;
	std::shared_ptr<RootSignature> _rootSignature;
	std::shared_ptr<ConstantBuffer> _constantBuffer;

};