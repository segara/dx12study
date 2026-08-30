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


class CommandQueue
{
public:
	~CommandQueue();
	void Init(ComPtr<ID3D12Device> device, std::shared_ptr<SwapChain> swapChain, std::shared_ptr<DescriptorHeap> descriptorHeap);
	void WaitSync();
	void RenderBegin(const D3D12_VIEWPORT* vp, const D3D12_RECT* rect);
	void RenderEnd();
private:
	//CommandQueue : DX12 에 등장
	//외주를 요청할때 하나씩 요청하면비효율적
	//외주목록에 일감을 차곡차곡 기록했다가 한방에 요청
	ComPtr<ID3D12CommandQueue> _cmdQueue;
	ComPtr<ID3D12CommandAllocator> _cmdAlloc;
	ComPtr<ID3D12GraphicsCommandList> _cmdList;//일감 리스트

	//cpu/gpu 동기화를 위한 간단한 도구
	ComPtr<ID3D12Fence> _fence;
	uint32				_fenceValue = 0;
	HANDLE				_fenceEvent = INVALID_HANDLE_VALUE;

	std::shared_ptr<SwapChain> _swapChain;
	std::shared_ptr<DescriptorHeap> _descHeap;
};