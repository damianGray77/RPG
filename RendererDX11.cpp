#include "pch.h"
#include "RendererDX11.h"

bool RendererDX11::init_buffer(HWND window, void** bits, const uint32 width, const uint32 height) {
	this->window = window;
	this->bits = bits;

	if (texture) { texture->Release(); texture = NULL; }
	if (srv)     { srv    ->Release();     srv = NULL; }

	DXGI_SWAP_CHAIN_DESC scd = {};
	scd.BufferCount       = 2;
	scd.BufferDesc.Width  = width;
	scd.BufferDesc.Height = height;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow      = window;
	scd.SampleDesc.Count  = 1;
	scd.Windowed          = TRUE;
	scd.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
		  NULL
		, D3D_DRIVER_TYPE_HARDWARE
		, NULL
		, 0
		, NULL
		, 0
		, D3D11_SDK_VERSION
		, &scd
		, &swap_chain
		, &device
		, &fl
		, &context
    );

	if (FAILED(hr)) { return false; }

	ID3D11Texture2D* back_buffer = NULL;
	swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer);
	device->CreateRenderTargetView(back_buffer, NULL, &rtv);
	back_buffer->Release();

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width    = (float)width;
	viewport.Height   = (float)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	context->RSSetViewports(1, &viewport);

	D3D11_TEXTURE2D_DESC tex_desc = {};
	tex_desc.Width            = width;
	tex_desc.Height           = height;
	tex_desc.MipLevels        = 1;
	tex_desc.ArraySize        = 1;
	tex_desc.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.Usage            = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
	tex_desc.CPUAccessFlags   = 0;
	tex_desc.MiscFlags        = 0;
	device->CreateTexture2D(&tex_desc, NULL, &texture);

	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = tex_desc.Format;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(texture, &srv_desc, &srv);

	D3D11_SAMPLER_DESC samp_desc = {};
	samp_desc.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	device->CreateSamplerState(&samp_desc, &sampler);

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(quad);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA initData = { quad };
	device->CreateBuffer(&bd, &initData, &vbuffer);

	ID3DBlob* vsblob = NULL;
	ID3DBlob* psblob = NULL;
	D3DCompile(vscode, strlen(vscode), NULL, NULL, NULL, "main", "vs_4_0", 0, 0, &vsblob, NULL);
	D3DCompile(pscode, strlen(pscode), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &psblob, NULL);

	device->CreateVertexShader(vsblob->GetBufferPointer(), vsblob->GetBufferSize(), NULL, &vshader);
	device->CreatePixelShader (psblob->GetBufferPointer(), psblob->GetBufferSize(), NULL, &pshader);

	// Input layout
	D3D11_INPUT_ELEMENT_DESC ied[] = {
		  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex3uv, x), D3D11_INPUT_PER_VERTEX_DATA, 0 }
		, { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex3uv, u), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	device->CreateInputLayout(ied, 2, vsblob->GetBufferPointer(), vsblob->GetBufferSize(), &layout);

	vsblob->Release();
	psblob->Release();

	return true;
}

bool RendererDX11::display_buffer(const uint32 width, const uint32 height) {
	context->UpdateSubresource(texture, 0, NULL, *bits, width * 4, 0);

	uint stride = sizeof(Vertex3uv), offset = 0;
	context->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	context->IASetInputLayout(layout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	context->VSSetShader(vshader, NULL, 0);
	context->PSSetShader(pshader, NULL, 0);
	context->PSSetShaderResources(0, 1, &srv);
	context->PSSetSamplers(0, 1, &sampler);

	context->OMSetRenderTargets(1, &rtv, NULL);

	context->Draw(4, 0);

	HRESULT hr = swap_chain->Present(1, 0);

	return SUCCEEDED(hr);
}

void RendererDX11::set_display_mode(const uint32 width, const uint32 height) { }

void RendererDX11::unload_buffer() {
	if(device)	   { device    ->Release(); }
	if(context)	   { context   ->Release(); }
	if(swap_chain) { swap_chain->Release(); }
	if(rtv)		   { rtv       ->Release(); }

	if(texture)	   { texture   ->Release(); }
	if(srv)		   { srv       ->Release(); }
	if(sampler)	   { sampler   ->Release(); }

	if(vbuffer)	   { vbuffer   ->Release(); }
	if(layout)	   { layout    ->Release(); }
	if(vshader)	   { vshader   ->Release(); }
	if(pshader)	   { pshader   ->Release(); }
}