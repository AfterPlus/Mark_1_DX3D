#include "TextureClass.h"

TextureClass::TextureClass()
{
    m_targaData = nullptr;
    m_texture = nullptr;
    m_textureView = nullptr;
}

TextureClass::TextureClass(const TextureClass&)
{
	
}

TextureClass::~TextureClass()
{
	
}

bool TextureClass::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* filename)
{
    // use Map and Unmap for resources that would keep changing for every frame.  
    // Use UpdateSubresource for resource that updates only few times. 
    bool result;
    int height, width;
    D3D11_TEXTURE2D_DESC textureDesc;
    HRESULT hResult;
    unsigned int rowPitch;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    
    // Load the TARGA image data into memory.
    result = LoadTarga32Bit(filename);
    if(!result)
        return false;
    
    // Setup the description of the texture.
    textureDesc.Height = m_height;
    textureDesc.Width = m_width;
    textureDesc.MipLevels = 0;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    // Create the empty texture.
    hResult = device->CreateTexture2D(&textureDesc, NULL, &m_texture);
    if(FAILED(hResult))
    {
        return false;
    }

    // Set the row pitch of the targa image data.
    rowPitch = (m_width * 4) * sizeof(unsigned char);
    
    // Copy the targa image data into the texture.
    deviceContext->UpdateSubresource(m_texture, 0, NULL, m_targaData, rowPitch, 0);
    
    // You can create Mipmap using GenerateMipMap or use your MimMap.
    // Setup the shader resource view description.
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1;

    // Create the shader resource view for the texture.
    hResult = device->CreateShaderResourceView(m_texture, &srvDesc, &m_textureView);
    if(FAILED(hResult))
    {
        return false;
    }

    // Generate mipmaps for this texture.
    deviceContext->GenerateMips(m_textureView);

    // Release the targa image data now that the image data has been loaded into the texture.
    delete [] m_targaData;
    m_targaData = 0;

    return true;
}

void TextureClass::Shutdown()
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

    // Release the targa data.
    if(m_targaData)
    {
        delete [] m_targaData;
        m_targaData = 0;
    }

    return;
}

ID3D11ShaderResourceView* TextureClass::GetTexture()
{
    return m_textureView;
}

// Loads an uncompressed Targa, 24 or 32 bits per pixel, stored bottom-up or top-down (the name is historical).
// The result in m_targaData is always top-down RGBA, which is what the texture upload expects.
bool TextureClass::LoadTarga32Bit(char* filename)
{
	FILE* filePtr;
	TargaHeader header;
	unsigned char* image;
	const unsigned char* source;
	unsigned char* destination;
	int bytesPerPixel, imageSize, i, j, sourceRow;
	bool topDown;
	size_t count;

	if(fopen_s(&filePtr, filename, "rb") != 0)
	{
		return false;
	}

	if(fread(&header, sizeof(TargaHeader), 1, filePtr) != 1)
	{
		fclose(filePtr);
		return false;
	}

	// Only uncompressed true-colour images (image type 2) with 24 or 32 bits per pixel.
	if(header.data1[2] != 2 || (header.bpp != 24 && header.bpp != 32))
	{
		fclose(filePtr);
		return false;
	}

	// Skip the optional image ID field that sits between the header and the pixels.
	if(header.data1[0] > 0)
	{
		fseek(filePtr, header.data1[0], SEEK_CUR);
	}

	m_width = (int)header.width;
	m_height = (int)header.height;
	bytesPerPixel = header.bpp / 8;
	imageSize = m_width * m_height * bytesPerPixel;

	// Bit 5 of the descriptor byte: 1 = the first row in the file is the TOP row, 0 = it is the bottom row.
	topDown = (header.data2 & 0x20) != 0;

	image = new unsigned char[imageSize];

	count = fread(image, 1, imageSize, filePtr);
	fclose(filePtr);

	if((int)count != imageSize)
	{
		delete [] image;
		return false;
	}

	// Copy into top-down RGBA (Targa stores the colours as BGR(A)). A 24 bit image gets full alpha.
	m_targaData = new unsigned char[m_width * m_height * 4];

	for(j=0; j<m_height; j++)
	{
		sourceRow = topDown ? j : (m_height - 1 - j);
		source = image + (sourceRow * m_width * bytesPerPixel);
		destination = m_targaData + (j * m_width * 4);

		for(i=0; i<m_width; i++)
		{
			destination[0] = source[2];
			destination[1] = source[1];
			destination[2] = source[0];
			destination[3] = (bytesPerPixel == 4) ? source[3] : 255;

			source += bytesPerPixel;
			destination += 4;
		}
	}

	delete [] image;

	return true;
}


int TextureClass::GetWidth()
{
    return m_width;
}


int TextureClass::GetHeight()
{
    return m_height;
}


