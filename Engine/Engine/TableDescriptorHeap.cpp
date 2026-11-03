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
