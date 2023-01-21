#pragma once
#include "pch.h"
#include "Matrix.h"
#include "Texture.h"

using namespace dae;
class Effect
{

public:
    Effect(ID3D11Device* device, const std::wstring& assetFile);
    ~Effect();
    static ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile);

    ID3DX11EffectTechnique* GetTechnique();
    void SetMatrix(const Matrix& matrix, const Matrix& worldMatrix, const Vector3& cameraPos);
    void SetMaps(Texture* pDiffuseTexture, Texture* pSpecularMap, Texture* pNormalMap, Texture* pGlossMap);
    void SetMaps(Texture* pDiffuseTexture);
    void ChangeEffect(LPCSTR name);
    void ChangeSamplerState(ID3D11SamplerState* sample);

private:
    ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable{ nullptr };
    ID3DX11EffectShaderResourceVariable* m_pGlossMapVariable{ nullptr };
    ID3DX11EffectShaderResourceVariable* m_pNormalMapVariable{ nullptr };
    ID3DX11EffectShaderResourceVariable* m_pSpecularMapVariable{ nullptr };
    ID3DX11Effect* m_pEffect{ nullptr };

    ID3DX11EffectTechnique* m_pTechnique{ nullptr };
    ID3DX11EffectMatrixVariable* m_pMatWorldViewProjVariable{ nullptr };
    ID3DX11EffectMatrixVariable* m_pWorldMatrixVariable{ nullptr };
    ID3DX11EffectVectorVariable* m_pOnbMatrixVariable{ nullptr };
    ID3DX11EffectSamplerVariable* m_pSamplerVariable{ nullptr };
};

