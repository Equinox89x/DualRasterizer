#include "pch.h"
#include "Effect.h"

Effect::Effect(ID3D11Device* device, const std::wstring& assetFile)
{
    m_pEffect = std::move(LoadEffect(device, assetFile));

    //technique
    //m_pTechnique = m_pEffect->GetTecniqueByIndex(0);
    m_pTechnique = m_pEffect->GetTechniqueByName("DefaultTechnique");
    if (!m_pTechnique->IsValid())
        std::wcout << L"Technique is not valid\n";

    m_pMatWorldViewProjVariable = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
    if (!m_pMatWorldViewProjVariable->IsValid()) {
        std::wcout << L"m_pMatWorldViewProjVariable is not valid! \n";
    }

    //maps
    m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
    if (!m_pDiffuseMapVariable->IsValid()) {
        std::wcout << L"m_pDiffuseMapVariable is not valid! \n";
    }
    m_pGlossMapVariable = m_pEffect->GetVariableByName("gGlossyMap")->AsShaderResource();
    if (!m_pDiffuseMapVariable->IsValid()) {
        std::wcout << L"m_pGlossMapVariable is not valid! \n";
    }
    m_pNormalMapVariable = m_pEffect->GetVariableByName("gNormalMap")->AsShaderResource();
    if (!m_pDiffuseMapVariable->IsValid()) {
        std::wcout << L"m_pNormalMapVariable is not valid! \n";
    }
    m_pSpecularMapVariable = m_pEffect->GetVariableByName("gSpecularMap")->AsShaderResource();
    if (!m_pDiffuseMapVariable->IsValid()) {
        std::wcout << L"m_pSpecularMapVariable is not valid! \n";
    }

    //matrix
    m_pWorldMatrixVariable = m_pEffect->GetVariableByName("gWorldMatrix")->AsMatrix();
    if (!m_pWorldMatrixVariable->IsValid()) {
        std::wcout << L"m_pWorldMatrixVariable is not valid! \n";
    }

    m_pOnbMatrixVariable = m_pEffect->GetVariableByName("gOnb")->AsVector();
    if (!m_pOnbMatrixVariable->IsValid()) {
        std::wcout << L"m_pOnbMatrixVariable is not valid! \n";
    }

    m_pSamplerVariable = m_pEffect->GetVariableByName("gSampler")->AsSampler();
    if (!m_pSamplerVariable->IsValid()) {
        std::wcout << L"m_pSamplerVariable is not valid! \n";
    }
}

Effect::~Effect()
{
    //correct order, gives me no memory leaks according to VLD
    m_pDiffuseMapVariable->Release();
    m_pDiffuseMapVariable = nullptr;

    m_pGlossMapVariable->Release();
    m_pGlossMapVariable = nullptr;

    m_pNormalMapVariable->Release();
    m_pNormalMapVariable = nullptr;
    
    m_pSpecularMapVariable->Release();
    m_pSpecularMapVariable = nullptr;
    
    m_pWorldMatrixVariable->Release();
    m_pWorldMatrixVariable = nullptr;
    
    m_pMatWorldViewProjVariable->Release();
    m_pMatWorldViewProjVariable = nullptr;
    
    m_pOnbMatrixVariable->Release();
    m_pOnbMatrixVariable = nullptr;    
    
    m_pSamplerVariable->Release();
    m_pSamplerVariable = nullptr;
   
    m_pEffect->GetVariableByName("gWorldViewProj")->Release();
    m_pEffect->GetVariableByName("gDiffuseMap")->Release();
    m_pEffect->GetVariableByName("gGlossyMap")->Release();
    m_pEffect->GetVariableByName("gNormalMap")->Release();
    m_pEffect->GetVariableByName("gSpecularMap")->Release();
    m_pEffect->GetVariableByName("gWorldMatrix")->Release();
    m_pEffect->GetVariableByName("gOnb")->Release();
    m_pEffect->GetVariableByName("gSampler")->Release();

    m_pEffect->GetTechniqueByName("DefaultTechnique")->Release();
    m_pEffect->GetTechniqueByName("LinearTechnique")->Release();
    m_pEffect->GetTechniqueByName("AniTechnique")->Release();
    m_pEffect->GetTechniqueByName("FlatTechnique")->Release();

    if (m_pTechnique) {
        m_pTechnique->Release();
        m_pTechnique = nullptr;
    }

    if (m_pEffect) {
        m_pEffect->Release();
        m_pEffect = nullptr;
    }
}

