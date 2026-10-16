#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <wincodec.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
using namespace DirectX;

// A sky around the camera: a cube map (six PNG faces) drawn behind everything.
// The folder holds px/nx/py/ny/pz/nz.png (+X right, +Y up, +Z forward, as in the world).
// Draw it after the opaque scene: it only fills pixels the scene left at the far plane.
//
// Header-only, with the shader in the source, so it needs no project or _Shader entries.
class SkyboxClass
{
private:
    struct MatrixBufferType
    {
        XMMATRIX viewProjection;    // view without its translation (the sky never gets closer) * projection
    };

public:
    SkyboxClass()
    {
        m_vertexShader = nullptr;
        m_pixelShader = nullptr;
        m_matrixBuffer = nullptr;
        m_sampleState = nullptr;
        m_depthState = nullptr;
        m_rasterState = nullptr;
        m_texture = nullptr;
        m_textureView = nullptr;
    }

    ~SkyboxClass()
    {
    }

    // folder: e.g. "Resources/SkyBox/Day" (relative to the working directory, like every other asset).
    bool Initialize(ID3D11Device* device, HWND hwnd, const char* folder)
    {
        if(!InitializeShader(device, hwnd))
        {
            return false;
        }

        if(!LoadCubeMap(device, folder))
        {
            return false;
        }

        return true;
    }

    void Shutdown()
    {
        Release(m_textureView);
        Release(m_texture);
        Release(m_rasterState);
        Release(m_depthState);
        Release(m_sampleState);
        Release(m_matrixBuffer);
        Release(m_pixelShader);
        Release(m_vertexShader);
    }

    // The cube is made in the vertex shader from SV_VertexID: no vertex or index buffer.
    bool Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix)
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        MatrixBufferType* data;
        ID3D11DepthStencilState* previousDepthState;
        ID3D11RasterizerState* previousRasterState;
        UINT previousStencilRef;
        HRESULT result;

        viewMatrix.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

        result = deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if(FAILED(result))
        {
            return false;
        }

        data = (MatrixBufferType*)mappedResource.pData;
        data->viewProjection = XMMatrixTranspose(XMMatrixMultiply(viewMatrix, projectionMatrix));

        deviceContext->Unmap(m_matrixBuffer, 0);

        deviceContext->OMGetDepthStencilState(&previousDepthState, &previousStencilRef);
        deviceContext->RSGetState(&previousRasterState);

        deviceContext->IASetInputLayout(NULL);
        deviceContext->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->VSSetShader(m_vertexShader, NULL, 0);
        deviceContext->VSSetConstantBuffers(0, 1, &m_matrixBuffer);
        deviceContext->PSSetShader(m_pixelShader, NULL, 0);
        deviceContext->PSSetShaderResources(0, 1, &m_textureView);
        deviceContext->PSSetSamplers(0, 1, &m_sampleState);
        deviceContext->OMSetDepthStencilState(m_depthState, 0);
        deviceContext->RSSetState(m_rasterState);

        deviceContext->Draw(36, 0);

        deviceContext->OMSetDepthStencilState(previousDepthState, previousStencilRef);
        deviceContext->RSSetState(previousRasterState);

        // The Get calls added a reference.
        Release(previousDepthState);
        Release(previousRasterState);

        return true;
    }

