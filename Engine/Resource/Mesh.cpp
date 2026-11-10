#include "../Utils/pch.h"
#include "Mesh.h"
#include "../Engine/Engine.h"


void Mesh::Init(const std::vector<Vertex>& vec, const std::vector<uint32>& indices)
{
	CreateVertexBuffer(const_cast<std::vector<Vertex>&>(vec));
	CreateIndexBuffer(const_cast<std::vector<uint32>&>(indices));

	

}

void Mesh::Render()
{
	CMD_LIST->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CMD_LIST->IASetVertexBuffers(0, 1, &_vetexBufferView);
	CMD_LIST->IASetIndexBuffer(&_indexBufferView);

	//ram -> gpu ram -> gpu resgister (에 gpu ram 주소값만 셋팅)
	//GEngine->GetConstantBuffer()->PushData(0, &_transform, sizeof(Transform));
	//GEngine->GetConstantBuffer()->PushData(1, &_transform, sizeof(Transform));
	
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = GEngine->GetConstantBuffer()->PushDataToTable(&_transform, sizeof(Transform));
		GEngine->GetTableDescriptorHeap()->SetCBV(handle, CBV_REGISTER::b0);
	}
	
	GEngine->GetTableDescriptorHeap()->CommitTable();	
	//CMD_LIST->
	//CMD_LIST->DrawInstanced(_vertexCount,1,0,0);
	CMD_LIST->DrawIndexedInstanced(_indexCount, 1, 0, 0, 0);

}

void Mesh::CreateVertexBuffer(std::vector<Vertex>& vec)
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

void Mesh::CreateIndexBuffer(std::vector<uint32>& buffer)
{
	_indexCount = static_cast<uint32>(buffer.size());
	uint32 bufferSize = _indexCount * sizeof(uint32);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD); //D3D12_HEAP_TYPE_UPLOAD
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	//gpu 버퍼 할당
	DEVICE->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_indexBuffer)
	);

	void* indexDataBuffer = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	_indexBuffer->Map(0, &readRange, &indexDataBuffer); //gpu 쓰기시작
	::memcpy(indexDataBuffer, &buffer[0], bufferSize);
	_indexBuffer->Unmap(0, nullptr);

	_indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
	_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	_indexBufferView.SizeInBytes = bufferSize;
}
