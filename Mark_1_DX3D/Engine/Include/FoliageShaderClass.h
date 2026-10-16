#ifndef _FOLIAGESHADERCLASS_H_
#define _FOLIAGESHADERCLASS_H_

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <fstream>
#include "ModelClass.h"
using namespace DirectX;
using namespace std;

// One copy of a foliage model. Models are 1 unit tall with the root at the origin (see Tools/blender/dh_export_grass.py).
// This is also the GPU layout of the per-instance vertex buffer, so keep it 16-byte friendly.
struct FoliageInstance
{
    XMFLOAT3 position;      // root, on the ground
    float yaw;              // radians around Y
    float height;           // metres
    float width;            // sideways scale relative to the height (1 = model's own proportions)
    float phase;            // wind phase, so neighbours do not sway in step
    float stiffness;        // 0 = does not move, 1 = full wind
    XMFLOAT4 tint;          // rgb colour multiplier (a unused)
};

// Wind and fade settings for a frame of foliage.
struct FoliageWind
{
    XMFLOAT2 direction;     // XZ, normalised
    float strength;         // how far the tips lean, in blade heights
    float speed;            // sway speed
    float time;             // seconds, keeps running
    float fadeStart;        // metres from the camera where the grass starts shrinking away ...
    float fadeEnd;          // ... and where it is gone
};

// Draws many copies of a model in one call (instancing), cut out by the texture's alpha, both sides, bent by the wind.
// Use: Begin (uploads every instance of the frame once) -> Draw per model -> End (restores the render states).
class FoliageShaderClass
{
private:
    struct FrameBufferType
    {
        XMMATRIX view;
        XMMATRIX projection;
        XMFLOAT3 lightPosition;
        float time;
        XMFLOAT3 cameraPosition;
        float windStrength;
        XMFLOAT2 windDirection;
        float windSpeed;
        float fadeStart;
        float fadeEnd;
        XMFLOAT3 padding;
    };

    struct LightBufferType
    {
        XMFLOAT4 ambientColor;
        XMFLOAT4 diffuseColor;
    };

public:
    FoliageShaderClass();
    ~FoliageShaderClass();

    bool Initialize(ID3D11Device*, HWND, int maxInstances);
    void Shutdown();

    // Instances beyond maxInstances are dropped.
    bool Begin(ID3D11DeviceContext*, const FoliageInstance* instances, int instanceCount, XMMATRIX viewMatrix, XMMATRIX projectionMatrix,
               XMFLOAT3 cameraPosition, XMFLOAT3 lightPosition, XMFLOAT4 ambientColor, XMFLOAT4 diffuseColor,
               const FoliageWind& wind, ID3D11ShaderResourceView* shadowTexture);
    // Draws instances [firstInstance, firstInstance + instanceCount) of the ones given to Begin, as copies of model.
    void Draw(ID3D11DeviceContext*, ModelClass* model, int firstInstance, int instanceCount);
    void End(ID3D11DeviceContext*);

private:
    bool InitializeShader(ID3D11Device*, HWND, WCHAR*, WCHAR*);
    void ShutdownShader();
    void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);

private:
    ID3D11VertexShader* m_vertexShader;
    ID3D11PixelShader* m_pixelShader;
    ID3D11InputLayout* m_layout;
    ID3D11Buffer* m_frameBuffer;
    ID3D11Buffer* m_lightBuffer;
    ID3D11Buffer* m_instanceBuffer;
    ID3D11SamplerState* m_sampleStateClamp;
    ID3D11SamplerState* m_sampleStateWrap;
    ID3D11RasterizerState* m_rasterState;       // no culling: blades are single sheets seen from both sides
    ID3D11BlendState* m_blendState;             // alpha-to-coverage

    ID3D11RasterizerState* m_previousRasterState;
    ID3D11BlendState* m_previousBlendState;
    FLOAT m_previousBlendFactor[4];
    UINT m_previousSampleMask;

    int m_maxInstances;
    int m_instanceCount;
};

#endif
