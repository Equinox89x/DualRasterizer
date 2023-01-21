#include "pch.h"
#include "Renderer.h"
#include "Material.h"
#include "Utils.h"
#include "Effect.h"
#include <future>
#include <ppl.h>
#include <iterator>
#include <vector>

namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

		int size{ m_Width * m_Height };
		m_pColorBuffer = new ColorRGB[size];
		m_pDepthBuffer = new float[size];

		m_TranslationTransform = Matrix::CreateTranslation(0, 0, 50);
		m_RotationTransform = Matrix::CreateRotationZ(0);
		m_ScaleTransform = Matrix::CreateScale(1, 1, 1);

		//Initialize Camera
		const float screenWidth{ static_cast<float>(m_Width) };
		const float screenHeight{ static_cast<float>(m_Height) };
		m_Camera.Initialize(45.f, { 0.f,0.f,0.f });
		m_Camera.m_AspectRatio = screenWidth / screenHeight;


		//Initialize DirectX pipeline
		const HRESULT result = InitializeDirectX();
		if (result == S_OK)
		{
			m_IsInitialized = true;
			std::cout << "DirectX is initialized and ready!\n";

			std::vector<std::string>names{ "Resources/vehicle.obj", "Resources/fireFX.obj" };
			for (size_t i = 0; i < names.size(); i++)
			{
				std::vector<Vertex>vertices{};
				std::vector<uint32_t> indices{};
				Utils::ParseOBJ(names[i], vertices, indices);
				m_Meshes.push_back(new Mesh{ m_pDevice, vertices, indices });

				m_Meshes[i]->SetWorldMatrix(m_ScaleTransform * m_RotationTransform * m_TranslationTransform);
				m_Meshes[i]->SetMatrix(m_Camera.m_WorldViewProjectionMatrix, m_Meshes[0]->m_WorldMatrix, m_Camera.m_Origin);
			}
			m_Camera.m_WorldViewProjectionMatrix = m_Meshes[0]->m_WorldMatrix * m_Camera.m_ViewMatrix * m_Camera.GetProjectionMatrix();


			m_pTexture = Texture::LoadFromFile("Resources/vehicle_diffuse.png", m_pDevice);
			m_pTextureGloss = Texture::LoadFromFile("Resources/vehicle_gloss.png", m_pDevice);
			m_pTextureNormal = Texture::LoadFromFile("Resources/vehicle_normal.png", m_pDevice);
			m_pTextureSpecular = Texture::LoadFromFile("Resources/vehicle_specular.png", m_pDevice);
			m_pCombustionTexture = Texture::LoadFromFile("Resources/fireFX_diffuse.png", m_pDevice);

			m_Meshes[0]->m_pEffect->SetMaps(m_pTexture, m_pTextureSpecular, m_pTextureNormal, m_pTextureGloss);
			m_Meshes[1]->m_pEffect->SetMaps(m_pCombustionTexture);
			m_Meshes[1]->m_pEffect->ChangeEffect("FlatTechnique");
		}
		else
		{
			std::cout << "DirectX initialization failed!\n";
		}
	}

	Renderer::~Renderer()
	{
		#pragma region clearing normal resources
		delete m_pTexture;
		m_pTexture = nullptr;
		delete m_pTextureGloss;
		m_pTextureGloss = nullptr;
		delete m_pTextureNormal;
		m_pTextureNormal = nullptr;
		delete m_pTextureSpecular;
		m_pTextureSpecular = nullptr;
		delete m_pCombustionTexture;
		m_pCombustionTexture = nullptr;

		delete[] m_pDepthBuffer;
		delete[] m_pColorBuffer;
		m_pDepthBuffer = nullptr;
		m_pColorBuffer = nullptr;

		m_pBackBufferPixels = nullptr;
		if (m_pFrontBuffer) {
			SDL_FreeSurface(m_pFrontBuffer);
			m_pFrontBuffer = nullptr;
		}
		if (m_pBackBuffer) {

			SDL_FreeSurface(m_pBackBuffer);
			m_pBackBuffer = nullptr;
		}
		#pragma endregion

		#pragma region clearing directx resources
		//// does not work
		////delete m_pWindow;
		//delete m_Meshes[0];
		//m_Meshes[0] = nullptr;
		//
		//delete m_Meshes[1];
		//m_Meshes[1] = nullptr;
		//
		//m_Meshes.clear();
		//
		//if (m_pDepthStencilView)m_pDepthStencilView->Release();
		//if (m_PRenderTargetView)m_PRenderTargetView->Release();
		//if (m_pDepthStencilBuffer)m_pDepthStencilBuffer->Release();
		//if (m_pRenderTargetBuffer)m_pRenderTargetBuffer->Release();
		//if (m_pSwapChain)m_pSwapChain->Release();
		//
		//
		//m_pDevice->Release();
		//
		//if (m_pDeviceContext)
		//{
		//	//m_pDeviceContext->ClearState();
		//	m_pDeviceContext->Flush();
		//	m_pDeviceContext->Release();
		//}		
		// 
		//m_pDepthStencilView = nullptr;
		//m_PRenderTargetView = nullptr;
		//m_pRenderTargetBuffer = nullptr;
		//m_pDepthStencilBuffer = nullptr;
		//m_pSwapChain = nullptr;
		//m_pDeviceContext = nullptr;
		//m_pDevice = nullptr;

		//works, gives no memory leaks according to VLD
		m_pDeviceContext->ClearState();
		m_pDeviceContext->Flush();

		delete m_Meshes[0];
		delete m_Meshes[1];

		//m_pDepthStencilBuffer->Release();
		//m_pRenderTargetBuffer->Release();

		////correct order
		//m_pDepthStencilView->Release();
		//m_PRenderTargetView->Release();
		m_pSwapChain->Release();
		//m_pDeviceContext->Release();
		//m_pDevice->Release();

		m_pSwapChain = nullptr;
		#pragma endregion
	}

	void Renderer::Update(const Timer* pTimer)
	{
		const float screenWidth{ static_cast<float>(m_Width) };
		const float screenHeight{ static_cast<float>(m_Height) };
		m_Camera.m_AspectRatio = screenWidth / screenHeight;
		m_Camera.Update(pTimer);

		m_IsHardware ? m_SelectedColor = m_HardwareColor : m_SelectedColor = m_SoftwareColor;
		if (m_HasClearColor) {
			m_SelectedColor = m_UniformColor;
		}

		if (m_HasRotation) {
			m_RotationAngle = pTimer->GetTotal() * (m_RotationSpeed * PI / 180);
			m_RotationTransform = Matrix::CreateRotationY(m_RotationAngle);
			for (Mesh* mesh : m_Meshes)
			{
				mesh->SetWorldMatrix(m_ScaleTransform * m_RotationTransform * m_TranslationTransform);
				if (m_IsHardware) {
					mesh->SetMatrix(m_Camera.m_WorldViewProjectionMatrix, mesh->m_WorldMatrix, m_Camera.m_Origin);
				}
			}
		}
		m_Camera.m_WorldViewProjectionMatrix = m_Meshes[0]->m_WorldMatrix * m_Camera.m_ViewMatrix * m_Camera.GetProjectionMatrix();
	}

	void Renderer::Render() const
	{
		if (m_IsHardware) {
			if (!m_IsInitialized)
				return;

			//1. CLEAR RTV & DSV
			m_pDeviceContext->ClearRenderTargetView(m_PRenderTargetView, &m_SelectedColor.r);
			m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

			//2. SET PIPELINE + INVOKE DRAWCALLS (= RENDER)
			//...
			m_Meshes[0]->Render(m_pDeviceContext);
			if (m_HasFire) {
				m_Meshes[1]->Render(m_pDeviceContext);
			}

			//3. PRESENT BACKBUFFER (SWAP)
			m_pSwapChain->Present(0, 0);

		}
		else {
			//@START
			//Lock BackBuffer
			SDL_LockSurface(m_pBackBuffer);

			const int size{ m_Width * m_Height };
			std::fill(m_pDepthBuffer, m_pDepthBuffer + size, FLT_MAX);
			std::fill(m_pColorBuffer, m_pColorBuffer + size, m_SelectedColor);

			SDL_FillRect(m_pBackBuffer, nullptr, m_HasClearColor ? 0x191919 : 0x636363);


			//RENDER LOGIC
			//Loop through meshes and perform correct render based on topology
			/*for (const Mesh* mesh : m_Meshes)
			{*/
				//we only render the first mesh because we don't render the flame in the software version.
				RenderMeshTriangleList(*m_Meshes[0]);
			//}

			//@END
			//Update SDL Surface
			SDL_UnlockSurface(m_pBackBuffer);
			SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
			SDL_UpdateWindowSurface(m_pWindow);
		}
	}

	#pragma region software
	void Renderer::HandleRenderBB(std::vector<Vertex_Out>& verts) const
	{
		ColorRGB finalColor{};

		//Triangle edge
		const Vector2 a{ verts[1].position.x - verts[0].position.x, verts[1].position.y - verts[0].position.y };
		const Vector2 b{ verts[2].position.x - verts[1].position.x, verts[2].position.y - verts[1].position.y };
		const Vector2 c{ verts[0].position.x - verts[2].position.x, verts[0].position.y - verts[2].position.y };

		//Triangle verts
		const Vector2 triangleV0{ verts[0].position.x, verts[0].position.y };
		const Vector2 triangleV1{ verts[1].position.x, verts[1].position.y };
		const Vector2 triangleV2{ verts[2].position.x, verts[2].position.y };

		//find the top left and bottom right point of the bounding box
		const float maxX = std::max(std::max(triangleV2.x, triangleV0.x), std::max(triangleV0.x, triangleV1.x));
		const float minX = std::min(std::min(triangleV0.x, triangleV1.x), std::min(triangleV2.x, triangleV0.x));
		const float maxY = std::max(std::max(triangleV0.y, triangleV1.y), std::max(triangleV2.y, triangleV0.y));
		const float minY = std::min(std::min(triangleV0.y, triangleV1.y), std::min(triangleV2.y, triangleV0.y));

		if (
			((minX >= 0) && (maxX <= (m_Width - 1))) &&
			((minY >= 0) && (maxY <= (m_Height - 1)))) {
			//Loop through bounding box pixels and ceil to remove lines between triangles.
			for (int px{ static_cast<int>(minX) }; px < std::ceil(maxX); ++px)
			{
				for (int py{ static_cast<int>(minY) }; py < std::ceil(maxY); ++py)
				{

					const int currentPixel{ px + (py * m_Width) };
					if (!(currentPixel > 0 && currentPixel < m_Width * m_Height)) continue;
					const Vector2 pixel{ static_cast<float>(px) + 0.5f, static_cast<float>(py) + 0.5f };

					//Pixel position to vertices (also the weight)
					Vector2 pointToSide{ pixel - triangleV1 };
					const float edgeA{ Vector2::Cross(b, pointToSide) };

					pointToSide = pixel - triangleV2;
					const float edgeB{ Vector2::Cross(c, pointToSide) };

					pointToSide = pixel - triangleV0;
					const float edgeC{ Vector2::Cross(a, pointToSide) };

					const float triangleArea{ edgeA + edgeB + edgeC };
					const float w0{ edgeA / triangleArea };
					const float w1{ edgeB / triangleArea };
					const float w2{ edgeC / triangleArea };

					//check if pixel is inside triangle
					if (w0 > 0.f && w1 > 0.f && w2 > 0.f) {

						switch (m_CullMode)
						{
						case CullMode::Back:
							if (Vector3::Dot(verts[0].normal, verts[0].viewDirection) < 0)
							{
								return;
							}
							break;
						case CullMode::Front:
							if (Vector3::Dot(verts[0].normal, verts[0].viewDirection) > 0)
							{
								return;
							}
							break;
						default:
							break;
						}

						//depth test
						const float interpolatedDepth{ 1 / ((1 / verts[0].position.z) * w0 + (1 / verts[1].position.z) * w1 + (1 / verts[2].position.z) * w2) };
						if (interpolatedDepth > m_pDepthBuffer[currentPixel]) {
							continue;
						}
						m_pDepthBuffer[currentPixel] = interpolatedDepth;

						float gloss{ 0 };
						ColorRGB specularKS{  };
						const Vertex_Out pixelVertexPos{ CalculateVertexWithAttributes(verts, w0, w1, w2, gloss, specularKS) };

						if (m_IsShowDepthBuffer) {
							const float linearDepth = (2.0 * m_Camera.nearZ) / (m_Camera.farZ + m_Camera.nearZ - interpolatedDepth * (m_Camera.farZ - m_Camera.nearZ));
							m_pColorBuffer[currentPixel] = ColorRGB{ linearDepth, linearDepth, linearDepth };
						}
						else {
							m_pColorBuffer[currentPixel] = PixelShading(pixelVertexPos, gloss, specularKS);
						}

					}

					if (m_HasBB) m_pColorBuffer[currentPixel] = colors::White;
					
					//change color accordingly to triangle
					finalColor = m_pColorBuffer[currentPixel];

					//Update Color in Buffer
					finalColor.MaxToOne();
					m_pBackBufferPixels[currentPixel] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
		}
	}

	Vertex_Out Renderer::CalculateVertexWithAttributes(const std::vector<Vertex_Out>& verts, const float w0, const float w1, const float w2, float& outGloss, ColorRGB& outSpecularKS) const
	{
		#pragma region calculate interpolated attributes
		const float interpolatedDepthW{ 1 / (
			(1 / verts[0].position.w) * w0 +
			(1 / verts[1].position.w) * w1 +
			(1 / verts[2].position.w) * w2) };

		const Vector2 interpolatedUV{
			(((verts[0].uv / verts[0].position.w) * w0) +
				((verts[1].uv / verts[1].position.w) * w1) +
				((verts[2].uv / verts[2].position.w) * w2)) * interpolatedDepthW };

		const Vector3 interpolatedNormal{ (
			(verts[0].normal / (verts[0].position.w)) * w0 +
			(verts[1].normal / verts[1].position.w) * w1 +
			(verts[2].normal / verts[2].position.w) * w2) * interpolatedDepthW };

		const Vector3 interpolatedTangent{ (
			(verts[0].tangent / (verts[0].position.w)) * w0 +
			(verts[1].tangent / verts[1].position.w) * w1 +
			(verts[2].tangent / verts[2].position.w) * w2) * interpolatedDepthW };

		const Vector3 viewDirection{ (
			(verts[0].viewDirection / (verts[0].position.w)) * w0 +
			(verts[1].viewDirection / verts[1].position.w) * w1 +
			(verts[2].viewDirection / verts[2].position.w) * w2) * interpolatedDepthW };

		const Vector4 interpolatedPosition{ (
			verts[0].position * w0 +
			verts[1].position * w1 +
			verts[2].position * w2) * interpolatedDepthW };
		#pragma endregion

		//color from diffuse map

		const ColorRGB currentColor{ m_pTexture->Sample(interpolatedUV) };

		#pragma region normals
		const auto [Nr, Ng, Nb] { m_pTextureNormal->Sample(interpolatedUV) };
		const Vector3 binormal = Vector3::Cross(interpolatedNormal, interpolatedTangent);
		const Matrix tangentSpaceAxis{ Matrix{ interpolatedTangent,binormal,interpolatedNormal,Vector3::Zero } };

		Vector3 sampledNormal{ Nr,Ng,Nb };
		sampledNormal = 2.f * sampledNormal - Vector3{ 1.f, 1.f, 1.f };
		sampledNormal /= 255.f;
		sampledNormal = tangentSpaceAxis.TransformVector(sampledNormal).Normalized();
		#pragma endregion

		#pragma region Phong
		//gloss
		const auto [Gr, Gg, Gb] { m_pTextureGloss->Sample(interpolatedUV) };
		outGloss = Gr;

		//specular
		outSpecularKS = m_pTextureSpecular->Sample(interpolatedUV);
		#pragma endregion

		return { interpolatedPosition, currentColor, interpolatedUV, m_HasNormalMap ? sampledNormal.Normalized() : interpolatedNormal, interpolatedTangent, viewDirection.Normalized()};

	}

	ColorRGB Renderer::PixelShading(const Vertex_Out& v, const float gloss, const ColorRGB specularKS) const
	{
		float ObservedArea{ Vector3::Dot(v.normal, -m_LightDirection) };
		ObservedArea = Clamp(ObservedArea, 0.f, 1.f);

		switch (m_LightingMode)
		{
			case LightingMode::ObservedArea: {
				return ColorRGB{ ObservedArea,ObservedArea,ObservedArea };
			}

			case LightingMode::Diffuse: {
				Material_Lambert material{ Material_Lambert(v.color, m_LightIntensity) };
				const ColorRGB diffuse{ material.Shade(v) * ObservedArea };
				return diffuse;
			}

			case LightingMode::Specular: {
				const ColorRGB specular{ BRDF::Phong(specularKS, gloss * m_Shininess, m_LightDirection, v.viewDirection, v.normal) };
				return specular;
			}

			case LightingMode::Combined: {
				Material_Lambert material{ Material_Lambert(v.color, m_LightIntensity) };
				const ColorRGB diffuse{ material.Shade(v) * ObservedArea };
				const ColorRGB specular{ BRDF::Phong(specularKS, gloss * m_Shininess, m_LightDirection, v.viewDirection, v.normal) };
				const ColorRGB phong{ diffuse + specular };
				return phong;
			}

			default: {
				Material_Lambert material{ Material_Lambert(v.color, m_LightIntensity) };
				const ColorRGB diffuse{ material.Shade(v) * ObservedArea };
				const ColorRGB specular{ BRDF::Phong(specularKS, gloss * m_Shininess, m_LightDirection, v.viewDirection, v.normal) };
				const ColorRGB phong{ diffuse + specular };
				return phong;
			}
		}
	}

	void Renderer::RenderMeshTriangleList(const Mesh& mesh) const
	{
		//we divide the amount by 3 because we are going to get 3 vertexes of a triangle per loop
		concurrency::parallel_for(0u, uint32_t((mesh.indices.size()-2) / 3), [&, this](int i) {

			int index = i * 3;
			//for every 3rd indice, calculate the triangle
			#pragma region Calculate the triangles from a mesh
			const int indice1{ static_cast<int>(mesh.indices[index]) };
			const int indice2{ static_cast<int>(mesh.indices[index + 1]) };
			const int indice3{ static_cast<int>(mesh.indices[index + 2]) };
			std::vector triangleVerts{ mesh.vertices[indice1], mesh.vertices[indice2], mesh.vertices[indice3] };
			#pragma endregion

			std::vector<Vertex_Out> verts{ };

			VertexTransformationFunction(triangleVerts, verts, mesh.m_WorldMatrix);

			HandleRenderBB(verts);
		});
	}

	void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out, const Matrix& worldMatrix) const
	{
		vertices_out.resize(vertices_in.size());

		//Add viewmatrix with camera space matrix
		const Matrix worldViewProjectionMatrix{ worldMatrix * m_Camera.m_ViewMatrix * m_Camera.m_ProjectionMatrix };

		for (int i{}; i < vertices_in.size(); i++)
		{
			Vector4 point{ vertices_in[i].position, 1 };

			//Transform points to correct space
			Vector4 transformedVert{ worldViewProjectionMatrix.TransformPoint(point) };

			//Project point to 2d view plane (perspective divide)
			const float projectedVertexW{ transformedVert.w };
			float projectedVertexX{ transformedVert.x / transformedVert.w };
			float projectedVertexY{ transformedVert.y / transformedVert.w };
			float projectedVertexZ{ transformedVert.z / transformedVert.w };
			projectedVertexX = ((projectedVertexX + 1) / 2) * m_Width;
			projectedVertexY = ((1 - projectedVertexY) / 2) * m_Height;

			//transform normals to correct space and solve visibility problem
			const Vector3 normal{ worldMatrix.TransformVector(vertices_in[i].normal) };
			const Vector3 tangent{ worldMatrix.TransformVector(vertices_in[i].tangent) };

			Vector4 pos{ projectedVertexX, projectedVertexY , projectedVertexZ, projectedVertexW };
			const Vector3 viewDirection{ m_Camera.m_Origin - transformedVert };

			if (!(pos.x < -1 && pos.x > 1) && !(pos.y < -1 && pos.y > 1)) {
				vertices_out[i] = { pos, vertices_in[i].color,  vertices_in[i].uv, normal, tangent, viewDirection };
			}
		}
	}
	#pragma endregion

	bool Renderer::SaveBufferToImage() const
	{
		return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
	}

	HRESULT Renderer::InitializeDirectX()
	{
		//1. Create Device & DeviceContext
		//=====
		D3D_FEATURE_LEVEL featureLevel{ D3D_FEATURE_LEVEL_11_1 };
		uint32_t createDeviceFlags{ 0 };
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		HRESULT result{ D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel, 1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext) };


		if (FAILED(result))
			return result;

		//Create DXGI factory
		IDXGIFactory1* pDxgiFactory{};
		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pDxgiFactory));
		if (FAILED(result))
			return result;
		//2. Create Swapchain
		//=====
		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;
		//Get the handle (HWND) from the SDL Backbuffer
		SDL_SysWMinfo sysWMinfo{};
		SDL_VERSION(&sysWMinfo.version)
			SDL_GetWindowWMInfo(m_pWindow, &sysWMinfo);
		swapChainDesc.OutputWindow = sysWMinfo.info.win.window;
		//Create SwapChain
		result = pDxgiFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
		if (FAILED(result))
			return result;

		//3. Create DepthStencil (DS) & DepthStencilView (DSV)
		//Resource
		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;
		//View
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);
		if (FAILED(result))
			return result;
		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
		if (FAILED(result))
			return result;
		//4. Create RenderTarget (RT) & RenderTargetView (RTV)
		//=====

		//Resources
		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));
		if (FAILED(result))
			return result;

		//view
		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_PRenderTargetView);
		if (FAILED(result))
			return result;

		//rasterizer state
		D3D11_RASTERIZER_DESC rasterizer{};
		rasterizer.FillMode = D3D11_FILL_SOLID; //?
		rasterizer.CullMode = D3D11_CULL_NONE;
		rasterizer.FrontCounterClockwise = false;
		rasterizer.DepthBias = 0;
		rasterizer.SlopeScaledDepthBias = 0.0f;
		rasterizer.DepthBiasClamp = 0.0f;
		rasterizer.DepthClipEnable = true;
		rasterizer.ScissorEnable = false;
		rasterizer.MultisampleEnable = false;
		rasterizer.AntialiasedLineEnable = false;

		result = m_pDevice->CreateRasterizerState(&rasterizer, &m_pRasterizerStateNone);
		if (FAILED(result))
			return result;

		D3D11_RASTERIZER_DESC rasterizer2{};
		rasterizer2.FillMode = D3D11_FILL_SOLID; //?
		rasterizer2.CullMode = D3D11_CULL_FRONT;
		rasterizer2.FrontCounterClockwise = false;
		rasterizer2.DepthBias = 0;
		rasterizer2.SlopeScaledDepthBias = 0.0f;
		rasterizer2.DepthBiasClamp = 0.0f;
		rasterizer2.DepthClipEnable = true;
		rasterizer2.ScissorEnable = false;
		rasterizer2.MultisampleEnable = false;
		rasterizer2.AntialiasedLineEnable = false;

		result = m_pDevice->CreateRasterizerState(&rasterizer2, &m_pRasterizerStateFront);
		if (FAILED(result))
			return result;


		D3D11_RASTERIZER_DESC rasterizer3{};
		rasterizer3.FillMode = D3D11_FILL_SOLID;
		rasterizer3.CullMode = D3D11_CULL_BACK;
		rasterizer3.FrontCounterClockwise = false;
		rasterizer3.DepthBias = 0;
		rasterizer3.SlopeScaledDepthBias = 0.0f;
		rasterizer3.DepthBiasClamp = 0.0f;
		rasterizer3.DepthClipEnable = true;
		rasterizer3.ScissorEnable = false;
		rasterizer3.MultisampleEnable = false;
		rasterizer3.AntialiasedLineEnable = false;

		result = m_pDevice->CreateRasterizerState(&rasterizer3, &m_pRasterizerStateBack);
		if (FAILED(result))
			return result;

		D3D11_SAMPLER_DESC pointSampleState{};
		pointSampleState.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		pointSampleState.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		pointSampleState.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		pointSampleState.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		pointSampleState.MinLOD = -FLT_MAX;
		pointSampleState.MaxLOD = FLT_MAX;
		pointSampleState.MipLODBias = 0.0f;
		pointSampleState.MaxAnisotropy = 1;
		pointSampleState.ComparisonFunc = D3D11_COMPARISON_NEVER;
		pointSampleState.BorderColor[0] = 1.0f;
		pointSampleState.BorderColor[1] = 1.0f;
		pointSampleState.BorderColor[2] = 1.0f;
		pointSampleState.BorderColor[3] = 1.0f;
		result = m_pDevice->CreateSamplerState(&pointSampleState, &m_pPointSampler);
		if (FAILED(result))
			return result;


		D3D11_SAMPLER_DESC linearSampleState{};
		linearSampleState.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		linearSampleState.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		linearSampleState.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		linearSampleState.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		linearSampleState.MinLOD = -FLT_MAX;
		linearSampleState.MaxLOD = FLT_MAX;
		linearSampleState.MipLODBias = 0.0f;
		linearSampleState.MaxAnisotropy = 1;
		linearSampleState.ComparisonFunc = D3D11_COMPARISON_NEVER;
		linearSampleState.BorderColor[0] = 1.0f;
		linearSampleState.BorderColor[1] = 1.0f;
		linearSampleState.BorderColor[2] = 1.0f;
		linearSampleState.BorderColor[3] = 1.0f;
		result = m_pDevice->CreateSamplerState(&linearSampleState, &m_pLinearSampler);
		if (FAILED(result))
			return result;


		D3D11_SAMPLER_DESC anisotropicSampleState{};
		anisotropicSampleState.Filter = D3D11_FILTER_ANISOTROPIC;
		anisotropicSampleState.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		anisotropicSampleState.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		anisotropicSampleState.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		anisotropicSampleState.MinLOD = -FLT_MAX;
		anisotropicSampleState.MaxLOD = FLT_MAX;
		anisotropicSampleState.MipLODBias = 0.0f;
		anisotropicSampleState.MaxAnisotropy = 1;
		anisotropicSampleState.ComparisonFunc = D3D11_COMPARISON_NEVER;
		anisotropicSampleState.BorderColor[0] = 1.0f;
		anisotropicSampleState.BorderColor[1] = 1.0f;
		anisotropicSampleState.BorderColor[2] = 1.0f;
		anisotropicSampleState.BorderColor[3] = 1.0f;
		result = m_pDevice->CreateSamplerState(&anisotropicSampleState, &m_pAnisotropicSampler);
		if (FAILED(result))
			return result;

		//5. Bind RTV & DSV to Ouput Merger Stage
		//=====
		m_pDeviceContext->OMSetRenderTargets(1, &m_PRenderTargetView, m_pDepthStencilView);


		//6. Set Viewport
		//=====
		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(m_Width);
		viewport.Height = static_cast<float>(m_Height);
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		m_pDeviceContext->RSSetViewports(1, &viewport);
		//return S_FALSE;


		pDxgiFactory->Release();

		return result;
	}

	#pragma region Cyclers
	void Renderer::CycleSampler()
	{
		m_SampleMode == SampleMode::Anisotropic ?
			m_SampleMode = SampleMode(0) :
			m_SampleMode = SampleMode(static_cast<int>(m_SampleMode) + 1);

		switch (m_SampleMode)
		{
		case SampleMode::Point:
			std::cout << "-----Point Sampler-----\n";
			m_Meshes[0]->m_pEffect->ChangeSamplerState(m_pPointSampler);
			break;
		case SampleMode::Linear:
			std::cout << "-----Linear Sampler-----\n";
			m_Meshes[0]->m_pEffect->ChangeSamplerState(m_pLinearSampler);
			break;
		case SampleMode::Anisotropic:
			std::cout << "-----Anisotropic Sampler-----\n";
			m_Meshes[0]->m_pEffect->ChangeSamplerState(m_pAnisotropicSampler);
			break;
		default:
			break;
		}

	}

	void Renderer::CycleLightingMode() {
		//if it's combined, start from first enum, otherwise take the next
		m_LightingMode == LightingMode::Combined ?
			m_LightingMode = LightingMode(0) :
			m_LightingMode = LightingMode(static_cast<int>(m_LightingMode) + 1);

		switch (m_LightingMode)
		{
		case LightingMode::ObservedArea:
			std::cout << "-----Observed Area Lighting-----\n";
			break;
		case LightingMode::Diffuse:
			std::cout << "-----Diffuse Lighting-----\n";
			break;
		case LightingMode::Specular:
			std::cout << "-----Specular Lighting-----\n";
			break;
		case LightingMode::Combined:
			std::cout << "-----Combined Lighting-----\n";
			break;
		default:
			break;
		}
	}

	void Renderer::CycleCullMode() {
		m_CullMode == CullMode::Back ?
			m_CullMode = CullMode(0) :
			m_CullMode = CullMode(static_cast<int>(m_CullMode) + 1);

		switch (m_CullMode)
		{
		case CullMode::None:
			std::cout << "-----CullMode None-----\n";
			if (m_IsHardware) {
				m_pDeviceContext->RSSetState(m_pRasterizerStateNone);
			}
			break;
		case CullMode::Front:
			std::cout << "-----CullMode Front-----\n";
			if (m_IsHardware) {
				m_pDeviceContext->RSSetState(m_pRasterizerStateFront);
			}
			break;
		case CullMode::Back:
			std::cout << "-----CullMode Back-----\n";
			if (m_IsHardware) {
				m_pDeviceContext->RSSetState(m_pRasterizerStateBack);
			}
			break;
		default:
			break;
		}
	}
	#pragma endregion
}
