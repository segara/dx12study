#include "../Utils/pch.h"
#include "RootSignature.h"

void RootSignature::Init(ComPtr<ID3D12Device> device)
{
	//root constant buffer view  사용
	// Root CBV는 상수 버퍼의 GPU 가상 주소를 루트 시그니처에 
	// 직접 넣어 바인딩하는 방식입니다. 
	// 루트 파라미터 타입은 D3D12_ROOT_PARAMETER_TYPE_CBV이고, 앞에서 다룬 Root Descriptor 중 하나입니다.

		//크기	2 DWORD(64비트 GPU 주소)
		//디스크립터 힙	필요 없음.CreateConstantBufferView도 호출하지 않습니다
		//넘기는 값	ID3D12Resource::GetGPUVirtualAddress() + 오프셋
		//정렬	주소가 256바이트 단위로 정렬되어야 합니다
		//크기 정보	없음.주소만 넘기므로 범위 검사를 하지 않습니다
		//HLSL	cbuffer X : register(b#) 또는 ConstantBuffer<T>
	
	//동작 구조
	//	[Descriptor Table 방식]
	//	루트 시그니처 → 디스크립터 힙 → CBV 디스크립터 → 버퍼
	//	(간접 참조 2단계)

	//	[Root CBV 방식]
	//	루트 시그니처 → 버퍼
	//	(간접 참조 1단계)

	//D3D12_ROOT_PARAMETER는 루트 시그니처를 이루는 파라미터 하나
	//(슬롯 하나)를 정의하는 D3D12 기본 구조체입니다.

	CD3DX12_ROOT_PARAMETER rootParameter[2];
	rootParameter[0].InitAsConstantBufferView(0); //0번 -> b0 -> cbv
	rootParameter[1].InitAsConstantBufferView(1); //1번 -> b1 -> cbv

	//InitAsConstantBufferView는 뷰를 생성하지 않습니다. 루트 시그니처에 **”이 슬롯은 Root CBV 타입이고 b0에 연결된다"는 선언(레이아웃)**만 적는 함수
	//SetGraphicsRootConstantBufferView(N, addr)에서 실제 주소를 채움
	//	상수 버퍼란
	//	드로우콜 하나 동안 모든 스레드가 똑같이 읽는 작은 데이터입니다.예를 들면 World / View / Projection 행렬, 카메라 위치, 머티리얼 파라미터, 시간 값 같은 것들입니다.
	//	셰이더에서는 읽기만 할 수 있습니다.
	//	HLSL 레지스터** b#** 에 바인딩됩니다.
	
	D3D12_ROOT_SIGNATURE_DESC sigDesc = CD3DX12_ROOT_SIGNATURE_DESC(2, rootParameter); //서명
	//입력 조립기 단계를 사용하겠다.
	sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; //입력 조립기 단계
	ComPtr<ID3DBlob> blobSignature;
	ComPtr<ID3DBlob> blobError;
	::D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blobSignature, &blobError);
	device->CreateRootSignature(0, blobSignature->GetBufferPointer(), blobSignature->GetBufferSize(), IID_PPV_ARGS(&_signature));
}

//주소가 256바이트 단위로 정렬되어야 합니다 의 의미는 ?
//GPU 주소 값이 256의 배수여야 한다는 뜻입니다.즉 주소 % 256 == 0을 만족해야 합니다.
//
//예시
//0x10000   (65536)  → 256 × 256      ✅ 가능
//0x10100   (65792)  → 256 × 257      ✅ 가능
//0x10200   (66048)  → 256 × 258      ✅ 가능
//0x10050   (65616)  → 256으로 나누어떨어지지 않음 ❌ 불가
//
//16진수로 보면 주소의 마지막 두 자리가 00이어야 합니다. 256 = 0x100이기 때문입니다.
//
//실제로 문제가 되는 상황
//
//버퍼의 시작 주소(GetGPUVirtualAddress())는 리소스 생성 시 64KB 단위로 정렬되므로 원래 조건을 만족합니다.문제는 오프셋입니다.
//
//cpp
//struct PerObject {        // sizeof = 80 바이트
//	float4x4 World;       // 64
//	float4   Color;       // 16
//};
//
//// ❌ 잘못된 예: 80바이트 간격으로 배치
//addr = base + i * 80;     // i=1 → base+80, 256의 배수가 아님
//
//// ✅ 올바른 예: 256바이트 간격으로 배치
//UINT stride = (sizeof(PerObject) + 255) & ~255;  // 80 → 256
//addr = base + i * stride; // base+0, base+256, base+512 ...
//(size + 255) & ~255 해석
//
//크기를 256의 배수로 올림하는 비트 연산입니다.
//
//+ 255: 다음 256 배수를 넘기도록 올립니다.
//& ~255 : 하위 8비트를 0으로 만들어 256 배수로 내립니다.
//왜 256인가
//
//GPU 하드웨어가 상수 데이터를 일정 크기 블록 단위로 가져오도록 설계되어 있어서, D3D12가 그 최소 단위를 규칙으로 정해 둔 것입니다.상수 이름은 D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT(= 256)입니다.
//
//헷갈리기 쉬운 것 : 16바이트 규칙
//
//HLSL cbuffer 내부 멤버의 16바이트(float4) 패킹 규칙은 별개입니다.
//
//256바이트 정렬 : 버퍼가 어디서 시작하느냐(주소) 문제입니다.
//16바이트 패킹 : 버퍼 안에서 변수를 어떻게 배치하느냐(레이아웃) 문제입니다.