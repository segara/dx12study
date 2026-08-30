#include "../Utils/pch.h"
#include "CommandQueue.h"

CommandQueue::~CommandQueue()
{
	::CloseHandle(_fenceEvent);
}

void CommandQueue::Init(ComPtr<ID3D12Device> device, std::shared_ptr<SwapChain> swapChain, std::shared_ptr<DescriptorHeap> descriptorHeap)
{
	_swapChain = swapChain;
	_descHeap = descriptorHeap;

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //gpu가 직접실행하는 명령목록
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_cmdQueue));

	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAlloc));

	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&_cmdList));

	_cmdList->Close();

	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	_fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
}

void CommandQueue::WaitSync()
{
	_fenceValue++; //Fence는 GPU 작업의 진행 번호표라고 생각하면 편합니다.

	_cmdQueue->Signal(_fence.Get(), _fenceValue);
	if (_fence->GetCompletedValue() < _fenceValue )  //현재 GPU가 완료한 Fence 값을 확인합니다.
	{
		_fence->SetEventOnCompletion(_fenceValue, _fenceEvent);
		::WaitForSingleObject(_fenceEvent, INFINITE);
	}
}
