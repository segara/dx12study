#pragma once

//[TableDescriptorHeap 용도]
// Root Descriptor Table 방식으로 CBV 를 셰이더에 넘기기 위한 "셰이더가 볼 수 있는(SHADER_VISIBLE)" 디스크립터 힙.
//
// - ConstantBuffer 가 가진 _cbvHeap 은 CPU 전용(FLAG_NONE) 힙이라 셰이더가 직접 읽을 수 없다.
//   -> draw call 마다 필요한 CBV 디스크립터를 이 힙으로 복사(CopyDescriptors)한 뒤 테이블로 바인딩한다.
// - CBV_SRV_UAV 타입 힙은 한 번에 하나만 바인딩할 수 있고 SetDescriptorHeaps 교체 비용이 크다.
//   -> 큰 힙 하나를 만들어 draw call 마다 "그룹" 단위로 잘라 쓴다.
//
//[힙 레이아웃]  (count = 그룹 수, REGISTER_COUNT = 한 그룹에 들어가는 레지스터 수 b0~b4)
// [ group0: b0 b1 b2 b3 b4 ][ group1: b0 b1 b2 b3 b4 ] ... [ group(count-1) ]
//    ↑ draw call 1회 = 그룹 1개. 한 그룹 안의 디스크립터는 반드시 연속이어야 한다
//      (RootSignature 의 Descriptor Table 이 "시작 위치부터 b0~b4 를 연속으로 읽는다"고 선언했기 때문)
//
//[다른 클래스와의 관계]
// RootSignature  : rootParameter[0] = Descriptor Table (CBV b0~b4). CommitTable 이 이 0번 슬롯에 그룹 시작 주소를 넣음
// ConstantBuffer : PushDataToTable 이 데이터를 쓰고 그 CBV 의 CPU 핸들(원본)을 돌려줌 -> SetCBV 의 srcHandle
// CommandQueue   : RenderBegin 에서 Clear() + SetDescriptorHeaps 로 이 힙을 등록
// Mesh           : draw call 마다 SetCBV -> CommitTable -> DrawInstanced
// Engine         : 소유 및 Init(256)
//
//[사용 방법]
// 1) 초기화 (Engine::Init)
//      _tableDescriptorHeap->Init(256);                  //프레임당 최대 256번의 draw call
// 2) 프레임 시작 (CommandQueue::RenderBegin)
//      GEngine->GetTableDescriptorHeap()->Clear();       //그룹 인덱스 0으로
//      CMD_LIST->SetDescriptorHeaps(1, heaps);           //이 힙을 커맨드 리스트에 등록
// 3) draw call 마다 (Mesh::Render)
//      handle = GEngine->GetConstantBuffer()->PushDataToTable(&data, sizeof(data));
//      GEngine->GetTableDescriptorHeap()->SetCBV(handle, CBV_REGISTER::b0);   //필요한 레지스터만큼 반복
//      GEngine->GetTableDescriptorHeap()->CommitTable(); //그룹 바인딩 + 다음 그룹으로 이동
//      CMD_LIST->DrawInstanced(...);
//
//[주의]
// - CopyDescriptors 는 커맨드 리스트 명령이 아니라 CPU 에서 즉시 실행된다.
//   GPU 가 아직 읽지 않은 그룹을 덮어쓰면 안 되므로 draw call 마다 새 그룹을 쓰고,
//   Clear 는 WaitSync 로 이전 프레임 GPU 작업이 끝난 뒤(RenderBegin)에만 한다.
// - 그룹 수(count)를 넘는 draw call 에 대한 범위 검사가 없다.
//




//=====================================================================================
//[GPU 가 b0 를 읽는 흐름] 루트 시그니처 -> 디스크립터 힙 -> CBV 디스크립터 -> 버퍼 (간접 참조 2단계)
//=====================================================================================

