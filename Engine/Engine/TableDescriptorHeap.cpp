#include "../Utils/pch.h"
#include "TableDescriptorHeap.h"
#include "Engine.h"

//[목적] 셰이더가 볼 수 있는 디스크립터 힙을 "그룹 수 * 레지스터 수" 만큼 한 번에 생성한다.
// count : 그룹 개수 = 프레임당 최대 draw call 수
void TableDescriptorHeap::Init(uint32 count)
{
	_groupCount = count;

	// 의도 : _handleSize = 디스크립터 1개 크기, _groupSize = _handleSize * REGISTER_COUNT (한 그룹 크기)
	_handleSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_groupSize = _handleSize * REGISTER_COUNT;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = _groupCount * REGISTER_COUNT;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; //셰이더가 테이블로 읽을 수 있도록

	DEVICE->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_tableDescriptorHeap));
}

//[목적] 매 프레임 시작 시 그룹 인덱스를 0으로 되돌려 힙을 처음부터 재사용한다.
// CommandQueue::RenderBegin 에서 호출 (이전 프레임 GPU 작업은 WaitSync 로 이미 끝난 상태)
void TableDescriptorHeap::Clear()
{
	_currentGroupIndex = 0;
}

//[목적] ConstantBuffer 쪽 CBV 디스크립터(srcHandle)를 현재 그룹의 reg 위치로 복사한다.

//D3D12_CPU_DESCRIPTOR_HANDLE : 디스크립터가 놓인 위치를 가리키는 주소
//디스크립터 (View) :	리소스를 설명하는 작은 구조체(보통 32~64바이트). 주소, 크기, 포맷 등이 담김	/물건에 붙은 명세서
//CPU 디스크립터 핸들 :	디스크립터 힙 안에서 그 명세서가 놓인 칸의 주소 /	명세서가 꽂힌 서랍 번호

