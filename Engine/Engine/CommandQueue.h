#pragma once

//특히 큰 C++ 프로젝트에서는 헤더 하나가 다른 헤더 수십 개를 연쇄적으로 include하는 상황을 줄일 수 있습니다.
//
//컴파일 시간이 줄어듦
//
//예를 들어 :
//
//Game.h
//↓
//Player.h
//↓
//Weapon.h
//↓
//Item.h
//↓
//Inventory.h
//
//처럼 include가 연결되어 있으면 Inventory.h 하나를 수정했는데도 많은.cpp가 다시 컴파일될 수 있습니다.
//
//전방 선언을 사용하면 이런 불필요한 재컴파일을 줄이는 데 도움이 됩니다.
//
//순환 include 문제를 피하기 쉬움
//// Player.h
//#include "Enemy.h"
//
//class Player
//{
//	Enemy* target;
//};
//// Enemy.h
//#include "Player.h"
//
//class Enemy
//{
//	Player* target;
//};
//
//서로 include하는 구조가 됩니다.
//
//전방 선언을 사용하면 :
//
//// Player.h
//class Enemy;
//
//class Player
//{
//	Enemy* target;
//};
//// Enemy.h
//class Player;
//
//class Enemy
//{
//	Player* target;
//};

class SwapChain;
class DescriptorHeap;

//[CommandQueue 개요]
// CPU가 GPU에게 일을 맡기는 창구.
// 명령을 하나씩 보내지 않고 CommandList에 기록해 두었다가 Queue로 한 번에 제출한다.
// 한 프레임의 시작(RenderBegin)과 끝(RenderEnd)을 감싸고,
// 그 사이에 Shader / Mesh / ConstantBuffer 가 CMD_LIST 매크로로 명령을 채운다.
//
//[핵심 규칙]
// - CommandList 는 Close() 한 뒤에만 Queue 에 제출(Execute)할 수 있다.
// - CommandAllocator 는 GPU가 그 안의 명령을 다 실행한 뒤에만 Reset() 할 수 있다. -> Fence 로 보장
//
//[다른 클래스와의 관계]
// Engine         : shared_ptr 로 소유. Engine::RenderBegin/End 가 viewport/scissor 를 넘기며 위임
//                  CMD_LIST 매크로(GEngine->GetCommandQueue()->GetCmdList())의 진입점
// Device         : Init 에서 ID3D12Device 를 받아 Queue/Allocator/List/Fence 생성 (이후엔 사용 안 함)
// SwapChain      : 서로 의존
//                  1) SwapChain::Init 이 GetCmdQueue() 를 받는다 (DX12 는 Present 가 Queue 를 통해 처리되므로
//                     CreateSwapChain 에 Device 대신 Queue 를 넘김)
//                  2) CommandQueue 는 현재 백버퍼 리소스/RTV 로 Barrier, Clear 를 하고 Present, SwapIndex 호출
//                  -> 그래서 CommandQueue 가 먼저 Init 되고 SwapChain 포인터만 미리 받아둔다
// RootSignature  : 매 프레임 SetGraphicsRootSignature -> 이후 PushData(0/1) 가 b0/b1 에 연결됨
// ConstantBuffer : 서로 의존. RenderBegin 에서 Clear() 로 인덱스 초기화, CB 는 CMD_LIST 에 루트 CBV 바인딩 기록
//                  매 프레임 같은 256칸을 덮어써도 안전한 이유 = WaitSync 가 이전 프레임 GPU 작업 완료를 보장
// Shader / Mesh  : RenderBegin ~ RenderEnd 사이에 CMD_LIST 로 PSO 설정, Draw 를 기록
//                  (CommandQueue 는 이들을 모르고, 이들이 전역 매크로로 CommandQueue 에 접근)
// DescriptorHeap : 포인터만 보관 (현재 nullptr). 테이블 방식 CBV/SRV 로 가면 SetDescriptorHeaps 를 여기서 호출
// Game(Client)   : 초기화 마지막에 WaitSync() 호출로 GPU 준비 완료를 기다림
//
//[현재 구조의 한계]
// - 매 프레임 WaitSync 로 GPU 완료를 기다리므로 GPU가 그리는 동안 CPU는 쉰다 (병렬성 없음, 대신 단순/안전)
// - Allocator 가 하나뿐인 것도 이 동기화 덕분에 가능하다.
//   다음 단계는 "프레임 리소스" 구조: 백버퍼 수만큼 Allocator + 펜스 값을 두고 해당 프레임만 기다림
//   이때 ConstantBuffer 도 프레임별로 나눠야 한다 (아니면 GPU가 읽는 중인 CB 데이터를 덮어씀)
// - RenderBegin 이 GEngine->GetConstantBuffer()->Clear() 를 직접 호출해 Engine.h 에 의존한다.
//   CB 초기화는 Engine::RenderBegin 으로 옮기면 역할이 더 분명해진다.
class CommandQueue
{
public:
	~CommandQueue();
	void Init(ComPtr<ID3D12Device> device, std::shared_ptr<SwapChain> swapChain, std::shared_ptr<DescriptorHeap> descriptorHeap);
	void WaitSync();	//GPU가 지금까지 제출된 작업을 끝낼 때까지 CPU 대기
	void RenderBegin(const D3D12_VIEWPORT* vp, const D3D12_RECT* rect);	//프레임 기록 시작
	void RenderEnd();	//기록 종료 -> 제출 -> Present -> 동기화
	ComPtr<ID3D12CommandQueue> GetCmdQueue() { return _cmdQueue; }	//SwapChain 생성 시 사용
	ComPtr<ID3D12GraphicsCommandList> GetCmdList() { return _cmdList; }	//CMD_LIST 매크로
private:
	//CommandQueue : DX12 에 등장
	//외주를 요청할때 하나씩 요청하면비효율적
	//외주목록에 일감을 차곡차곡 기록했다가 한방에 요청
	ComPtr<ID3D12CommandQueue> _cmdQueue;			//외주 접수 창구 : 기록이 끝난 목록을 GPU에 제출
	ComPtr<ID3D12CommandAllocator> _cmdAlloc;		//목록을 적는 종이 : 기록된 명령이 실제로 저장되는 메모리
	ComPtr<ID3D12GraphicsCommandList> _cmdList;//일감 리스트 : Clear, Draw, Barrier 등을 기록

	//cpu/gpu 동기화를 위한 간단한 도구
	ComPtr<ID3D12Fence> _fence;							//GPU가 어디까지 끝냈는지 나타내는 값
	uint32				_fenceValue = 0;				//CPU가 발급한 마지막 번호표
	HANDLE				_fenceEvent = INVALID_HANDLE_VALUE;	//펜스 값 도달 시 깨워줄 Win32 이벤트

	std::shared_ptr<SwapChain> _swapChain;			//백버퍼 / RTV / Present
	std::shared_ptr<DescriptorHeap> _descHeap;		//아직 미사용 (nullptr)
};