
#ifndef _MODELCLASS_H_
#define _MODELCLASS_H_

#include <d3d11.h>
#include <directxmath.h>
#include <fstream>
using namespace std;
#include "TextureClass.h"
using namespace DirectX;

class ModelClass
{

private:
    struct VertexType
    {
        XMFLOAT4 position;
        XMFLOAT2 texture;
        XMFLOAT3 normal;
        XMFLOAT3 tangent;
        XMFLOAT3 binormal;
    };

    struct ModelType
    {
        float x, y, z;
        float tu, tv;
        float nx, ny, nz;
        float tx, ty, tz;
        float bx, by, bz;
    };

    struct TempVertexType
    {
        float x, y, z;
        float tu, tv;
        float nx, ny, nz;
    };

    struct VectorType
    {
        float x, y, z;
    };

public:
    ModelClass();
    ~ModelClass();

    bool Initialize(ID3D11Device*, ID3D11DeviceContext*, char*, char*, char*, char* = nullptr);
    void Shutdown();
    void Render(ID3D11DeviceContext*);

    int GetIndexCount();
    ID3D11ShaderResourceView* GetTexture(int);
    ID3D11ShaderResourceView* GetTexture();

private:
    bool InitializeBuffers(ID3D11Device*);
    void ShutdownBuffers();
    void RenderBuffers(ID3D11DeviceContext*);

    bool LoadTexture(ID3D11Device*, ID3D11DeviceContext*, char*, char*, char*);
    void ReleaseTexture();
    bool LoadModel(char*);
    void ReleaseModel();
    void CalculateModelVectors();
    void CalculateTangentBinormal(TempVertexType, TempVertexType, TempVertexType, VectorType&, VectorType&);

private:
    ID3D11Buffer *m_vertexBuffer, *m_indexBuffer;
    int m_vertexCount, m_indexCount;
    TextureClass* m_Textures;
    ModelType* m_model;

};

#endif