// srcHandle : ConstantBuffer::PushDataToTable 이 돌려준 CPU 핸들 (CPU 전용 힙)
// reg       : 셰이더 레지스터 (b0, b1 ...) -> 그룹 안에서의 위치
// CopyDescriptors 는 커맨드 리스트가 아닌 Device 함수라 호출 즉시 CPU 에서 복사된다.
void TableDescriptorHeap::SetCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, CBV_REGISTER reg)
{
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = GetCPUHandle(reg);
	uint32 destRange = 1; // Number of descriptors to copy to the destination
	uint32 srcRange = 1;  // Number of descriptors to copy from the source
	
	DEVICE->CopyDescriptors(1, &destHandle, &destRange ,1, &srcHandle, &srcRange, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

//=====================================================================================
//[CopyDescriptors] 디스크립터 힙 A 의 디스크립터를 힙 B 의 원하는 칸으로 복사
//=====================================================================================
// 복사되는 것은 리소스 데이터가 아니라 리소스를 설명하는 명세서(디스크립터)다.
//
//  ConstantBuffer::_cbvDescriptorHeap (CPU 전용, 원본 서류함)      TableDescriptorHeap (SHADER_VISIBLE, 셰이더가 읽는 결재판)
//   [CBV0][CBV1][CBV2][CBV3] ...                          [group0: b0 b1 b2 b3 b4][group1: b0 b1 ...]
//     |     |                                                ^    ^
//     +-----|------------- CopyDescriptors -----------------+    |
//           +------------- CopyDescriptors ----------------------+
//
//[복사하는 이유] 흩어진 디스크립터를 연속된 테이블로 모으기 위해
// - Descriptor Table 은 시작 위치부터 b0, b1, ... 이 연속으로 놓여 있어야 한다.
// - 그런데 draw 마다 쓰는 디스크립터는 흩어져 있다 (CBV 는 _cbvDescriptorHeap 의 i번 칸, 나중에 텍스처 SRV 는 다른 힙)
// -> 원본은 미리 한 번 만들어 두고, draw 마다 필요한 것만 셰이더용 힙의 연속된 칸에 모아 붙인다.
//
//   단계                     | 함수                                                | 시점
//   원본 디스크립터 생성       | CreateConstantBufferView (ConstantBuffer::CreateView) | 초기화 때 1번
//   셰이더용 힙에 모으기       | CopyDescriptors (SetCBV)                             | draw 마다
//   모은 테이블 바인딩         | SetGraphicsRootDescriptorTable (CommitTable)         | draw 마다
//
//[시그니처]
// void ID3D12Device::CopyDescriptors(
//     UINT NumDestDescriptorRanges,                                  // 목적지 범위 개수
//     const D3D12_CPU_DESCRIPTOR_HANDLE* pDestDescriptorRangeStarts, // 각 목적지 범위의 시작 핸들
//     const UINT* pDestDescriptorRangeSizes,                         // 각 목적지 범위의 칸 수 (nullptr 이면 모두 1)
//     UINT NumSrcDescriptorRanges,                                   // 원본 범위 개수
//     const D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorRangeStarts,  // 각 원본 범위의 시작 핸들
//     const UINT* pSrcDescriptorRangeSizes,                          // 각 원본 범위의 칸 수 (nullptr 이면 모두 1)
//     D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType);               // 힙 타입 (원본/목적지 동일)
// 동작 : 원본 범위들을 한 줄로 이어 붙인 뒤 그 순서대로 목적지 범위들에 채워 넣는다.
//        -> 원본 칸 수의 합 == 목적지 칸 수의 합 이어야 한다.
//
//[위 SetCBV 의 호출] 1칸 -> 1칸 복사 (가장 단순한 경우)
//   목적지 : 범위 1개, 1칸 (현재 그룹의 reg 자리)
//   원본   : 범위 1개, 1칸 (_cbvDescriptorHeap 의 i번 칸)
//   아래 간단 버전과 결과가 같다.
//   DEVICE->CopyDescriptorsSimple(1, destHandle, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
//
//[여러 범위 예] 흩어진 CBV 3개를 그룹 시작부터 연속 3칸(b0, b1, b2)에 한 번에 복사
//   D3D12_CPU_DESCRIPTOR_HANDLE dst    = groupStart;           UINT dstSize = 3;
//   D3D12_CPU_DESCRIPTOR_HANDLE src[3] = { cbv7, cbv2, cbv9 };  // 서로 떨어진 칸
//   DEVICE->CopyDescriptors(1, &dst, &dstSize, 3, src, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
//   결과 : group[b0]=cbv7, group[b1]=cbv2, group[b2]=cbv9
//   -> 레지스터가 많아지면 SetCBV 를 레지스터마다 부르는 대신 이렇게 한 번에 모아 복사할 수 있다.
//
//[규칙 / 주의]
// 1) 즉시 실행 : Device 함수라 커맨드 리스트에 기록되지 않고 호출 순간 CPU 에서 복사가 끝난다.
//    -> GPU 가 아직 읽지 않은 칸을 덮어쓰면 안 된다 (draw 마다 새 그룹을 쓰는 이유)
// 2) 원본과 목적지의 힙 타입이 같아야 한다 (CBV_SRV_UAV 끼리, SAMPLER 끼리)
// 3) 원본은 CPU 전용(FLAG_NONE) 힙이 좋다. SHADER_VISIBLE 힙의 CPU 쪽 메모리는 write-combined 라
//    CPU 가 읽으면 매우 느리다 -> _cbvDescriptorHeap 을 FLAG_NONE 으로 만든 이유
// 4) 원본 범위와 목적지 범위가 겹치면 안 된다.
// 5) 디스크립터만 복사한다. 복사된 디스크립터는 원본과 같은 버퍼 칸을 가리키고,
//    데이터는 여전히 PushDataToTable 의 memcpy 로 쓴다.
//
//[CreateConstantBufferView 를 직접 부르지 않고 복사하는 이유]
// - draw 마다 셰이더용 힙에 Create*View 를 직접 호출해도 결과는 같다.
// - 하지만 텍스처 SRV 같은 디스크립터는 로드 때 한 번만 만들어 두고 재사용하고, draw 때는 복사만 하면 된다.
// - 원본을 CPU 전용 힙에 두면 셰이더용 힙은 매 프레임 비우고 다시 채우는 임시 작업 공간이 된다 (Clear() 의 역할)

//[목적] 현재 그룹의 시작 GPU 주소를 루트 파라미터 0번(Descriptor Table)에 바인딩하고 다음 그룹으로 넘어간다.
// SetCBV 로 필요한 레지스터를 다 채운 뒤, DrawInstanced 직전에 호출한다.
// 셰이더는 이 시작 주소부터 b0, b1, ... 순서로 연속된 디스크립터를 읽는다.
void TableDescriptorHeap::CommitTable()
{
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = _tableDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += _currentGroupIndex * _groupSize;
	CMD_LIST->SetGraphicsRootDescriptorTable(0, gpuHandle); // root parameter index 0에 테이블을 바인딩
	_currentGroupIndex++; //다음 draw call 은 새 그룹 사용 (GPU 가 아직 읽지 않은 그룹을 덮어쓰지 않기 위해)
}

//[목적] CBV_REGISTER enum 을 슬롯 번호로 바꿔 private 버전에 넘기는 공개용 함수.
// 나중에 SRV(t#) 같은 다른 레지스터 종류가 추가되어도 같은 private 함수를 공유할 수 있다.
D3D12_CPU_DESCRIPTOR_HANDLE TableDescriptorHeap::GetCPUHandle(CBV_REGISTER reg)
{
	return GetCPUHandle(static_cast<uint32>(reg));
}


//[목적] 현재 그룹 안에서 reg 번째 디스크립터의 CPU 주소를 계산한다. (SetCBV 의 복사 목적지)
// 힙 시작 + 그룹 번호 * 그룹 크기 + 레지스터 번호 * 디스크립터 1개 크기
D3D12_CPU_DESCRIPTOR_HANDLE TableDescriptorHeap::GetCPUHandle(uint32 reg)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = _tableDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += _currentGroupIndex * _groupSize;
	handle.ptr += reg * _handleSize;
	return handle;
}
