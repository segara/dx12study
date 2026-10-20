#include "../Utils/pch.h"
#include "ConstantBuffer.h"
#include "Engine.h"
ConstantBuffer::ConstantBuffer()
{
}
ConstantBuffer::~ConstantBuffer()
{
	if(_cbvBuffer)
	{
		if(_cbvBuffer != nullptr)
			_cbvBuffer->Unmap(0, nullptr);
		_cbvBuffer = nullptr;
	}
}
void ConstantBuffer::Clear()
{
	_currentIndex = 0;
}
void ConstantBuffer::Init(uint32 size, uint32 count)
{
	//상수 버퍼는 256바이트 단위로 정렬되어야 한다.
	// 따라서 size를 256의 배수로 올림한다.
	_elementSize = (size + 255) & ~255;
	_elementCount = count;
	CreateBuffer();
}

void ConstantBuffer::PushData(uint32 rootParamIndex, void* buffer, uint32 size)
{
	assert(size <= _elementSize);

	::memcpy(&_mappedData[_currentIndex * _elementSize], buffer, size);
	D3D12_GPU_VIRTUAL_ADDRESS address = GetGPUVirtualAddress(_currentIndex);
	CMD_LIST->SetGraphicsRootConstantBufferView(rootParamIndex, address); //rootParamIndex는 root signature에서 상수 버퍼(b0, b1)를 바인딩한 인덱스입니다.
	_currentIndex++;
}

D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetGPUVirtualAddress(uint32 index)
{
	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = _cbvBuffer->GetGPUVirtualAddress();
	objCBAddress += index * _elementSize;
	return objCBAddress;
}

void ConstantBuffer::CreateBuffer()
{
	uint32 bufferSize = _elementSize * _elementCount;
	D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
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

	//	그래서 memcpy(_mappedData, ...)로 쓰는 순간 데이터는 이미 GPU가 읽을 그 메모리에 들어가 있습니다.복사 명령도, Flush도, Unmap도 필요 없습니다.업로드 힙은 캐시 일관성이 보장되기 때문입니다.

	//	그럼 GPU는 언제 그 값을 읽나

	//	GPU가 해당 드로우콜을 실제로 실행하는 순간에 셰이더가 그 주소에서 값을 읽습니다.이 시점은 CPU가 명령을 기록한 시점보다 한참 뒤입니다.
}

