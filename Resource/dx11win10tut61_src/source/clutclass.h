////////////////////////////////////////////////////////////////////////////////
// Filename: clutclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _CLUTCLASS_H_
#define _CLUTCLASS_H_


//////////////
// INCLUDES //
//////////////
#include <d3d11.h>
#include <stdio.h>


////////////////////////////////////////////////////////////////////////////////
// Class name: ClutClass
////////////////////////////////////////////////////////////////////////////////
class ClutClass
{
public:
    ClutClass();
    ClutClass(const ClutClass&);
    ~ClutClass();

    bool Initialize(ID3D11Device*, ID3D11DeviceContext*);
    void Shutdown();

    ID3D11ShaderResourceView* GetTexture();

private:
    bool LoadCLUTData(char*);
    void ReleaseCLUTData();

    bool BuildTexture(ID3D11Device*, ID3D11DeviceContext*);

private:
    ID3D11Texture3D* m_texture;
    ID3D11ShaderResourceView* m_textureView;
    unsigned char* m_targaData;
    int m_textureWidth, m_textureHeight, m_textureDepth;
};

#endif
