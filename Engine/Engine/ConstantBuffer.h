#pragma once


//역할 : cpu / ram 에서 먼저 버퍼를 쪼개서 할당하고
// gpu / vram 에서 커맨드리스트 하나 당 위의 버퍼를 하나씩 가져다가 사용한다.
class ConstantBuffer
{
public:
	ConstantBuffer();
	~ConstantBuffer();

	void Init(uint32 size, uint32 count);
	void PushData(uint32 rootParamIndex, void* buffer, uint32 size);
	void Clear();
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32 index);
private:
	void CreateBuffer();

private:
	uint32 _elementSize = 0;
	uint32 _elementCount = 0;
	BYTE* _mappedData = nullptr;
	ComPtr<ID3D12Resource> _cbvBuffer;

	uint32 _currentIndex = 0;
};