#pragma once


//역할 : cpu / ram 에서 먼저 버퍼를 쪼개서 할당하고
// gpu / vram 에서 커맨드리스트 하나 당 위의 버퍼를 하나씩 가져다가 사용한다.
//
//[ConstantBuffer 용도]
// 셰이더의 cbuffer(b0, b1 ...) 에 넘길 데이터(Transform 등)를 담는 상수 버퍼 풀.
// 업로드 힙에 큰 버퍼 하나를 만들고 256바이트 칸(element)으로 쪼개서, draw call 마다 한 칸씩 꺼내 쓴다.
// 같은 칸을 덮어쓰면 GPU 가 아직 읽지 않은 이전 draw 의 데이터가 바뀌므로 draw 마다 새 칸을 사용한다.
//
//[메모리 레이아웃] (Init(sizeof(Transform), 256) 기준)
// _cbvBuffer (Upload heap, _elementSize * _elementCount 바이트)
//   [elem0 256B][elem1 256B][elem2 256B] ... [elem255]
//    ↑ _mappedData : CPU 가 memcpy 로 직접 쓰는 주소 (Map 해둔 상태 유지)
// _cbvDescriptorHeap (CPU 전용 디스크립터 힙, _elementCount 개) - Descriptor Table 방식에서만 사용
//   [CBV0 -> elem0][CBV1 -> elem1][CBV2 -> elem2] ...
//
//[용어 정리] 디스크립터 힙 vs CBV  -  CBV 는 힙이 아니라 힙의 한 칸에 들어가는 디스크립터 한 종류
// 디스크립터 힙 (ID3D12DescriptorHeap) <- 서랍장. 디스크립터를 담는 연속된 배열 (실제 객체)
//  ├ 칸0 : CBV ----> 상수 버퍼 elem0      <- CBV = "이 버퍼의 이 범위를 상수 버퍼로 쓴다"는 명세서
//  ├ 칸1 : CBV ----> 상수 버퍼 elem1
//  ├ 칸2 : SRV ----> 텍스처               <- (나중에) 같은 힙에 다른 종류도 섞어 넣을 수 있음
//  └ ...
//
// 디스크립터 | 뜻                                    | 들어가는 힙 타입
// CBV       | Constant Buffer View (상수 버퍼, b#)   | CBV_SRV_UAV
// SRV       | Shader Resource View (텍스처 등, t#)   | CBV_SRV_UAV
// UAV       | Unordered Access View (읽기/쓰기, u#)  | CBV_SRV_UAV
// Sampler   | 텍스처 샘플링 방식 (s#)                 | SAMPLER
// RTV       | Render Target View (백버퍼)            | RTV  (SwapChain::_rtvHeap)
// DSV       | Depth Stencil View                    | DSV
// -> D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV = "CBV, SRV, UAV 디스크립터를 담는 힙" 이라는 뜻
//
// 헷갈리기 쉬운 점
// - DX11 은 ID3D11ShaderResourceView 같은 View 객체가 따로 있었지만 DX12 에는 CBV 객체가 없다.
//   D3D12_CONSTANT_BUFFER_VIEW_DESC(설명 구조체)를 CreateConstantBufferView(&desc, handle) 에 넘기면
//   힙의 handle 칸에 CBV 가 "기록"될 뿐이다.
// - "Root CBV"(ver1) 는 이름에 CBV 가 있지만 디스크립터가 아니라 GPU 주소를 루트 파라미터에 직접 넣는 방식이다.
//   그래서 힙도, CreateConstantBufferView 도 필요 없다.
//
//[두 가지 바인딩 방식]
// ver1. Root CBV        : PushData(rootParamIndex, ...)  -> SetGraphicsRootConstantBufferView 로 GPU 주소를 직접 바인딩
// ver2. Descriptor Table: PushDataToTable(...)           -> CBV 핸들을 돌려주고, TableDescriptorHeap 이 복사/바인딩
// 어떤 방식이 유효한지는 RootSignature 의 루트 파라미터 타입이 결정한다. (현재 RootSignature 는 ver2 테이블 방식)
//
//[다른 클래스와의 관계]
// Engine              : 소유 및 Init(sizeof(Transform), 256)
// CommandQueue        : RenderBegin 에서 Clear() -> 매 프레임 0번 칸부터 재사용
// Mesh                : Render 에서 PushDataToTable 로 Transform 기록
// TableDescriptorHeap : PushDataToTable 이 돌려준 핸들을 SetCBV 로 셰이더용 힙에 복사
// RootSignature       : ver1 이면 Root CBV 슬롯, ver2 면 Descriptor Table(b0~b4) 슬롯을 선언
//
//[사용 방법]
// 1) 초기화 (Engine::Init)
//      _constantBuffer->Init(sizeof(Transform), 256);     //256B 칸 256개
// 2) 프레임 시작 (CommandQueue::RenderBegin)
//      GEngine->GetConstantBuffer()->Clear();
// 3) draw call 마다 (Mesh::Render, ver2)
//      handle = GEngine->GetConstantBuffer()->PushDataToTable(&_transform, sizeof(Transform));
//      GEngine->GetTableDescriptorHeap()->SetCBV(handle, CBV_REGISTER::b0);
//
//[주의]
// - 칸 재사용(Clear)이 안전한 이유는 RenderEnd 의 WaitSync 가 이전 프레임 GPU 작업 완료를 보장하기 때문.
//   프레임 리소스 구조(CPU/GPU 병렬)로 바꾸면 ConstantBuffer 도 프레임별로 나눠야 한다.
// - _currentIndex 가 _elementCount 를 넘는지 검사하지 않는다 (프레임당 Push 횟수 <= 256 이어야 함).
class ConstantBuffer
{
public:
	ConstantBuffer();
	~ConstantBuffer();

