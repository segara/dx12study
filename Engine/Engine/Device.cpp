#include "../Utils/pch.h"
#include "Device.h"

void Device::Init()
{
#ifdef _DEBUG
    HRESULT hr = S_OK;
    // D3D12 SDK Layers가 없을 수 있으므로 반환값을 체크합니다.
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(&_debugController));
    if (SUCCEEDED(hr) && _debugController)
    {
        _debugController->EnableDebugLayer();
    }
    else
    {
        // SDK Layers가 설치되어 있지 않으면 디버그 레이어를 건너뜁니다.
        OutputDebugStringA("경고: D3D12 SDK Layers가 시스템에 없습니다. EnableDebugLayer()를 건너뜁니다.\n");
        // _debugController는 빈 ComPtr 상태로 남습니다(안전).
    }
#endif
	::CreateDXGIFactory(IID_PPV_ARGS(&_dxgi));
	::D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device));
}
