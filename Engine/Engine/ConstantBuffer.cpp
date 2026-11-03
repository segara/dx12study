#include "../Utils/pch.h"
#include "ConstantBuffer.h"
#include "Engine.h"
ConstantBuffer::ConstantBuffer()
{
}
//[목적] CreateBuffer 에서 Map 해둔 버퍼를 Unmap 하고 해제한다.
ConstantBuffer::~ConstantBuffer()
{
	if(_cbvBuffer)
	{
		if(_cbvBuffer != nullptr)
			_cbvBuffer->Unmap(0, nullptr);
		_cbvBuffer = nullptr;
	}
}
//[목적] 매 프레임 시작 시 칸 인덱스를 0으로 되돌려 버퍼를 처음부터 재사용한다.
// CommandQueue::RenderBegin 에서 호출 (이전 프레임 GPU 작업은 WaitSync 로 이미 끝난 상태)
void ConstantBuffer::Clear()
{
	_currentIndex = 0;
}

//[목적] 칸 크기/개수를 정하고 버퍼와 CBV 디스크립터를 한 번에 만든다.
// size  : 칸 하나에 담을 데이터 크기 (예: sizeof(Transform))
// count : 칸 개수 = 프레임당 최대 Push 횟수
void ConstantBuffer::Init(uint32 size, uint32 count)
{
	//상수 버퍼는 256바이트 단위로 정렬되어야 한다.
	// 따라서 size를 256의 배수로 올림한다.
	_elementSize = (size + 255) & ~255;
	_elementCount = count;
	CreateBuffer();
	CreateView();
}

//[목적] ver1. Root CBV 방식 - 데이터를 다음 칸에 쓰고 그 GPU 주소를 루트 파라미터에 직접 바인딩한다.
// rootParamIndex : RootSignature 의 루트 파라미터 인덱스 (InitAsConstantBufferView 로 선언된 슬롯이어야 함)
// 현재 RootSignature 는 Descriptor Table 방식이라 이 함수를 쓰면 슬롯 타입이 맞지 않는다.
void ConstantBuffer::PushData(uint32 rootParamIndex, void* buffer, uint32 size)
{
	assert(size <= _elementSize);

	::memcpy(&_mappedData[_currentIndex * _elementSize], buffer, size);
	D3D12_GPU_VIRTUAL_ADDRESS address = GetGPUVirtualAddress(_currentIndex);
	CMD_LIST->SetGraphicsRootConstantBufferView(rootParamIndex, address); //rootParamIndex는 root signature에서 상수 버퍼(b0, b1)를 바인딩한 인덱스입니다.
	_currentIndex++;
}


//[목적] ver2. Descriptor Table 방식 - 데이터를 다음 칸에 쓰고 그 칸을 가리키는 CBV 핸들을 돌려준다.
// 바인딩은 하지 않는다. 돌려준 핸들을 TableDescriptorHeap::SetCBV 에 넘겨 셰이더용 힙으로 복사한다.
D3D12_CPU_DESCRIPTOR_HANDLE ConstantBuffer::PushDataToTable(void* buffer, uint32 size)
{
	assert(size <= _elementSize);

	::memcpy(&_mappedData[_currentIndex * _elementSize], buffer, size);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUDescriptorHandle(_currentIndex);
	_currentIndex++;
	return handle;
}

//[목적] index 번 칸의 GPU 가상 주소를 계산한다. (버퍼 시작 + index * 칸 크기)
// 칸 크기가 256 배수라 결과 주소도 256바이트 정렬을 만족한다.
D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetGPUVirtualAddress(uint32 index)
{
	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = _cbvBuffer->GetGPUVirtualAddress();
	objCBAddress += index * _elementSize;
	return objCBAddress;
}

//[목적] index 번 칸을 가리키는 CBV 디스크립터의 CPU 핸들을 계산한다. (힙 시작 + index * 디스크립터 크기)
D3D12_CPU_DESCRIPTOR_HANDLE ConstantBuffer::GetCPUDescriptorHandle(uint32 index)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(_cbvHeapCPUHandleStart, index * _handleincrementSize);
}