	void Init(uint32 size, uint32 count);	//size : 칸 하나에 담을 데이터 크기, count : 칸 개수
	void PushData(uint32 rootParamIndex, void* buffer, uint32 size);	//ver1. Root CBV 방식
	D3D12_CPU_DESCRIPTOR_HANDLE PushDataToTable(void* buffer, uint32 size);	//ver2. Descriptor Table 방식
	void Clear();	//프레임 시작 시 칸 인덱스 초기화
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32 index);	//index 번 칸의 GPU 주소
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32 index);	//index 번 칸을 가리키는 CBV 디스크립터 핸들
private:
	void CreateBuffer();	//업로드 힙에 버퍼 생성 + Map
	void CreateView();		//칸마다 CBV 디스크립터 생성

private:
	uint32 _elementSize = 0;	//칸 하나의 크기 (256 배수로 올림)
	uint32 _elementCount = 0;	//칸 개수 = 프레임당 최대 Push 횟수
	BYTE* _mappedData = nullptr;	//Map 된 CPU 주소. 여기에 memcpy 하면 곧바로 GPU 가 읽을 메모리에 반영됨

	ComPtr<ID3D12Resource> _cbvBuffer;	//업로드 힙 버퍼 (실제 데이터)

	ComPtr<ID3D12DescriptorHeap> _cbvDescriptorHeap;	//CPU 전용(FLAG_NONE) 디스크립터 힙. CopyDescriptors 의 원본으로 사용
	// _cbvHeapCPUHandleStart * _handleincrementSize 로 계산해서 현재 인덱스에 맞는 핸들을 가져온다.
	// (정확히는 시작 핸들 + index * _handleincrementSize)
	D3D12_CPU_DESCRIPTOR_HANDLE _cbvHeapCPUHandleStart;	//_cbvDescriptorHeap 의 시작 핸들
	uint32 _handleincrementSize = 0;	//디스크립터 1개 크기 (GPU 마다 다름)
	uint32 _currentIndex = 0;	//다음에 사용할 칸 번호 (Push 마다 +1, Clear 시 0)
};