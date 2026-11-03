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
	//SwapChain 은 아직 Init 전이다 (SwapChain::Init 에 이 Queue 가 필요하기 때문)
	//여기서는 포인터만 저장하고 실제 사용은 RenderBegin 부터
	_swapChain = swapChain;
	_descHeap = descriptorHeap;

	//Device(인력 사무소)로 Queue / Allocator / List / Fence 생성. 모두 DIRECT(그래픽 명령) 타입
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //gpu가 직접실행하는 명령목록
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_cmdQueue));

	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAlloc));

	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&_cmdList));

	//CommandList 는 생성되자마자 기록(open) 상태
	//첫 프레임 RenderBegin 의 Reset() 이 정상 동작하도록 미리 닫아둔다
	_cmdList->Close();

	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	_fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
}

//CPU / GPU 동기화
// 1) 번호표 N 발급
// 2) Signal(fence, N) 을 Queue 맨 뒤에 넣음 -> "앞의 작업이 다 끝나면 펜스 값을 N 으로 올려라"
// 3) 아직 N 에 도달 못했으면 이벤트를 걸고 CPU 대기
//이 함수 덕분에 Allocator Reset, ConstantBuffer 재사용이 안전하다
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

//프레임 기록 시작 (Engine::RenderBegin 에서 호출)
//이 함수와 RenderEnd 사이에 Shader::Update, Mesh::Render, ConstantBuffer::PushData 가 CMD_LIST 에 명령을 기록한다
void CommandQueue::RenderBegin(const D3D12_VIEWPORT* vp, const D3D12_RECT* rect)
{
	//지난 프레임 명령 메모리 재사용 (지난 프레임 RenderEnd 의 WaitSync 로 GPU 완료가 보장됨)
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
	//RootSignature(계약서) : b0, b1 슬롯 레이아웃 -> 이후 PushData(0/1) 가 여기에 연결됨
	_cmdList->SetGraphicsRootSignature(ROOT_SIGNATURE.Get());
	//ConstantBuffer 사용 인덱스를 0으로 (Engine.h 에 의존하게 되는 부분)
	GEngine->GetConstantBuffer()->Clear();
	GEngine->GetTableDescriptorHeap()->Clear();
	ID3D12DescriptorHeap* heaps[] = { GEngine->GetTableDescriptorHeap()->GetDescriptorHeap().Get()};
	_cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	_cmdList->ResourceBarrier(1, &barrier);
	//Engine 이 넘겨준 화면 영역
	_cmdList->RSSetViewports(1, vp);
	_cmdList->RSSetScissorRects(1, rect);

	//SwapChain 의 현재 백버퍼 RTV 를 지우고 출력 대상으로 지정
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferView = _swapChain->GetBackBufferView();
	_cmdList->ClearRenderTargetView(backBufferView, Colors::LightSteelBlue, 0, nullptr);
	_cmdList->OMSetRenderTargets(1, &backBufferView, FALSE, nullptr);

}

//기록 종료 -> 제출 -> 화면 출력 -> 동기화 (Engine::RenderEnd 에서 호출)
void CommandQueue::RenderEnd()
{
	//백버퍼를 "그릴 수 있는 상태" -> "화면에 보여줄 상태"로 되돌림
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_swapChain->GetCurrentBackBufferResource().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	_cmdList->ResourceBarrier(1, &barrier);
	_cmdList->Close(); //기록 종료. Close 해야 Execute 가능

	ID3D12CommandList* cmdListArr[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr); //실행 (이 시점부터 GPU가 실제로 작업)
	_swapChain->Present();		//화면 출력 요청
	WaitSync();					//GPU 완료까지 대기 -> 다음 프레임에 Allocator / ConstantBuffer 재사용 가능
	_swapChain->SwapIndex();	//다음 백버퍼로 (0,1 순환)
}
