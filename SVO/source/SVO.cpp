#pragma warning(disable: 26812)
#pragma warning(disable: 26819)

#include <stdio.h>
#include <d3d11.h>

#include "SDL2/SDL_syswm.h"
#include "SDL2/SDL.h"
#undef main

int main()
{
	printf("Starting Up...\n");

	const int32_t width = 1280;
	const int32_t height = 720;

	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* window = SDL_CreateWindow("SVO Renderer", 10, 100, width, height, 0);
	SDL_SetWindowFullscreen(window, 0);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	SDL_SysWMinfo sysWMInfo;
	SDL_VERSION(&sysWMInfo.version);
	SDL_GetWindowWMInfo(window, &sysWMInfo);

	IDXGISwapChain* swapChain;
	ID3D11Device* dev;
	ID3D11DeviceContext* devCon;
	ID3D11RenderTargetView* backbuffer;

	// create a struct to hold information about the swap chain
	DXGI_SWAP_CHAIN_DESC scd;

	// clear out the struct for use
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	// fill the swap chain description struct
	scd.BufferCount = 1;                                    // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
	scd.OutputWindow = sysWMInfo.info.win.window;           // the window to be used
	scd.SampleDesc.Count = 4;                               // how many multisamples
	scd.Windowed = TRUE;                                    // windowed/full-screen mode

	// create a device, device context and swap chain using the information in the scd struct
	D3D11CreateDeviceAndSwapChain(NULL,
								  D3D_DRIVER_TYPE_HARDWARE,
								  NULL,
								  NULL,
								  NULL,
								  NULL,
								  D3D11_SDK_VERSION,
								  &scd,
								  &swapChain,
								  &dev,
								  NULL,
								  &devCon);

	// get the address of the back buffer
	ID3D11Texture2D* pBackBuffer;
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	// use the back buffer address to create the render target
	dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
	pBackBuffer->Release();

	// set the render target as the back buffer
	devCon->OMSetRenderTargets(1, &backbuffer, NULL);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = width;
	viewport.Height = height;

	devCon->RSSetViewports(1, &viewport);

	while (true)
	{
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
				case SDL_QUIT:
				{
				    exit(0);
				    break;
				}

				default:
				{
				    break;
				}
            }
        }

		const FLOAT color[4] = { 0.5f, 0.0f, 0.0f, 1.0f };
		devCon->ClearRenderTargetView(backbuffer, color);
		swapChain->Present(0, 0);
	}

	SDL_DestroyWindow(window);

	swapChain->Release();
	backbuffer->Release();
	dev->Release();
	devCon->Release();

	return 0;
}
