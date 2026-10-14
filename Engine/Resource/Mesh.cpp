#include "../Utils/pch.h"
#include "Mesh.h"
#include "../Engine/Engine.h"

void Mesh::Init(std::vector<Vertex>& vec)
{
	_vertexCount = static_cast<uint32>(vec.size());
	uint32 bufferSize = _vertexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD); //D3D12_HEAP_TYPE_UPLOAD
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	//gpu 버퍼 할당
	DEVICE->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_vertexBuffer)
	);

	void* vertexDataBuffer = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	_vertexBuffer->Map(0, &readRange, &vertexDataBuffer); //gpu 쓰기시작
	::memcpy(vertexDataBuffer, &vec[0], bufferSize);
	_vertexBuffer->Unmap(0, nullptr);

	_vetexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vetexBufferView.StrideInBytes = sizeof(Vertex);
	_vetexBufferView.SizeInBytes = bufferSize;


}

void Mesh::Render()
{
	CMD_LIST->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CMD_LIST->IASetVertexBuffers(0, 1, &_vetexBufferView);

	//ram -> gpu ram -> gpu resgister (에 gpu ram 주소값만 셋팅)
	//CMD_LIST->
	CMD_LIST->DrawInstanced(_vertexCount,1,0,0);

}