private:
    template<typename T> static void Release(T*& object)
    {
        if(object)
        {
            object->Release();
            object = nullptr;
        }
    }

    bool InitializeShader(ID3D11Device* device, HWND hwnd)
    {
        // The vertex keeps z = w, so after the divide it sits exactly on the far plane (depth 1): drawn with
        // LESS_EQUAL and no depth write, it shows only where the scene drew nothing.
        static const char shaderSource[] =
            "cbuffer MatrixBuffer : register(b0)\n"
            "{\n"
            "    matrix viewProjection;\n"
            "};\n"
            "TextureCube skyTexture : register(t0);\n"
            "SamplerState skySampler : register(s0);\n"
            "struct PixelInputType\n"
            "{\n"
            "    float4 position : SV_POSITION;\n"
            "    float3 direction : TEXCOORD0;\n"
            "};\n"
            "static const float3 corners[8] =\n"
            "{\n"
            "    float3(-1, -1, -1), float3( 1, -1, -1), float3(-1,  1, -1), float3( 1,  1, -1),\n"
            "    float3(-1, -1,  1), float3( 1, -1,  1), float3(-1,  1,  1), float3( 1,  1,  1)\n"
            "};\n"
            "static const uint indices[36] =\n"
            "{\n"
            "    0, 2, 1,  1, 2, 3,\n"   // -Z
            "    4, 5, 6,  5, 7, 6,\n"   // +Z
            "    0, 4, 2,  2, 4, 6,\n"   // -X
            "    1, 3, 5,  3, 7, 5,\n"   // +X
            "    0, 1, 4,  1, 5, 4,\n"   // -Y
            "    2, 6, 3,  3, 6, 7\n"    // +Y
            "};\n"
            "PixelInputType SkyboxVertexShader(uint vertexId : SV_VertexID)\n"
            "{\n"
            "    PixelInputType output;\n"
            "    float3 corner = corners[indices[vertexId]];\n"
            "    output.position = mul(float4(corner, 1.0f), viewProjection).xyww;\n"
            "    output.direction = corner;\n"
            "    return output;\n"
            "}\n"
            "float4 SkyboxPixelShader(PixelInputType input) : SV_TARGET\n"
            "{\n"
            "    return float4(skyTexture.Sample(skySampler, input.direction).rgb, 1.0f);\n"
            "}\n";

        ID3D10Blob* vertexShaderBuffer;
        ID3D10Blob* pixelShaderBuffer;
        ID3D10Blob* errorMessage;
        D3D11_BUFFER_DESC bufferDesc;
        D3D11_SAMPLER_DESC samplerDesc;
        D3D11_DEPTH_STENCIL_DESC depthDesc;
        D3D11_RASTERIZER_DESC rasterDesc;
        HRESULT result;

        vertexShaderBuffer = nullptr;
        pixelShaderBuffer = nullptr;
        errorMessage = nullptr;

        result = D3DCompile(shaderSource, sizeof(shaderSource) - 1, "Skybox", NULL, NULL, "SkyboxVertexShader", "vs_5_0",
                            D3D10_SHADER_ENABLE_STRICTNESS, 0, &vertexShaderBuffer, &errorMessage);
        if(FAILED(result))
        {
            ShowShaderError(errorMessage, hwnd);
            return false;
        }

        result = D3DCompile(shaderSource, sizeof(shaderSource) - 1, "Skybox", NULL, NULL, "SkyboxPixelShader", "ps_5_0",
                            D3D10_SHADER_ENABLE_STRICTNESS, 0, &pixelShaderBuffer, &errorMessage);
        if(FAILED(result))
        {
            ShowShaderError(errorMessage, hwnd);
            vertexShaderBuffer->Release();
            return false;
        }

        result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_vertexShader);
        vertexShaderBuffer->Release();
        if(FAILED(result))
        {
            pixelShaderBuffer->Release();
            return false;
        }

        result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pixelShader);
        pixelShaderBuffer->Release();
        if(FAILED(result))
        {
            return false;
        }

        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.ByteWidth = sizeof(MatrixBufferType);
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

        result = device->CreateBuffer(&bufferDesc, NULL, &m_matrixBuffer);
        if(FAILED(result))
        {
            return false;
        }

        ZeroMemory(&samplerDesc, sizeof(samplerDesc));
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        result = device->CreateSamplerState(&samplerDesc, &m_sampleState);
        if(FAILED(result))
        {
            return false;
        }

        // Test against the scene's depth, but do not write: the sky is infinitely far away.
        ZeroMemory(&depthDesc, sizeof(depthDesc));
        depthDesc.DepthEnable = true;
        depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        depthDesc.StencilEnable = false;

        result = device->CreateDepthStencilState(&depthDesc, &m_depthState);
        if(FAILED(result))
        {
            return false;
        }

        // Seen from inside, so no culling. No depth clip either: rounding can put z/w a hair past 1.
        ZeroMemory(&rasterDesc, sizeof(rasterDesc));
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.DepthClipEnable = false;
        rasterDesc.MultisampleEnable = true;

        result = device->CreateRasterizerState(&rasterDesc, &m_rasterState);
        if(FAILED(result))
        {
            return false;
        }

        return true;
    }

    void ShowShaderError(ID3D10Blob* errorMessage, HWND hwnd)
    {
        if(errorMessage)
        {
            MessageBoxA(hwnd, (char*)errorMessage->GetBufferPointer(), "Skybox shader error", MB_OK);
            errorMessage->Release();
        }
    }

    // Reads the six faces with WIC (any format it knows, converted to 8-bit RGBA) into one cube texture.
    bool LoadCubeMap(ID3D11Device* device, const char* folder)
    {
        // D3D11 cube face order.
        static const char* faceNames[6] = { "px", "nx", "py", "ny", "pz", "nz" };

        IWICImagingFactory* factory;
        std::vector<unsigned char> pixels[6];
        D3D11_SUBRESOURCE_DATA faceData[6];
        D3D11_TEXTURE2D_DESC textureDesc;
        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
        UINT width, height, faceWidth, faceHeight;
        HRESULT result, comResult;
        bool loaded;
        int i;

        // WIC needs COM on this thread. S_FALSE / RPC_E_CHANGED_MODE mean it is already set up, which is fine.
        comResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

        factory = nullptr;
        result = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));

        loaded = SUCCEEDED(result);
        faceWidth = 0;
        faceHeight = 0;

        for(i=0; i<6 && loaded; i++)
        {
            char path[260];
            wchar_t widePath[260];

            sprintf_s(path, "%s/%s.png", folder, faceNames[i]);
            MultiByteToWideChar(CP_ACP, 0, path, -1, widePath, 260);

            loaded = LoadImageRGBA(factory, widePath, pixels[i], width, height);

            // Every face must be the same square size.
            if(loaded && i == 0)
            {
                faceWidth = width;
                faceHeight = height;
            }

            loaded = loaded && width == faceWidth && height == faceHeight && width == height;

            faceData[i].pSysMem = loaded ? pixels[i].data() : NULL;
            faceData[i].SysMemPitch = width * 4;
            faceData[i].SysMemSlicePitch = 0;
        }

        if(factory)
        {
            factory->Release();
        }

        if(SUCCEEDED(comResult))
        {
            CoUninitialize();
        }

        if(!loaded)
        {
            return false;
        }

        textureDesc.Width = faceWidth;
        textureDesc.Height = faceHeight;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 6;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        result = device->CreateTexture2D(&textureDesc, faceData, &m_texture);
        if(FAILED(result))
        {
            return false;
        }

        viewDesc.Format = textureDesc.Format;
        viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        viewDesc.TextureCube.MostDetailedMip = 0;
        viewDesc.TextureCube.MipLevels = 1;

        result = device->CreateShaderResourceView(m_texture, &viewDesc, &m_textureView);
        if(FAILED(result))
        {
            return false;
        }

        return true;
    }

    static bool LoadImageRGBA(IWICImagingFactory* factory, const wchar_t* filename, std::vector<unsigned char>& pixels, UINT& width, UINT& height)
    {
        IWICBitmapDecoder* decoder;
        IWICBitmapFrameDecode* frame;
        IWICFormatConverter* converter;
        HRESULT result;

        decoder = nullptr;
        frame = nullptr;
        converter = nullptr;
        width = 0;
        height = 0;

        result = factory->CreateDecoderFromFilename(filename, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
        if(SUCCEEDED(result))
        {
            result = decoder->GetFrame(0, &frame);
        }

        if(SUCCEEDED(result))
        {
            result = factory->CreateFormatConverter(&converter);
        }

        if(SUCCEEDED(result))
        {
            result = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);
        }

        if(SUCCEEDED(result))
        {
            result = converter->GetSize(&width, &height);
        }

        if(SUCCEEDED(result))
        {
            pixels.resize((size_t)width * height * 4);
            result = converter->CopyPixels(NULL, width * 4, (UINT)pixels.size(), pixels.data());
        }

        Release(converter);
        Release(frame);
        Release(decoder);

        return SUCCEEDED(result);
    }

private:
    ID3D11VertexShader* m_vertexShader;
    ID3D11PixelShader* m_pixelShader;
    ID3D11Buffer* m_matrixBuffer;
    ID3D11SamplerState* m_sampleState;
    ID3D11DepthStencilState* m_depthState;
    ID3D11RasterizerState* m_rasterState;
    ID3D11Texture2D* m_texture;
    ID3D11ShaderResourceView* m_textureView;
};
