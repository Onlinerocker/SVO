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

#include "SVO.h"
#include "Camera.h"

struct Vertex
{
	glm::vec2 pos;
};

struct AppInfo
{
	glm::vec3 pos;
	float time;

	glm::vec4 forward;
	glm::vec4 right;
	glm::vec4 up;
};

struct InputData
{
	bool w{ false };
	bool a{ false };
	bool s{ false };
	bool d{ false };
};

int main()
{
	printf("Starting Up...\n");

	//Application info
	const int32_t width = 1280;
	const int32_t height = 720;
	float flightSpeed = 100.0f;

	Camera camera;
	InputData input;
	AppInfo appInfo{ {0, 0, -300.0f}, 0.0f, {0, 0, 1, 0}, {1, 0, 0, 0}, {0, 1, 0, 0} };

	std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
	std::chrono::microseconds deltaTime;
	float deltaTimeSec = 0.0f;
	float fps = 0.0f;
	float frameTime = 0.0f;
	int mouseX = 0;
	int mouseY = 0;

	//SVO::Element root{ 1, static_cast<uint32_t>(0b00101011 << 24) | (0b00000011 << 16) };
	//
	//SVO::Element child{ 1,  static_cast<uint32_t>(0b10111111 << 24) | (0b11111111 << 16) };
	//SVO::Element child1{ 1, static_cast<uint32_t>(0b01111111 << 24) | (0b10111111 << 16) };

	int max = 10;
	SVO svo(max + 1);

	SVO::Element root{ 123, static_cast<uint32_t>(0b00110111 << 24) | (0b00000111 << 16) };
	svo.vec().push_back(root);
	//svo.vec().push_back(child);
	//svo.vec().push_back(child1);

	
	for (int i = 0; i <= max; ++i)
	{
		svo.vec().back().childPtr = (uint32_t)svo.vec().size()+1;
		if(i > 0) svo.vec()[svo.vec().size()-2].childPtr = (uint32_t)svo.vec().size();

		if (i == max)
		{
			SVO::Element newChild{ 123, static_cast<uint32_t>(0b00100111 << 24) | (0b00100111 << 16) };
			svo.vec().push_back(newChild);
		}
		else
		{
			SVO::Element newChild{ 123, static_cast<uint32_t>(0b00100111 << 24) | (0b00000111 << 16) };
			svo.vec().push_back(newChild);
		}

		if (i == max)
		{
			SVO::Element newChild{ 123, static_cast<uint32_t>(0b00100111 << 24) | (0b00100111 << 16) };
			svo.vec().push_back(newChild);
		}
		else
		{
			SVO::Element newChild{ 123, static_cast<uint32_t>(0b00100111 << 24) | (0b00000111 << 16) };
			svo.vec().push_back(newChild);
		}
	}

	//SDL Setup
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* window = SDL_CreateWindow("SVO Renderer", 10, 100, width, height, 0);
	SDL_SetWindowFullscreen(window, 0);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	SDL_SysWMinfo sysWMInfo;
	SDL_VERSION(&sysWMInfo.version);
	SDL_GetWindowWMInfo(window, &sysWMInfo);	

	IDXGISwapChain* swapChain = nullptr;
	ID3D11Device* dev = nullptr;
	ID3D11DeviceContext* devCon = nullptr;
	ID3D11RenderTargetView* backbuffer = nullptr;
	ID3D11VertexShader* vShader = nullptr;
	ID3D11PixelShader* pShader = nullptr;
	ID3D11Buffer* vBuffer = nullptr;
	ID3D11InputLayout* inputLayout = nullptr;

	//ID3D11Buffer* cameraConstBuffer = nullptr;
	//ID3D11Buffer* svoConstBuffer = nullptr;

	ID3D11Buffer* constBuffers[2];

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

	UINT constBufferCount = 0;
	auto createConstantBuffer = [&](ID3D11Buffer** buffer, void* data, UINT dataSize)
	{
		data;
		D3D11_BUFFER_DESC constBuffDesc;
		ZeroMemory(&constBuffDesc, sizeof(constBuffDesc));

		constBuffDesc.Usage = D3D11_USAGE_DEFAULT;
		constBuffDesc.ByteWidth = dataSize; //must be a multiple of 16
		constBuffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		dev->CreateBuffer(&constBuffDesc, nullptr, buffer);
		++constBufferCount;
	};

	//Setup and load app info constant buffer
	createConstantBuffer(&constBuffers[0], &appInfo, 64);

	//Setup and load svo constant buffer
	createConstantBuffer(&constBuffers[1], svo.vec().data(), 16 * (UINT)svo.vec().size());
		//svo.data().size() < 64 ? (UINT)svo.data().size() * (UINT)sizeof(SVO::Element) : 64 * (UINT)sizeof(SVO::Element)); //max of 64 elements, for now

	devCon->PSSetConstantBuffers(0, constBufferCount, constBuffers);

	devCon->UpdateSubresource(constBuffers[0], 0, nullptr, &appInfo, 0, 0);
	devCon->UpdateSubresource(constBuffers[1], 0, nullptr, svo.vec().data(), 0, 0);

	//ID3D11Buffer* buffers;
	//devCon->PSGetConstantBuffers(0, 2, &buffers);

	//DearImGui setup
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForD3D(window);
	ImGui_ImplDX11_Init(dev, devCon);

	int frameCount = 0;
	while (true)
	{
		deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime);
		deltaTimeSec = (static_cast<float>(deltaTime.count()) / 1000000.0f);
		appInfo.time += deltaTimeSec;

		++frameCount;
		if (frameCount == 1000)
		{
			fps = 1000000.0f / static_cast<float>(deltaTime.count());
			frameTime = static_cast<float>(deltaTime.count()) / 1000.0f;
			frameCount = 0;
		}

		startTime = std::chrono::high_resolution_clock::now();

        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
			ImGui_ImplSDL2_ProcessEvent(&event);
			uint32_t mouseBtn = SDL_GetMouseState(&mouseX, &mouseY);
			int btn = SDL_BUTTON(mouseBtn);

			if (btn >= 2)
			{
				SDL_SetRelativeMouseMode(SDL_TRUE);
			}
			else
			{
				SDL_SetRelativeMouseMode(SDL_FALSE);
			}

            switch (event.type)
            {
				case SDL_QUIT:
				{
				    exit(0);
				    break;
				}

				case SDL_KEYDOWN:
				{
					if (event.key.keysym.scancode == SDL_SCANCODE_W)
					{
						input.w = true;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_S)
					{
						input.s = true;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_A)
					{
						input.a = true;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_D)
					{
						input.d = true;
					}
					break;
				}

				case SDL_KEYUP:
				{
					if (event.key.keysym.scancode == SDL_SCANCODE_W)
					{
						input.w = false;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_S)
					{
						input.s = false;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_A)
					{
						input.a = false;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_D)
					{
						input.d = false;
					}
					break;
				}

				case SDL_MOUSEMOTION:
				{
					if (btn >= 2)
					{
						camera.update(-(event.motion.xrel) / 10.0f, -(event.motion.yrel) / 10.0f);

						appInfo.forward = glm::vec4(camera.forward(), 0);
						appInfo.right = glm::vec4(camera.right(), 0);
						appInfo.up = glm::vec4(camera.up(), 0);
					}
					break;
				}

				default:
				{
				    break;
				}
            }
        }

		if (input.w)
		{
			appInfo.pos += glm::vec3(appInfo.forward) * deltaTimeSec * flightSpeed;
		}
		else if (input.s)
		{
			appInfo.pos -= glm::vec3(appInfo.forward) * deltaTimeSec * flightSpeed;
		}

		if (input.a)
		{
			appInfo.pos -= glm::vec3(appInfo.right) * deltaTimeSec * flightSpeed;
		}
		else if (input.d)
		{
			appInfo.pos += glm::vec3(appInfo.right) * deltaTimeSec * flightSpeed;
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
		ImGui::Text("[Flight Controls] WASD, right click + mouse");
		ImGui::Text("FPS %.1f", fps);
		ImGui::Text("%0.2f ms", frameTime);
		ImGui::Text("Pos: %0.5f, %0.5f, %0.5f", appInfo.pos.x, appInfo.pos.y, appInfo.pos.z);

		(ImGui::SliderFloat("Flight speed", &flightSpeed, 1.0f, 1000.0f));

		devCon->UpdateSubresource(constBuffers[0], 0, nullptr, &appInfo, 0, 0);
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