// 루트 파라미터 --(1)--> 디스크립터 힙의 한 칸 --(2)--> 그 칸의 CBV 디스크립터 --(3)--> 실제 버퍼 데이터
// "테이블 시작"         "b0 자리"                  "버퍼 주소, 크기"               "offset0 값"
//
// 단계                 | 들어있는 값                    | 누가 채우나
// 루트 파라미터 0번     | 테이블 시작 GPU 핸들            | CommitTable (SetGraphicsRootDescriptorTable)
// 디스크립터 힙의 칸    | CBV 디스크립터 (32~64B 구조체)  | SetCBV (CopyDescriptors)
// CBV 디스크립터 내용   | BufferLocation, SizeInBytes    | ConstantBuffer::CreateView (Init 때 미리 생성)
// 버퍼                 | Transform 데이터               | ConstantBuffer::PushDataToTable (memcpy)<-이 줄이 끝나는 순간 데이터는 이미 GPU 가 읽을 메모리 안에 있음
//
// (0) 약속 : RootSignature 가 CD3DX12_DESCRIPTOR_RANGE(CBV, CBV_REGISTER_COUNT, 0) 으로
//           "테이블 시작부터 연속 5칸 = b0, b1, b2, b3, b4" 라고 선언
//           -> 셰이더 레지스터 번호 = 테이블 안에서의 칸 위치 (b1 = 시작 + 1칸)
// (1) 루트 파라미터 -> 힙의 칸 : 셰이더가 b1 을 읽으면 GPU 는 gpuHandle + 1 * 디스크립터 크기 이동
//           -> GPU 가 직접 읽으므로 이 힙은 SHADER_VISIBLE 이어야 하고, SetDescriptorHeaps 로 등록돼 있어야 한다
// (2) 힙의 칸 -> CBV 디스크립터 : 칸에 적힌 명세서 { BufferLocation = elem i 의 GPU 주소, SizeInBytes = 256 }
//           원본은 ConstantBuffer::_cbvHeap(CPU 전용)에 있고, SetCBV 가 이 힙으로 복사해 온다
// (3) CBV 디스크립터 -> 버퍼 : BufferLocation 으로 가서 cbuffer 안의 오프셋 위치를 읽는다
//           (offset0 은 TEST_B0 의 첫 멤버라 오프셋 0). SizeInBytes 가 있어 범위 검사가 가능
//
//[실제 숫자 예시] Game 에서 메시 2번 draw, inc = 디스크립터 1개 크기, cbBase = 상수 버퍼 시작 주소
// CPU 기록 (Mesh::Render x2)
//   draw0 : elem0 -> CBV0 -> 테이블힙[0](b0),  elem1 -> CBV1 -> 테이블힙[1](b1),  루트0 = gpuStart + 0*groupSize
//   draw1 : elem2 -> CBV2 -> 테이블힙[5](b0),  elem3 -> CBV3 -> 테이블힙[6](b1),  루트0 = gpuStart + 1*groupSize (= +5*inc)
// GPU 실행 (draw1 의 VS 가 offset0 을 읽을 때)
//   (1) 루트0 = gpuStart + 5*inc,  b0 -> +0칸 -> 테이블힙[5]
//   (2) 테이블힙[5] = CBV2 { BufferLocation = cbBase + 2*256, Size = 256 }
//   (3) cbBase + 512 + 0 -> float4 offset0 = draw1 의 Transform.offset
//
//  Root arg      Table heap (SHADER_VISIBLE)                  Upload buffer (_cbvBuffer)
//  [param0]--+   [0]CBV0 [1]CBV1 [2] [3] [4] | [5]CBV2 [6]CBV3   [elem0][elem1][elem2][elem3]
//            +---------------------------------^                                ^
//                                              +-- CBV2.BufferLocation ---------+
//
//[값이 확정되는 시점]
// 루트 파라미터(테이블 시작 위치) : 커맨드 기록 시점에 커맨드 리스트에 저장 -> draw 마다 다른 값이어도 각각 유지
// 힙 칸의 디스크립터         : GPU 실행 시점에 읽음 -> 실행 전에 덮어쓰면 안 됨 -> draw 마다 새 그룹
// 버퍼 데이터               : GPU 실행 시점에 읽음 -> draw 마다 새 elem
// Clear() 로 0 부터 재사용해도 되는 이유 = RenderEnd 의 WaitSync 가 이전 프레임 GPU 읽기 완료를 보장
//
//[간접 참조를 2번이나 거치는 이유] Root CBV(1단계) 와 비교
//                  | Root CBV                        | Descriptor Table
// 루트 시그니처 크기 | CBV 1개당 2 DWORD               | 테이블 1개당 1 DWORD (5개든 100개든)
// 지원 리소스       | 버퍼만 (CBV, 버퍼 SRV/UAV)       | 텍스처까지 모두
// 크기 정보         | 없음 (범위 검사 불가)            | 디스크립터에 SizeInBytes 있음
// 비용             | 간접 참조 1단계                  | 간접 참조 2단계 + 디스크립터 복사
// 텍스처는 포맷/밉맵/차원 정보가 필요해 주소 하나로 표현할 수 없고 반드시 디스크립터가 필요하다.
// -> 텍스처(t0)를 추가하면 이 테이블에 SRV 범위가 붙는다.
class TableDescriptorHeap
{
public:
	void Init(uint32 count); //count : 테이블에 들어갈 그룹의 개수
	void Clear();
	void SetCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, CBV_REGISTER reg);
	void CommitTable(); //테이블을 GPU에 바인딩

	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() const { return _tableDescriptorHeap; } //SetDescriptorHeaps 에 등록할 힙

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(CBV_REGISTER reg);


private:
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32 reg);
private:

	ComPtr<ID3D12DescriptorHeap> _tableDescriptorHeap;	//SHADER_VISIBLE 힙 (그룹 수 * REGISTER_COUNT 개)
	uint64 _handleSize = 0;		//디스크립터 1개의 크기 (GetDescriptorHandleIncrementSize, GPU마다 다름)
	uint64 _groupSize = 0; //groupSize : 한 그룹이 차지하는 핸들 크기, 그룹은 하나 테이블에 들어가는 CBV의 개수만큼의 핸들 크기
	uint64 _groupCount = 0;		//그룹 개수 = 프레임당 최대 draw call 수
	uint32 _currentGroupIndex = 0;	//이번 draw call 이 사용할 그룹 번호 (CommitTable 마다 +1, Clear 시 0)

};

