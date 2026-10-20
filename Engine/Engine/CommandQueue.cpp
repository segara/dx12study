#include "../Utils/pch.h"
#include "CommandQueue.h"
#include "SwapChain.h"
#include "DescriptorHeap.h"
#include "Engine.h"
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

void CommandQueue::RenderBegin(const D3D12_VIEWPORT* vp, const D3D12_RECT* rect)
{
	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc.Get(), nullptr);
	//backbuffer 에 그려줄준비
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_swapChain->GetCurrentBackBufferResource().Get(),
		D3D12_RESOURCE_STATE_PRESENT, //현재 보여주는애를 rendertarget으로 설정
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	//삼각형 그릴시 : SetGraphicsRootSignature 를 추가하지 않아도 정상 실행되는 이유
	//셰이더가 리소스를 전혀 쓰지 않음
	//VS가 SV_VertexID로 정점을 계산하고 PS가 고정 색상을 출력하는 경우처럼 셰이더가 cbuffer, 텍스처, 샘플러를 하나도 읽지 않으면
	// GPU가 루트 인자를 참조할 일이 없습니다.그래서 바인딩이 비어 있어도 실제로는 문제가 드러나지 않습니다.
	_cmdList->SetGraphicsRootSignature(ROOT_SIGNATURE.Get());
	GEngine->GetConstantBuffer()->Clear();

	_cmdList->ResourceBarrier(1, &barrier);
	_cmdList->RSSetViewports(1, vp);
	_cmdList->RSSetScissorRects(1, rect);

	D3D12_CPU_DESCRIPTOR_HANDLE backBufferView = _swapChain->GetBackBufferView();
	_cmdList->ClearRenderTargetView(backBufferView, Colors::LightSteelBlue, 0, nullptr);
	_cmdList->OMSetRenderTargets(1, &backBufferView, FALSE, nullptr);

}

void CommandQueue::RenderEnd()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_swapChain->GetCurrentBackBufferResource().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	_cmdList->ResourceBarrier(1, &barrier);
	_cmdList->Close();

	ID3D12CommandList* cmdListArr[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr); //실행
	_swapChain->Present();
	WaitSync();
	_swapChain->SwapIndex();
}
