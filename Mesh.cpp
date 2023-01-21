#include "pch.h"
#include "Effect.h"
#include <cassert>
#include "Mesh.h"

Mesh::Mesh(ID3D11Device* pDevice, std::vector<Vertex> verts, std::vector<uint32_t> ind)
{
    m_pEffect = new Effect(pDevice, L"./Resources/PosCol3D.fx");
    m_pTechnique = m_pEffect->GetTechnique();

    //create Vertex Layout
    static constexpr uint32_t numElements{ 5 };
    D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

    vertexDesc[0].SemanticName = "POSITION";
    vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertexDesc[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertexDesc[1].SemanticName = "COLOR";
    vertexDesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertexDesc[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertexDesc[2].SemanticName = "TEXCOORD";
    vertexDesc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
    vertexDesc[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertexDesc[3].SemanticName = "NORMAL";
    vertexDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertexDesc[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertexDesc[4].SemanticName = "TANGENT";
    vertexDesc[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertexDesc[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    vertexDesc[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;


    //create input layout
    D3DX11_PASS_DESC passDesc{};
    m_pTechnique->GetPassByIndex(0)->GetDesc(&passDesc);

    /*const HRESULT result = pDevice->CreateInputLayout(*/
    HRESULT result = pDevice->CreateInputLayout(
        vertexDesc,
        numElements,
        passDesc.pIAInputSignature,
        passDesc.IAInputSignatureSize,
        &m_pInputLayout);

    if (FAILED(result))
        assert(false); // or return


    //create vertex buffer
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeof(Vertex) * static_cast<uint32_t>(verts.size());
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = verts.data();

    /*HRESULT result = pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);*/
    result = pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
    if (FAILED(result))
        return;

    //create indexBuffer
    m_NumIndices = static_cast<uint32_t>(ind.size());
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeof(uint32_t) * m_NumIndices;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    initData.pSysMem = ind.data();
    result = pDevice->CreateBuffer(&bd, &initData, &m_pIndexBuffer);
    if (FAILED(result))
        return;

    vertices = verts;
    indices = ind;
}

Mesh::~Mesh()
{
    //correct order, gives me no memory leaks according to VLD
    m_pTechnique = nullptr;
    m_pVertexBuffer->Release();
    m_pVertexBuffer = nullptr;
    m_pIndexBuffer->Release();
    m_pIndexBuffer = nullptr;
    m_pInputLayout->Release();
    m_pInputLayout = nullptr;

    if (m_pEffect) {
        delete m_pEffect;
        m_pEffect = nullptr;
    }
}

void Mesh::SetMatrix(const Matrix& matrix, const Matrix& worldMatrix, const Vector3& cameraPos)
{
    m_pEffect->SetMatrix(matrix, worldMatrix, cameraPos);
}

void Mesh::Render(ID3D11DeviceContext* pDeviceContext) const
{
    //1. set primitive topology
    pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //2. set input layout
    pDeviceContext->IASetInputLayout(m_pInputLayout); //Different than slides

    //3. set vertex buffer
    constexpr UINT stride = sizeof(Vertex);
    constexpr UINT offset = 0;
    pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

    //4. set indexBuffer
    pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    //5. Draw
    D3DX11_TECHNIQUE_DESC techDesc{};
    m_pEffect->GetTechnique()->GetDesc(&techDesc);
    for (UINT p = 0; p < techDesc.Passes; ++p)
    {
        //m_pEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, pDeviceContext);
        ID3DX11EffectPass* pass{ m_pEffect->GetTechnique()->GetPassByIndex(p) };
        pass->Apply(0, pDeviceContext);
        pDeviceContext->DrawIndexed(m_NumIndices, 0, 0);
    }
}

void Mesh::CycleTechnique()
{
    m_Technique == Technique::Anisotropic ?
        m_Technique = Technique(0) :
        m_Technique = Technique(static_cast<int>(m_Technique) + 1);


    switch (m_Technique)
    {
    case Technique::Point:
        std::cout << "Point Sampling \n";
        m_pEffect->ChangeEffect("DefaultTechnique");
        break;
    case Technique::Linear:
        std::cout << "Linear Sampling \n";
        m_pEffect->ChangeEffect("LinearTechnique");
        break;
    case Technique::Anisotropic:
        std::cout << "Anisotropic Sampling \n";
        m_pEffect->ChangeEffect("AniTechnique");
        break;
    case Technique::Flat:
        m_pEffect->ChangeEffect("FlatTechnique");
        break;
    default:
        break;
    }
    m_pTechnique = m_pEffect->GetTechnique();
}