ID3DX11Effect* Effect::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
{
    HRESULT result{};
    ID3D10Blob* pErrorBlob{ nullptr };
    ID3DX11Effect* pEffect{ nullptr };

    DWORD shaderFlags{ 0 };
#if defined(DEBUG) || defined(_DEBUG)
    shaderFlags |= D3DCOMPILE_DEBUG;
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    result = D3DX11CompileEffectFromFile(assetFile.c_str(),
        nullptr,
        nullptr,
        shaderFlags,
        0,
        pDevice,
        &pEffect,
        &pErrorBlob);

    //D3DX11CompileEffectFromFile(L"color.fx", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, shaderFlags,
    //    0, pDevice, &pEffect, &pErrorBlob);

    if (FAILED(result))
    {
        if (pErrorBlob != nullptr)
        {
            const char* pErrors{ static_cast<char*>(pErrorBlob->GetBufferPointer()) };

            std::wstringstream ss;
            for (unsigned int i = 0; i < pErrorBlob->GetBufferSize(); i++)
                ss << pErrors[i];

            OutputDebugStringW(ss.str().c_str());
            pErrorBlob->Release();
            pErrorBlob = nullptr;

            std::wcout << ss.str() << std::endl;
        }
        else
        {
            std::wstringstream ss;
            ss << "EffectLoader: Failed to CreateEffectFromFile!\nPath: " << assetFile;
            std::wcout << ss.str() << std::endl;
            return nullptr;
        }
    }
    if (pErrorBlob) pErrorBlob->Release();
    return pEffect;
}

ID3DX11EffectTechnique* Effect::GetTechnique()
{
    return m_pTechnique;
}

void Effect::SetMatrix(const Matrix& matrix, const Matrix& worldMatrix, const Vector3& cameraPos) {
    m_pMatWorldViewProjVariable->SetMatrix(reinterpret_cast<const float*>(&matrix));
    m_pWorldMatrixVariable->SetMatrix(reinterpret_cast<const float*>(&worldMatrix));
    m_pOnbMatrixVariable->SetFloatVector(reinterpret_cast<const float*>(&cameraPos));
}

void Effect::SetMaps(Texture* pDiffuseTexture, Texture* pSpecularMap, Texture* pNormalMap, Texture* pGlossMap)
{
    if (m_pDiffuseMapVariable) {
        m_pDiffuseMapVariable->SetResource(pDiffuseTexture->GetSRV());
    }
    if (m_pSpecularMapVariable) {
        m_pSpecularMapVariable->SetResource(pSpecularMap->GetSRV());
    }
    if (m_pNormalMapVariable) {
        m_pNormalMapVariable->SetResource(pNormalMap->GetSRV());
    }
    if (m_pGlossMapVariable) {
        m_pGlossMapVariable->SetResource(pGlossMap->GetSRV());
    }
}

void Effect::SetMaps(Texture* pDiffuseTexture)
{
    if (m_pDiffuseMapVariable) {
        m_pDiffuseMapVariable->SetResource(pDiffuseTexture->GetSRV());
    }
}

void Effect::ChangeEffect(LPCSTR name)
{
    m_pTechnique = m_pEffect->GetTechniqueByName(name);
}

void Effect::ChangeSamplerState(ID3D11SamplerState* sampler)
{
    m_pSamplerVariable->SetSampler(0, sampler);
}