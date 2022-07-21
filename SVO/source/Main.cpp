#pragma warning(disable: 26812)
#pragma warning(disable: 26819)

#include <stdio.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <chrono>
#include <random>
#include <thread>
#include <mutex>

#include "glm/glm.hpp"

#include "imgui.h"
#include "ImGuiBackend/imgui_impl_sdl.h"
#include "ImGuiBackend/imgui_impl_dx11.h"

#include "SDL2/SDL_syswm.h"
#include "SDL2/SDL.h"
#undef main

#include "renderdoc_app.h"

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

	uint32_t hitIndex;
	uint32_t hitChildIndex;
	uint32_t didHit;
	uint32_t EditMode; //0 remove, 1 add

	glm::vec3 placePos;
	float rootRadius;

	uint32_t debugMode{ 0 };
	glm::vec3 padding;
};

struct InputData
{
	bool w{ false };
	bool a{ false };
	bool s{ false };
	bool d{ false };
	bool f{ false };
	bool rightMouse{ false };
};

struct DeferUpdate
{
	D3D11_BOX destRegion;
	size_t index;
};

struct DeferRaytrace
{
	glm::vec3 org;
	glm::vec3 dir;
};

uint8_t* createWorld(size_t res)
{
	const size_t size = res * res * res;
	uint8_t* buffer = (uint8_t*)malloc(size);
	unsigned int seed = (unsigned int)std::chrono::high_resolution_clock::now().time_since_epoch().count();
	std::srand(seed);
	
	assert(buffer != nullptr);

	for (size_t ind = 0; ind < size; ++ind)
	{
		int val = std::rand() % 100;
		if (val > 100) buffer[ind] = 0;
		else buffer[ind] = 1;
	}

	return buffer;
}

char getBlockAt(char* world, size_t x, size_t y, size_t z)
{
	assert(world != nullptr);
	return world[x + (y * 256) + (z * 256 * 256)];
}

void editVoxel(int& voxelCount, const SVO::HitReturn& res, const AppInfo& appInfo, SVO& svo, ID3D11DeviceContext* devCon, ID3D11Buffer* structuredBuffer, std::vector<DeferUpdate>& updates, uint8_t* updatedBlocks = nullptr)
{
	structuredBuffer;
	devCon;

	D3D11_BOX destRegion;
	uint32_t stackInd = res.stackIndex;
	uint32_t rootIndex = res.index;
	uint32_t hitChildIndex = res.childIndex;

	if (appInfo.EditMode == 0) svo.vec().data()[rootIndex].masks &= (~((1 << hitChildIndex) << 24));
	else svo.vec().data()[rootIndex].masks |= (((1 << hitChildIndex) << 24));

	while (appInfo.EditMode == 0 ? (svo.vec().data()[rootIndex].masks & (0b11111111 << 24)) == 0 : (svo.vec().data()[rootIndex].masks & ((1 << hitChildIndex) << 24)) > 0)
	{
		rootIndex = res.stack[stackInd].index;
		hitChildIndex = res.stack[stackInd].childIndex;
		if (appInfo.EditMode == 0)
		{
			uint32_t mask = (~((1 << hitChildIndex) << 24));
			svo.vec().data()[rootIndex].masks &= mask;
		}
		else
		{
			if ((svo.vec().data()[rootIndex].masks & ((1 << hitChildIndex) << 24)) > 0) break;
			else svo.vec().data()[rootIndex].masks |= (((1 << hitChildIndex) << 24));
		}

		destRegion.left = (int)(sizeof(SVO::Element) * rootIndex);
		destRegion.right = destRegion.left + sizeof(SVO::Element);
		destRegion.top = 0;
		destRegion.bottom = 1;
		destRegion.front = 0;
		destRegion.back = 1;
		//devCon->UpdateSubresource(structuredBuffer, 0, &destRegion, &svo.vec().data()[rootIndex], 0, 0);
		updates.push_back({ destRegion, rootIndex });
		if (updatedBlocks != nullptr) updatedBlocks[rootIndex] = 0;

		if (stackInd > 0) --stackInd;
		else break;
	}

	destRegion.left = (int)(sizeof(SVO::Element) * res.index);
	destRegion.right = destRegion.left + sizeof(SVO::Element);
	destRegion.top = 0;
	destRegion.bottom = 1;
	destRegion.front = 0;
	destRegion.back = 1;
	//devCon->UpdateSubresource(structuredBuffer, 0, &destRegion, &svo.vec().data()[res.index], 0, 0);
	updates.push_back({ destRegion, res.index });
	if (updatedBlocks != nullptr) updatedBlocks[res.index] = 0;
	--voxelCount;
}

