#pragma once
#include "Texture.h"
#include "Camera.h"
#include "Mesh.h"
#include "Datatypes.h"
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void Render() const;

		bool SaveBufferToImage() const;

		void ToggleRenderer() { 
			m_IsHardware = !m_IsHardware;
			m_IsHardware ? std::cout << "-----Hardware Rasterizer-----\n" : std::cout << "-----Software Rasterizer-----\\n";
		};

		void ToggleRotation() {
			m_HasRotation = !m_HasRotation;
			m_HasRotation ? std::cout << "-----Rotation on-----\n" : std::cout << "-----Rotation off-----\\n";
		};

		void ToggleNormalMap() { 
			m_HasNormalMap = !m_HasNormalMap; 
			m_HasNormalMap ? std::cout << "-----Normal map on-----\n" : std::cout << "-----Normal map off-----\\n";
		}

		void ToggleBoundingBox() { 
			m_HasBB = !m_HasBB; 
			m_HasBB ? std::cout << "-----bounding box on-----\n" : std::cout << "-----bounding box off-----\n";
		};

		void ToggleFire() { m_HasFire = !m_HasFire; };
		void ToggleDepthBuffer() { m_IsShowDepthBuffer = !m_IsShowDepthBuffer; };
		void ToggleClearColor() { m_HasClearColor = !m_HasClearColor; };

		void CycleCullMode();
		void CycleLightingMode();
		void CycleSampler();

	private:
		SDL_Window* m_pWindow{};
		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};
		float* m_pDepthBuffer{};
		ColorRGB* m_pColorBuffer{};

		CullMode m_CullMode{ CullMode::Back };
		SampleMode m_SampleMode{ SampleMode::Point };
		LightingMode m_LightingMode{ LightingMode::Combined };
		Vector3 m_LightDirection{ .577f, -.577f, .577f };
		const static int m_LightIntensity{ 7 };
		const static int m_Shininess{ 25 };

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };
		bool m_IsHardware{ false };

		bool m_HasRotation{ true };
		bool m_HasNormalMap{ true };
		bool m_HasFire{ true };
		bool m_IsShowDepthBuffer{ false };
		bool m_HasBB{ false };
		bool m_HasClearColor{ false };

		ColorRGB m_UniformColor{ .1f, .1f, .1f };
		ColorRGB m_HardwareColor{ .39f,.59f, .93f };
		ColorRGB m_SoftwareColor{ .39f, .39f, .39f };
		ColorRGB m_SelectedColor{ m_SoftwareColor };

		float m_RotationAngle{ 0 };
		const float m_RotationSpeed{ 45.f };

		Camera m_Camera{ };

		ID3D11Device* m_pDevice{ nullptr };
		ID3D11DeviceContext* m_pDeviceContext{ nullptr };
		IDXGISwapChain* m_pSwapChain{ nullptr };
		ID3D11Texture2D* m_pDepthStencilBuffer{ nullptr };
		ID3D11DepthStencilView* m_pDepthStencilView{ nullptr };
		ID3D11Resource* m_pRenderTargetBuffer{ nullptr };
		ID3D11RenderTargetView* m_PRenderTargetView{ nullptr };
		ID3D11RasterizerState* m_pRasterizerStateNone{ nullptr };
		ID3D11RasterizerState* m_pRasterizerStateFront{ nullptr };
		ID3D11RasterizerState* m_pRasterizerStateBack{ nullptr };
		ID3D11SamplerState* m_pPointSampler{ nullptr };
		ID3D11SamplerState* m_pLinearSampler{ nullptr };
		ID3D11SamplerState* m_pAnisotropicSampler{ nullptr };

		float m_test{};

		Texture* m_pTexture{ nullptr };
		Texture* m_pTextureGloss{ nullptr };
		Texture* m_pTextureNormal{ nullptr };
		Texture* m_pTextureSpecular{ nullptr };
		Texture* m_pCombustionTexture{ nullptr };

		Matrix m_TranslationTransform{};
		Matrix m_RotationTransform{};
		Matrix m_ScaleTransform{};

		std::vector<Mesh*> m_Meshes{ };


		//DIRECTX
		HRESULT InitializeDirectX();
		//...

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out, const Matrix& worldMatrix) const;

		void HandleRenderBB(std::vector<Vertex_Out>& verts, ColorRGB& finalColor) const;

		Vertex_Out CalculateVertexWithAttributes(const std::vector<Vertex_Out>& verts, float w0, float w1, float w2, float& outGloss, ColorRGB& outSpecularKS) const;
		ColorRGB PixelShading(const Vertex_Out& v, const float gloss, const ColorRGB specularKS) const;

		//void RenderMeshTriangleStrip(const Mesh& mesh) const;
		void RenderMeshTriangleList(const Mesh& mesh) const;
	};
}
