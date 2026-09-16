#pragma once
//[계약서 / 결재]
// 외주를 맡길 때 CPU에서 GPU 에 리소스를 예약등 할때 서명한다.
class RootSignature 
{
public:
	void Init(ComPtr<ID3D12Device> device);
	ComPtr<ID3D12RootSignature> GetSignature() { return _signature; }
private:
	ComPtr<ID3D12RootSignature> _signature;

};