void editVoxelUseMap(int& voxelCount, uint32_t index, uint32_t childIndex, const AppInfo& appInfo, SVO& svo, ID3D11DeviceContext* devCon, ID3D11Buffer* structuredBuffer, std::vector<DeferUpdate>& updates, uint8_t* updatedBlocks = nullptr)
{
	structuredBuffer;
	devCon;
	updatedBlocks;
	updates;

	D3D11_BOX destRegion;
	uint32_t rootIndex = index;
	uint32_t hitChildIndex = childIndex;

	if (appInfo.EditMode == 0) svo.vec().data()[rootIndex].masks &= (~((1 << hitChildIndex) << 24));
	else svo.vec().data()[rootIndex].masks |= (((1 << hitChildIndex) << 24));

	while (appInfo.EditMode == 0 ? (svo.vec().data()[rootIndex].masks & (0b11111111 << 24)) == 0 : (svo.vec().data()[rootIndex].masks & ((1 << hitChildIndex) << 24)) > 0)
	{
		hitChildIndex = svo.posMap()[rootIndex].child;
		rootIndex = (uint32_t)svo.posMap()[rootIndex].parent;
		if (appInfo.EditMode == 0)
		{
			uint32_t mask = (~((1 << hitChildIndex) << 24));
			svo.vec().data()[rootIndex].masks &= mask;
		}
		else
		{
			if ((svo.vec().data()[rootIndex].masks & ((1 << hitChildIndex) << 24)) > 0) break;
			else svo.vec().data()[rootIndex].masks |= (((1 << hitChildIndex) << 24));
		}

		destRegion.left = (int)(sizeof(SVO::Element) * rootIndex);
		destRegion.right = destRegion.left + sizeof(SVO::Element);
		destRegion.top = 0;
		destRegion.bottom = 1;
		destRegion.front = 0;
		destRegion.back = 1;
		//devCon->UpdateSubresource(structuredBuffer, 0, &destRegion, &svo.vec().data()[rootIndex], 0, 0);
		updates.push_back({ destRegion, rootIndex });
		if (updatedBlocks != nullptr) updatedBlocks[rootIndex] = 0;

		if (rootIndex <= 0) break;
	}

	destRegion.left = (int)(sizeof(SVO::Element) * index);
	destRegion.right = destRegion.left + sizeof(SVO::Element);
	destRegion.top = 0;
	destRegion.bottom = 1;
	destRegion.front = 0;
	destRegion.back = 1;
	//devCon->UpdateSubresource(structuredBuffer, 0, &destRegion, &svo.vec().data()[index], 0, 0);
	updates.push_back({ destRegion, index });
	if (updatedBlocks != nullptr) updatedBlocks[index] = 0;
	--voxelCount;
}

