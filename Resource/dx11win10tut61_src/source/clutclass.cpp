////////////////////////////////////////////////////////////////////////////////
// Filename: clutclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "clutclass.h"


ClutClass::ClutClass()
{
    m_targaData = 0;
    m_texture = 0;
    m_textureView = 0;
}


ClutClass::ClutClass(const ClutClass& other)
{
}


ClutClass::~ClutClass()
{
}


bool ClutClass::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    char filename[256];
    bool result;


    // Set the filename of the CLUT data file.
    strcpy_s(filename, "../Engine/data/clut.dat");

    // Load the CLUT data from the file.
    result = LoadCLUTData(filename);
    if(!result)
    {
        return false;
    }

    // Create the texture from the targa data.
    result = BuildTexture(device, deviceContext);
    if(!result)
    {
        return false;
    }

    // Release the CLUT data now that the 3D texture has been created from it.
    ReleaseCLUTData();

    return true;
}


void ClutClass::Shutdown()
{
    // Release the texture view resource.
    if(m_textureView)
    {
        m_textureView->Release();
        m_textureView = 0;
    }

    // Release the texture.
    if(m_texture)
    {
        m_texture->Release();
        m_texture = 0;
    }

    // Release the CLUT data.
    if(m_targaData)
    {
        delete [] m_targaData;
        m_targaData = 0;
    }
    return;
}


bool ClutClass::LoadCLUTData(char* filename)
{
    FILE* filePtr;
    unsigned char* buffer;
    unsigned long long count, bufferSize, imageSize;
    int error, cubeSize, i, j, k, index, index2;


    // Open the targa file for reading in binary.
    error = fopen_s(&filePtr, filename, "rb");
    if(error != 0)
    {
        return false;
    }

    // Read in the cube size.
    count = fread(&cubeSize, sizeof(int), 1, filePtr);
    if(count != 1)
    {
        return false;
    }

    // Set the size of the buffer using the cube size.
    bufferSize = cubeSize * cubeSize * cubeSize * 3;

    // Create the buffer.
    buffer = new unsigned char[bufferSize];

    // Read the CLUT data into the buffer.
    count = fread(buffer, sizeof(unsigned char), bufferSize, filePtr);
    if(count != bufferSize)
    {
        return false;
    }

    // Close the file.
    error = fclose(filePtr);
    if(error != 0)
    {
        return false;
    }

    // Set the 3D texture size.
    m_textureHeight = cubeSize;
    m_textureWidth = cubeSize;
    m_textureDepth = cubeSize;

    // Calculate the size of the 32 bit image data.
    imageSize = cubeSize * cubeSize * cubeSize * 4;

    // Allocate memory for the targa destination data.
    m_targaData = new unsigned char[imageSize];

    // Copy the 24bit data into the 32 bit buffer.
    index  = 0;
    index2 = 0;

    for(k=0; k<cubeSize; k++)
    {
        for(j=0; j<cubeSize; j++)
        {
            for(i=0; i<cubeSize; i++)
            {
                m_targaData[index + 0] = buffer[index2 + 0];  // Red.
                m_targaData[index + 1] = buffer[index2 + 1];  // Green.
                m_targaData[index + 2] = buffer[index2 + 2];  // Blue
                m_targaData[index + 3] = (unsigned char)255;  // Alpha

                index  += 4;
                index2 += 3;
            }
        }
    }

    // Release the old buffer.
    delete [] buffer;
    buffer = 0;

    return true;
}


void ClutClass::ReleaseCLUTData()
{
    if(m_targaData)
    {
        delete [] m_targaData;
        m_targaData = 0;
    }

    return;
}


bool ClutClass::BuildTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    D3D11_TEXTURE3D_DESC textureDesc;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    HRESULT hResult;
    unsigned int rowPitch, depthPitch;


    // Setup the description of the texture.
    textureDesc.Height = m_textureHeight;
    textureDesc.Width = m_textureWidth;
    textureDesc.Depth = m_textureDepth;
    textureDesc.MipLevels = 0;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    // Create the empty texture.
    hResult = device->CreateTexture3D(&textureDesc, NULL, &m_texture);
    if(FAILED(hResult))
    {
        return false;
    }

    // Set the row pitch of the targa image data.
    rowPitch = (m_textureWidth * 4) * sizeof(unsigned char);
    depthPitch = rowPitch * m_textureHeight;  // Source Depth Pitch = [Source Row Pitch] * [number of rows (height)]

    // Copy the targa image data into the texture.
    deviceContext->UpdateSubresource(m_texture, 0, NULL, m_targaData, rowPitch, depthPitch);

    // Setup the shader resource view description.
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Texture3D.MostDetailedMip = 0;
    srvDesc.Texture3D.MipLevels = -1;

    // Create the shader resource view for the texture.
    hResult = device->CreateShaderResourceView(m_texture, &srvDesc, &m_textureView);
    if(FAILED(hResult))
    {
        return false;
    }

    // Generate mipmaps for this texture.
    deviceContext->GenerateMips(m_textureView);

    return true;
}


ID3D11ShaderResourceView* ClutClass::GetTexture()
{
    return m_textureView;
}