//[목적] 업로드 힙에 "칸 크기 * 칸 개수" 버퍼를 만들고, CPU 가 바로 쓸 수 있도록 Map 해둔다.
void ConstantBuffer::CreateBuffer()
{
	uint32 bufferSize = _elementSize * _elementCount;
	D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD); 
	//앞에서 업로드 힙은 GPU가 읽기에 느리다고 했는데, 그 이유가 이 통로입니다.
	//힙 타입	메모리 위치 (일반적)	GPU가 읽을 때	CPU가 쓸 때
	//UPLOAD	시스템 RAM	PCIe를 건너가서 읽음 → 느림	직접 씀 → 빠름
	D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	DEVICE->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_cbvBuffer));
	_cbvBuffer->Map(0, nullptr, reinterpret_cast<void**>(&_mappedData)); 
	//DirectX 12에서 업로드 힙에 만든 상수 버퍼(CBV용 버퍼)를 CPU 메모리 주소로 연결하는 코드입니다.이 호출이 끝나면 _mappedData 포인터로 GPU가 읽을 버퍼에 CPU가 직접 데이터를 쓸 수 있습니다.
	// We do not need to unmap until we are done with the resource.  However, we must not write to
	// the resource while it is in use by the GPU (so we must use synchronization techniques).

	//따로 "밀어 넣는" 시점은 없습니다.이 부분이 D3D12 업로드 힙의 핵심입니다.

	//	왜 따로 전송하지 않는가

	//	_cbvBuffer가 업로드 힙(D3D12_HEAP_TYPE_UPLOAD)에 만들어졌다면, 그 메모리는 CPU와 GPU가 같이 볼 수 있는 메모리입니다.보통은 시스템 RAM에 있고, GPU가 PCIe를 통해 직접 읽어 갑니다(ReBAR가 켜진 환경에서는 VRAM에 있고 CPU가 직접 쓰기도 합니다).
	// PCIe는 CPU 쪽(메인보드, 시스템 RAM)과 그래픽카드를 연결하는 고속 통로(버스)입니다. 이름은 Peripheral Component Interconnect Express의 줄임말입니다. 그래픽카드를 꽂는 메인보드의 긴 슬롯이 바로 PCIe x16 슬롯입니다.
	//	그래서 memcpy(_mappedData, ...)로 쓰는 순간 데이터는 이미 GPU가 읽을 그 메모리에 들어가 있습니다.복사 명령도, Flush도, Unmap도 필요 없습니다.업로드 힙은 캐시 일관성이 보장되기 때문입니다.

	//	그럼 GPU는 언제 그 값을 읽나

	//	GPU가 해당 드로우콜을 실제로 실행하는 순간에 셰이더가 그 주소에서 값을 읽습니다.이 시점은 CPU가 명령을 기록한 시점보다 한참 뒤입니다.
}

//[목적] Descriptor Table 방식용 - 칸마다 그 칸을 가리키는 CBV 디스크립터를 미리 만들어 둔다.
// 이 힙은 CPU 전용(FLAG_NONE) 이라 셰이더가 직접 읽지 못한다.
// draw call 마다 TableDescriptorHeap(SHADER_VISIBLE) 으로 복사해서 사용한다.
void ConstantBuffer::CreateView()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = _elementCount;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//CopyDescriptors 의 원본은 CPU 전용 힙이어야 한다 (SHADER_VISIBLE 힙은 CPU 에서 읽기가 매우 느림)
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DEVICE->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_cbvHeap));

	//**디스크립터 힙(Descriptor Heap)**은 디스크립터들을 담아 두는 연속된 배열 메모리입니다. 
	// 셰이더가 리소스(텍스처, 버퍼 등)에 접근하려면 그 리소스를 설명하는 디스크립터가 필요한데,
	// DX12에서는 이 디스크립터들을 앱이 힙에 직접 만들고 관리합니다.

	_cbvHeapCPUHandleStart = _cbvHeap->GetCPUDescriptorHandleForHeapStart();
	_handleincrementSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for(uint32 i = 0; i < _elementCount; ++i)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = GetGPUVirtualAddress(i);	//i 번 칸의 시작 주소 (이미 i * _elementSize 가 더해져 있음)
		cbvDesc.SizeInBytes = _elementSize;					//256 배수여야 함

		D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUDescriptorHandle(i);

		//CreateConstantBufferView는 디스크립터 힙의 특정 슬롯에 CBV 디스크립터를 기록해 둡니다.
		DEVICE->CreateConstantBufferView(&cbvDesc, handle);
	}


}