int main()
{
	printf("Starting Up...\n");

	//Application info
	const bool readFile = false;
	const size_t treeDepth = 8;
	const int32_t width = 1920;
	const int32_t height = 1080;
	float flightSpeed = 100.0f;
	float destroyRad2 = 4.0f;
	char mapFileName[100] = {'f', 'i', 'l', 'e', '\0'};

	std::mutex mut;
	std::vector<std::thread> threads;
	threads.reserve(16);

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
	//uint8_t* worldData = createWorld(1 << (treeDepth + 1));

	struct ParentBlock
	{
		size_t index;
		uint8_t childIndex;
	};

	struct Block
	{
		size_t index;
		int depth;
		glm::vec3 pos;
		float radius;

		ParentBlock stack[16];
	};

	//int blockInd = 0;
	int voxelCount = 0;
	int voxelTotal = 0;
	SVO svo(treeDepth + 1);

	std::vector<Block> createStack;

	glm::vec3 offsets[8] =
	{
		{0.5,  -0.5, 0.5},
		{-0.5, -0.5, 0.5},
		{-0.5, -0.5, -0.5},
		{0.5,  -0.5, -0.5},

		{0.5, 0.5, 0.5},
		{-0.5, 0.5, 0.5},
		{-0.5, 0.5, -0.5},
		{0.5, 0.5, -0.5},
	};

	auto isInWorld = [](glm::vec3 pos)
	{
		return pos.y < (127.0f * sin(pos.x / 50.0f)) + (127.0f * cos(pos.z / 50.0f));
		//return pos.y < 0.0f;
	};

	if (readFile)
	{
		FILE* f = fopen("hills_big.bin", "rb");
		size_t svoSize = 0;
		
		fread(&appInfo.rootRadius, sizeof(float), 1, f);
		fread(&svoSize, sizeof(size_t), 1, f);

		svo.rootRadius() = appInfo.rootRadius;
		svo.vec().reserve(svoSize);
		svo.vec().resize(svoSize);

		fread(svo.vec().data(), sizeof(SVO::Element), svoSize, f);
		fclose(f);
	}
	else
	{
		auto start = (std::chrono::high_resolution_clock::now());

		appInfo.rootRadius = 256.0f;// 1 << treeDepth;
		svo.rootRadius() = appInfo.rootRadius;

		Block begin{ 0, 0, { 0, 0, 0 }, appInfo.rootRadius };
		createStack.push_back(begin);
		SVO::Element root{ 1, static_cast<uint32_t>(0b00000000 << 24) | (0b00000000 << 16) };
		//svo.vec().reserve(19173961*8);
		//svo.posMap().reserve(svo.vec().capacity());
		svo.vec().push_back(root);
		svo.posMap().push_back({ glm::vec3(0,0,0), 0, appInfo.rootRadius });

		while (createStack.size() > 0)
		{
			Block cur = createStack.back();
			createStack.pop_back();

			if (cur.depth >= treeDepth)
			{
				for (int i = 0; i < 8; ++i)
				{
					bool isIn = isInWorld(cur.pos);
					if (isIn)
					{
						++voxelCount;
						svo.vec()[cur.index].masks |= (1 << (24 + i));

						for (int d = cur.depth - 1; d >= 0; --d)
						{
							ParentBlock& p = cur.stack[d];
							if ((svo.vec()[p.index].masks & (1 << (24 + p.childIndex))) > 0) break;
							svo.vec()[p.index].masks |= (1 << (24 + p.childIndex));
						}
					}

					++voxelTotal;
				}
				continue;
			}

			svo.vec()[cur.index].childPointer = (uint32_t)svo.vec().size();

			for (int i = 0; i < 8; ++i)
			{
				SVO::Element item{ 123, static_cast<uint32_t>(0b00000000 << 24) | (0b00000000 << 16) };
				glm::vec3 childPos = cur.pos + (offsets[i] * cur.radius);
				float childRad = cur.radius / 2.0f;

				if (cur.depth + 1 >= treeDepth)
				{
					item.masks = 0;
					item.masks |= static_cast<uint32_t>(0b11111111 << 16);
				}

				
				//bool didFind = false;
				//for (float xMin = childPos.x - childRad + 0.5f; xMin <= childPos.x + childRad - 0.5f; ++xMin)
				//{
				//	for (float yMin = childPos.y - childRad + 0.5f; yMin <= childPos.y + childRad - 0.5f; ++yMin)
				//	{
				//		for (float zMin = childPos.z - childRad + 0.5f; zMin <= childPos.z + childRad - 0.5f; ++zMin)
				//		{
				//			if (isInWorld(glm::vec3(xMin, yMin, zMin)))
				//			{
				//				didFind = true;
				//				break;
				//			}
				//		}
				//	}
				//	if (didFind)
				//	{
				//		svo.vec()[cur.index].masks |= (1 << (24 + i));
				//		break;
				//	}
				//}

				svo.posMap().push_back({ childPos, svo.vec().size(), childRad, cur.index, (uint8_t)i });
				svo.vec().push_back(item);

				Block childBlock = { svo.vec().size() - 1, cur.depth + 1, childPos, childRad };
				memcpy(childBlock.stack, cur.stack, sizeof(ParentBlock)* cur.depth);
				childBlock.stack[cur.depth] = { cur.index, (uint8_t)i };
				createStack.push_back(childBlock);
			}
		}

		auto end = (std::chrono::high_resolution_clock::now());
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		printf("gen time %lld\n", time.count());
	}

	uint8_t* updatedBlocks = new uint8_t[svo.vec().size()];
	memset(updatedBlocks, 0, svo.vec().size());

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
	createConstantBuffer(&constBuffers[0], &appInfo, sizeof(AppInfo));

	//Setup and load svo constant buffer
	//createConstantBuffer(&constBuffers[1], svo.vec().data(), 16 * ((UINT)svo.vec().size()));
	//svo.data().size() < 64 ? (UINT)svo.data().size() * (UINT)sizeof(SVO::Element) : 64 * (UINT)sizeof(SVO::Element)); //max of 64 elements, for now

	devCon->PSSetConstantBuffers(0, 1, constBuffers);

	devCon->UpdateSubresource(constBuffers[0], 0, nullptr, &appInfo, 0, 0);
	//devCon->UpdateSubresource(constBuffers[1], 0, nullptr, svo.vec().data(), 0, 0);

	//Structured buffer setup
	//https://www.gamedev.net/forums/topic/709796-working-with-structuredbuffer-in-hlsl-directx-11/
	ID3D11Buffer* structuredBuffer = nullptr;
	ID3D11ShaderResourceView* structuredRscView = nullptr;
	D3D11_BUFFER_DESC structBuffDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC structuredRscDesc;
	//D3D11_MAPPED_SUBRESOURCE mappedStructuredBuffer;

	structBuffDesc.ByteWidth = sizeof(SVO::Element) * ((UINT)svo.vec().size());
	structBuffDesc.Usage = D3D11_USAGE_DEFAULT;
	structBuffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	structBuffDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	structBuffDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	structBuffDesc.StructureByteStride = sizeof(SVO::Element);

	dev->CreateBuffer(&structBuffDesc, nullptr, &structuredBuffer);
	assert(structuredBuffer != nullptr);

	structuredRscDesc.Format = DXGI_FORMAT_UNKNOWN;
	structuredRscDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	structuredRscDesc.Buffer.FirstElement = 0;
	structuredRscDesc.Buffer.NumElements = (UINT)svo.vec().size();

	dev->CreateShaderResourceView(structuredBuffer, &structuredRscDesc, &structuredRscView);
	assert(structuredRscView != nullptr);

	//devCon->Map(structuredBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedStructuredBuffer);
	//memcpy(mappedStructuredBuffer.pData, svo.vec().data(), structBuffDesc.ByteWidth);
	//devCon->Unmap(structuredBuffer, 0);
	auto start = std::chrono::high_resolution_clock::now();
	devCon->UpdateSubresource(structuredBuffer, 0, nullptr, svo.vec().data(), 0, 0);
	auto end = (std::chrono::high_resolution_clock::now());
	auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	printf("%lld upload time\n", time.count());
	devCon->PSSetShaderResources(0, 1, &structuredRscView);

	

	//free(worldData);

	//DearImGui setup
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForD3D(window);
	ImGui_ImplDX11_Init(dev, devCon);

	int frameCount = 0;
	float totalSeconds = 0.0f;
	float clickDelay = 0.0f;
	float clickDelayThresh = 0.1f;

	std::vector<DeferUpdate> updates;
	std::vector<DeferRaytrace> updatesTrace;

	SDL_Event event;
	SVO::HitReturn res;
	int btn = 0;

	while (true)
	{
		deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime);
		deltaTimeSec = (static_cast<float>(deltaTime.count()) / 1000000.0f);
		appInfo.time += deltaTimeSec;
		totalSeconds += deltaTimeSec;

		++frameCount;
		if (true)
		{
			fps = 1000000.0f / static_cast<float>(deltaTime.count());
			frameTime = static_cast<float>(deltaTime.count()) / 1000.0f;
			//frameCount = 0;
		}

		if (clickDelay < clickDelayThresh)
		{
			clickDelay += deltaTimeSec;
		}

		startTime = std::chrono::high_resolution_clock::now();

        while (SDL_PollEvent(&event))
        {
			ImGui_ImplSDL2_ProcessEvent(&event);
			uint32_t mouseBtn = SDL_GetMouseState(&mouseX, &mouseY);
			btn = SDL_BUTTON(mouseBtn);
			input.rightMouse = (btn >= 2);

			if (input.rightMouse)
			{
				SDL_SetRelativeMouseMode(SDL_TRUE);

				res = svo.getHit(appInfo.pos, appInfo.forward, appInfo.EditMode > 0);
				appInfo.hitIndex = res.index;
				appInfo.hitChildIndex = res.childIndex;
				appInfo.didHit = res.didHit;
				appInfo.placePos = res.position;
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
					
					if (event.key.keysym.scancode == SDL_SCANCODE_F)
					{
						input.f = true;
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

					if (event.key.keysym.scancode == SDL_SCANCODE_F)
					{
						input.f = false;
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

		//get voxel at function
		//clear voxel at function

		if (btn >= 16 && clickDelay >= clickDelayThresh && appInfo.didHit > 0)
		{
			clickDelay = 0.0f;
			updates.clear();
			editVoxelUseMap(voxelCount, res.index, res.childIndex, appInfo, svo, devCon, structuredBuffer, updates);
			//editVoxel(voxelCount, res, appInfo, svo, devCon, structuredBuffer, updates);
			for (size_t i = 0; i < updates.size(); ++i)
			{
				devCon->UpdateSubresource(structuredBuffer, 0, &updates[i].destRegion, &svo.vec().data()[updates[i].index], 0, 0);
			}

			res = svo.getHit(appInfo.pos, appInfo.forward, appInfo.EditMode > 0);
			appInfo.hitIndex = res.index;
			appInfo.hitChildIndex = res.childIndex;
			appInfo.didHit = res.didHit;
			appInfo.placePos = res.position;
		}

		if (input.rightMouse)
		{
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

			static const int32_t leafMask = (0b11111111 << 16);
			size_t test = 0;

			if (input.f && clickDelay >= clickDelayThresh)
			{
				clickDelay = 0.0f;
				//uint32_t oldMode = appInfo.EditMode;
				//appInfo.EditMode = 0;

				auto modifyVoxels = [&svo, &res, destroyRad2, devCon, &appInfo, &voxelCount, structuredBuffer, offsets, &test, &updates, updatedBlocks](size_t start, size_t end)
				{
					for (size_t i = start; i < end; ++i)
					{
						glm::vec3 pos = svo.posMap()[i].position;
						glm::vec3 dist = pos - res.position;
						if (glm::dot(dist, dist) < destroyRad2)
						{
							for (size_t j = 0; j < 8; ++j)
							{
								uint32_t modi = (uint32_t)j;
								if (appInfo.EditMode < 1 ? (svo.vec()[svo.posMap()[i].index].masks & (1 << (16 + modi))) > 0 && (svo.vec()[svo.posMap()[i].index].masks & (1 << (24 + modi))) > 0 :
								(svo.vec()[svo.posMap()[i].index].masks & (1 << (16 + modi))) > 0)
								{
									//glm::vec3 dir = j < 4 ? glm::vec3(0, 1, 0) : glm::vec3(0, -1, 0);
									//glm::vec3 org = pos + (offsets[j] * svo.posMap()[i].rad);
									//SVO::HitReturn r = svo.getHit(org, dir, false, true);
									//if (r.didHit)
									//{
									//}
									editVoxelUseMap(voxelCount, (uint32_t)svo.posMap()[i].index, modi, appInfo, svo, devCon, structuredBuffer, updates, updatedBlocks);
									updatedBlocks[svo.posMap()[i].index] = 0;
								}
							}
						}
					}
				};

				//memset(updatedBlocks, 0, svo.vec().size());
				//updates.clear();

				modifyVoxels(0, svo.posMap().size());
				
				//appInfo.EditMode = oldMode;
			}
		}
		
		size_t i;
		for (i = 0; i < updates.size(); ++i)
		{
			uint8_t val = updatedBlocks[updates[i].index];
			if (val <= 0)
			{
				devCon->UpdateSubresource(structuredBuffer, 0, &updates[i].destRegion, &svo.vec().data()[updates[i].index], 0, 0);
				updatedBlocks[updates[i].index] = 1;
			}
		}

		updates.erase(updates.begin(), updates.begin() + i);

		res = svo.getHit(appInfo.pos, appInfo.forward, appInfo.EditMode > 0);
		appInfo.hitIndex = res.index;
		appInfo.hitChildIndex = res.childIndex;
		appInfo.didHit = res.didHit;
		appInfo.placePos = res.position;

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
		ImGui::Text("%d voxels filled", voxelCount);
		ImGui::Text("%d voxels total", voxelTotal);
		ImGui::Text("%0.2f time running", totalSeconds);
		ImGui::Text("FPS %.1f", fps);
		ImGui::Text("%0.2f ms", frameTime);
		ImGui::Text("Pos: %0.5f, %0.5f, %0.5f", appInfo.pos.x, appInfo.pos.y, appInfo.pos.z);
		ImGui::Checkbox("Visualize octree", (bool*)(&appInfo.debugMode));
		ImGui::End();

		ImGui::Begin("Controls");
		if(ImGui::Checkbox("Add Voxels", (bool*)&appInfo.EditMode))
		{
			res = svo.getHit(appInfo.pos, appInfo.forward, appInfo.EditMode > 0);
			appInfo.hitIndex = res.index;
			appInfo.hitChildIndex = res.childIndex;
			appInfo.didHit = res.didHit;
			appInfo.placePos = res.position;
		}

		ImGui::InputFloat("Click Delay", &clickDelayThresh);

		(ImGui::SliderFloat("Flight speed", &flightSpeed, 1.0f, 300.0f));
		(ImGui::SliderFloat("Destroy Radius Squared", &destroyRad2, 4.0f, 10000.0f));
		ImGui::End();

		ImGui::Begin("File");
		ImGui::InputText("File Name", mapFileName, 100);
		if (ImGui::Button("Save Map"))
		{
			ImGui::OpenPopup("SaveConfirm");
		}

		if (ImGui::BeginPopup("SaveConfirm"))
		{
			ImGui::Text("Are you sure you want to save?");
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Button("Confirm"))
			{
				FILE* f = fopen(mapFileName, "wb");
				size_t size = svo.vec().size();
				fwrite(&appInfo.rootRadius, sizeof(float), 1, f);
				fwrite(&size, sizeof(size_t), 1, f);
				fwrite(svo.vec().data(), sizeof(SVO::Element), svo.vec().size(), f);
				fclose(f);
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::Button("Load Map"))
		{

		}
		ImGui::End();
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		devCon->UpdateSubresource(constBuffers[0], 0, nullptr, &appInfo, 0, 0);
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
