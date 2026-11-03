#pragma once
// 루트 시그니처(Root Signature)
// 1. 개념
// 셰이더를 함수라고 보면, 루트 시그니처는 그 함수의 매개변수 목록입니다.

// 셰이더는 cbuffer, 텍스처, 샘플러 같은 리소스를 레지스터 번호(b0, t0, s0 등)로 읽습니다. 
// 루트 시그니처는 이 레지스터들에 무엇이 어떤 방식으로 연결되는지를 미리 정해 둔 레이아웃입니다. 

// 1. 데이터 구조체 (설계도, CPU 메모리)
// d3d12.h에 정의된 C 구조체들입니다.

// D3D12_ROOT_SIGNATURE_DESC            ← 루트 시그니처 전체 설명서
//  ├ NumParameters / pParameters ──▶ D3D12_ROOT_PARAMETER[]   ← 슬롯 하나
//  │     ParameterType (CBV / SRV / UAV / 32BIT_CONSTANTS / DESCRIPTOR_TABLE)
//  │     union {
//  │        D3D12_ROOT_CONSTANTS         Constants;        ← Root Constant
//  │        D3D12_ROOT_DESCRIPTOR        Descriptor;       ← Root CBV/SRV/UAV 
//  │        D3D12_ROOT_DESCRIPTOR_TABLE  DescriptorTable;  ──▶ D3D12_DESCRIPTOR_RANGE[]
//  │     }
//  │     ShaderVisibility
//  ├ NumStaticSamplers / pStaticSamplers ──▶ D3D12_STATIC_SAMPLER_DESC[]
//  └ Flags (D3D12_ROOT_SIGNATURE_FLAGS)

// 2. 직렬화된 바이너리 (ID3DBlob)
// D3D12SerializeRootSignature가 위 구조체를 드라이버가 읽을 수 있는 바이트코드로 바꿉니다. 셰이더 바이트코드와 같은 성격의 데이터라서 파일로 저장하거나 셰이더 안에 포함할 수도 있습니다.

// 3. 실제 GPU 객체 (ID3D12RootSignature)
// ID3D12Device::CreateRootSignature가 위 blob으로 만드는 COM 인터페이스입니다. 이것이 "루트 시그니처"라는 이름을 가진 진짜 객체입니다.

// 내부가 감춰진 불투명 객체이고, 자체 메서드가 거의 없습니다. 상속받은 GetDevice, SetName 정도만 있습니다.
// 다음 두 곳에 넘겨서 씁니다.
// D3D12_GRAPHICS_PIPELINE_STATE_DESC::pRootSignature (PSO 생성)
// ID3D12GraphicsCommandList::SetGraphicsRootSignature (커맨드 리스트)
// 만들어진 객체에서 설명서를 다시 읽어 오려면 D3D12CreateRootSignatureDeserializer를 씁니다.

class RootSignature 
{
public:
	void Init(ComPtr<ID3D12Device> device);
	ComPtr<ID3D12RootSignature> GetSignature() { return _signature; }
private:
	ComPtr<ID3D12RootSignature> _signature;

};

