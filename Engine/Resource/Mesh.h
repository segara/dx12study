#pragma once

class Mesh 
{
public:
	void Init(const std::vector<Vertex>& vec, const std::vector<uint32>& indices);
	
	void Render();
	void SetTransform(const Transform& transform) { _transform = transform; }
private:
	void CreateVertexBuffer(std::vector<Vertex>& vec);
	void CreateIndexBuffer(std::vector<uint32>& buffer);

private:
	ComPtr<ID3D12Resource> _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vetexBufferView = {};
	uint32 _vertexCount = 0;

	ComPtr<ID3D12Resource> _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView = {};
	uint32 _indexCount = 0;
	Transform _transform = {};
};

