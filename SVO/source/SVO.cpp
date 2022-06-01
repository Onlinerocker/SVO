#pragma warning(disable: 26812)
#pragma warning(disable: 26819)

#include <stdio.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <chrono>

#include "glm/glm.hpp"

#include "imgui.h"
#include "ImGuiBackend/imgui_impl_sdl.h"
#include "ImGuiBackend/imgui_impl_dx11.h"

#include "SDL2/SDL_syswm.h"
#include "SDL2/SDL.h"
#undef main

struct Vertex
{
	glm::vec2 pos;
};

struct CameraInfo
{
	glm::vec3 pos;
};

int main()
{
	printf("Starting Up...\n");

	//Application info
	const int32_t width = 1280;
	const int32_t height = 720;

	CameraInfo camInfo{ {0, 0, 0} };

	std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
	std::chrono::microseconds deltaTime;
	float fps = 0.0f;
	float frameTime = 0.0f;

	//SDL Setup
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
	ID3D11VertexShader* vShader;
	ID3D11PixelShader* pShader;
	ID3D11Buffer* vBuffer;
	ID3D11InputLayout* inputLayout;
	ID3D11Buffer* cameraConstBuffer;

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

	//Compile, create, and set shaders
	ID3D10Blob* vs;
	ID3D10Blob* ps;

	ID3D10Blob* vsError;
	ID3D10Blob* psError;

	auto resv = D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "vertexMain", "vs_4_0", 0, 0, &vs, &vsError);
	auto resp = D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "pixelMain", "ps_4_0", 0, 0, &ps, &psError);

	if (vsError != nullptr) printf("\nVERTEX SHADER ERRORS:\n%s\n", (const char*)vsError->GetBufferPointer());
	if (psError != nullptr) printf("\nPIXEL SHADER ERRORS:\n%s\n", (const char*)psError->GetBufferPointer());

	if (resv == D3D10_ERROR_FILE_NOT_FOUND) printf("file not found vs\n");
	if (resp == D3D10_ERROR_FILE_NOT_FOUND) printf("file not found ps\n");

	dev->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &vShader);
	dev->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &pShader);

	devCon->VSSetShader(vShader, nullptr, 0);
	devCon->PSSetShader(pShader, nullptr, 0);

	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	dev->CreateInputLayout(inputElementDesc, 1, vs->GetBufferPointer(), vs->GetBufferSize(), &inputLayout);
	devCon->IASetInputLayout(inputLayout);

	//Setup and load vertex buffer
	Vertex vertices[] =
	{
		{ {-1.0f, 1.0f} },
		{ {1.0f, 1.0f} },
		{ {-1.0f, -1.0f} },
		{ {-1.0f, -1.0f} },
		{ {1.0f, 1.0f} },
		{ {1.0f, -1.0f} }
	};

	D3D11_BUFFER_DESC vBuffDesc;
	ZeroMemory(&vBuffDesc, sizeof(vBuffDesc));

	vBuffDesc.Usage = D3D11_USAGE_DYNAMIC;
	vBuffDesc.ByteWidth = sizeof(vertices);
	vBuffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vBuffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	dev->CreateBuffer(&vBuffDesc, nullptr, &vBuffer);

	D3D11_MAPPED_SUBRESOURCE mappedBuffer;
	devCon->Map(vBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
	memcpy(mappedBuffer.pData, vertices, sizeof(vertices));
	devCon->Unmap(vBuffer, 0);

	//Setup and load camera constant buffer
	D3D11_BUFFER_DESC cameraConstBuffDesc;
	ZeroMemory(&cameraConstBuffDesc, sizeof(cameraConstBuffDesc));

	cameraConstBuffDesc.Usage = D3D11_USAGE_DEFAULT;
	cameraConstBuffDesc.ByteWidth = 16; //must be a multiple of 16
	cameraConstBuffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	dev->CreateBuffer(&cameraConstBuffDesc, nullptr, &cameraConstBuffer);
	devCon->PSSetConstantBuffers(0, 1, &cameraConstBuffer);
	devCon->UpdateSubresource(cameraConstBuffer, 0, 0, &camInfo, 0, 0);

	//DearImGui setup
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForD3D(window);
	ImGui_ImplDX11_Init(dev, devCon);

	int frameCount = 0;
	while (true)
	{
		deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime);
		
		++frameCount;
		if (frameCount == 1000)
		{
			fps = 1000000.0f / (float)deltaTime.count();
			frameTime = (float)deltaTime.count() / 1000.0f;
			frameCount = 0;
		}

		startTime = std::chrono::high_resolution_clock::now();

        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
			ImGui_ImplSDL2_ProcessEvent(&event);
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

		const FLOAT color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		devCon->ClearRenderTargetView(backbuffer, color);
		
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		
		devCon->IASetVertexBuffers(0, 1, &vBuffer, &stride, &offset);
		devCon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		devCon->Draw(6, 0);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Debug");
		ImGui::Text("FPS %.1f", fps);
		ImGui::Text("%0.2f ms", frameTime);
		if (ImGui::SliderFloat3("Camera Position", &camInfo.pos.x, -2.0f, 2.0f))
		{
			devCon->UpdateSubresource(cameraConstBuffer, 0, nullptr, &camInfo, 0, 0);
		}
		ImGui::End();
		
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		swapChain->Present(0, 0);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyWindow(window);

	vBuffer->Release();
	inputLayout->Release();
	pShader->Release();
	vShader->Release();
	swapChain->Release();
	backbuffer->Release();
	dev->Release();
	devCon->Release();

	return 0;
}
