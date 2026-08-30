#pragma once
//외주결과물을 어디에 받지?
//어떤종이(buffer)에 그려서 건내달라고 부탁해보자
//특수종이를 만들어서 처음에 건내주고 결과물을 해당 종이에 받는다
// 현재 화면을 출력화는 와중에 다음화면을 외주에 맡겨야함
//외냐하면 현재 화면 결과물을 출력중이기 때문

class SwapChain
{
public:
	void Init(const WindowInfo& info, ComPtr<IDXGIFactory> dxgi, ComPtr<ID3D12CommandQueue> cmdQueue);
private:
	ComPtr<IDXGISwapChain> _swapChain;
	ComPtr<ID3D12Resource> _renderTargets[SWAP_CHAIN_BUFFER_COUNT];
	uint32				   _backBufferIndex = 0;
};