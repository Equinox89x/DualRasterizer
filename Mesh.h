#pragma once
#include "Vector3.h"
#include "Matrix.h"
#include "Datatypes.h"


enum class Technique {
    Point,
    Linear,
    Anisotropic,
    Flat
};

using namespace dae;
class Effect;
//
//struct Vertex_PosCol
//{
//    Vector3 Pos;
//    Vector3 Color;
//    Vector2 Uv;
//    Vector3 Normal;
//    Vector3 Tangent;
//};

class Mesh
{
public:

    Mesh(ID3D11Device* pDevice, std::vector<Vertex> vertices, std::vector<uint32_t> indices);
    ~Mesh();

    void SetMatrix(const Matrix& matrix, const Matrix& worldMatrix, const Vector3& cameraPos);
    void SetWorldMatrix(Matrix matrix) { m_WorldMatrix = matrix; };
    void Render(ID3D11DeviceContext* pDeviceContext) const;
    void CycleTechnique();

    Effect* m_pEffect{ nullptr };
    Matrix m_WorldMatrix{};
    std::vector<Vertex> vertices{};
    std::vector<uint32_t> indices{};

private:
    ID3DX11EffectTechnique* m_pTechnique{ nullptr };
    ID3D11InputLayout* m_pInputLayout{ nullptr };
    ID3D11Buffer* m_pVertexBuffer{ nullptr };
    ID3D11Buffer* m_pIndexBuffer{ nullptr };
    int m_NumIndices{ 0 };
    Technique m_Technique{ Technique::Point };
};