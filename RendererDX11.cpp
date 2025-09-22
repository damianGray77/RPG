#include "pch.h"
#include "RendererDX11.h"

RendererDX11::RendererDX11() {
	window = NULL;
	bits   = NULL;

	buffer_width  = 0;
	buffer_height = 0;
	client_width  = 0;
	client_height = 0;

	device     = NULL;
	context    = NULL;
	swap_chain = NULL;
	rtv        = NULL;
	texture    = NULL;
	srv        = NULL;

	sampler_current = NULL;
	sampler_linear  = NULL;
	sampler_point   = NULL;

	vbuffer = NULL;
	layout  = NULL;
	vshader = NULL;
	pshader = NULL;
}

bool RendererDX11::init(HWND window, void** bits, const uint32 width, const uint32 height) {
	this->window = window;
	this->bits   = bits;

	buffer_width  = width;
	buffer_height = height;
	client_width  = width;
	client_height = height;

	HRESULT hr;

	DXGI_SWAP_CHAIN_DESC scd = {};
	scd.BufferCount       = 2;
	scd.BufferDesc.Width  = width;
	scd.BufferDesc.Height = height;
	scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	scd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow      = window;
	scd.SampleDesc.Count  = 1;
	scd.Windowed          = TRUE;
	scd.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	D3D_FEATURE_LEVEL fl;
    hr = D3D11CreateDeviceAndSwapChain(
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

	hr = init_viewport(width, height);
	if (FAILED(hr)) { return false; }

	D3D11_SAMPLER_DESC samp_desc = {};
	samp_desc.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	hr = device->CreateSamplerState(&samp_desc, &sampler_linear);
	if (FAILED(hr)) { return false; }

	samp_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = device->CreateSamplerState(&samp_desc, &sampler_point);
	if (FAILED(hr)) { return false; }

	D3D11_BUFFER_DESC bd = {};
	bd.Usage     = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(quad);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA init_data = { quad };
	hr = device->CreateBuffer(&bd, &init_data, &vbuffer);
	if (FAILED(hr)) { return false; }

	ID3DBlob* vsblob = NULL;
	ID3DBlob* psblob = NULL;
	hr = D3DCompile(vscode, strlen(vscode), NULL, NULL, NULL, "main", "vs_4_0", 0, 0, &vsblob, NULL);
	if (FAILED(hr)) { return false; }

	hr = D3DCompile(pscode, strlen(pscode), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &psblob, NULL);
	if (FAILED(hr)) { return false; }

	hr = device->CreateVertexShader(vsblob->GetBufferPointer(), vsblob->GetBufferSize(), NULL, &vshader);
	if (FAILED(hr)) { return false; }

	hr = device->CreatePixelShader (psblob->GetBufferPointer(), psblob->GetBufferSize(), NULL, &pshader);
	if (FAILED(hr)) { return false; }

	// Input layout
	D3D11_INPUT_ELEMENT_DESC ied[] = {
		  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex3uv, x), D3D11_INPUT_PER_VERTEX_DATA, 0 }
		, { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex3uv, u), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	hr = device->CreateInputLayout(ied, 2, vsblob->GetBufferPointer(), vsblob->GetBufferSize(), &layout);
	if (FAILED(hr)) { return false; }

	vsblob->Release();
	psblob->Release();

	return true;
}

bool RendererDX11::draw() {
	context->UpdateSubresource(texture, 0, NULL, *bits, buffer_width * 4, 0);

	context->OMSetRenderTargets(1, &rtv, NULL);

	context->Draw(4, 0);

	HRESULT hr = swap_chain->Present(1, 0);
	if (FAILED(hr)) { return false; }

	return true;
}

bool RendererDX11::resize(const uint32 width, const uint32 height) {
	if (0 == width || 0 == height) { return true; }

	client_width  = width;
	client_height = height;

	if (width < buffer_width || height < buffer_height) {
		sampler_current = sampler_linear;
	} else {
		sampler_current = sampler_point;
	}

	context->OMSetRenderTargets(0, NULL, NULL);
	if (rtv) { rtv->Release(); rtv = NULL; }

	HRESULT hr = swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr)) { return false; }

	hr = init_viewport(width, height);
	if (FAILED(hr)) { return false; }

	return true;
}

HRESULT RendererDX11::init_viewport(const uint32 width, const uint32 height) {
	if(texture) { texture->Release(); texture = NULL; }
	if(srv)	    { srv    ->Release(); srv     = NULL; }

	HRESULT hr;

	ID3D11Texture2D* back_buffer = NULL;
	hr = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer);
	if (FAILED(hr)) { return hr; }

	hr = device->CreateRenderTargetView(back_buffer, NULL, &rtv);
	if (FAILED(hr)) { return hr; }

	back_buffer->Release();

	D3D11_TEXTURE2D_DESC tex_desc = {};
	tex_desc.Width            = buffer_width;
	tex_desc.Height           = buffer_height;
	tex_desc.MipLevels        = 1;
	tex_desc.ArraySize        = 1;
	tex_desc.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.Usage            = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
	tex_desc.CPUAccessFlags   = 0;
	tex_desc.MiscFlags        = 0;
	hr = device->CreateTexture2D(&tex_desc, NULL, &texture);
	if (FAILED(hr)) { return hr; }

	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format                    = tex_desc.Format;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels       = 1;
	hr = device->CreateShaderResourceView(texture, &srv_desc, &srv);
	if (FAILED(hr)) { return hr; }

	context->OMSetRenderTargets(1, &rtv, NULL);

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = (float)width;
	viewport.Height   = (float)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	context->RSSetViewports(1, &viewport);

	uint stride = sizeof(Vertex3uv), offset = 0;
	context->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	context->IASetInputLayout(layout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	context->VSSetShader(vshader, NULL, 0);
	context->PSSetShader(pshader, NULL, 0);
	context->PSSetShaderResources(0, 1, &srv);
	context->PSSetSamplers(0, 1, &sampler_current);

	return S_OK;
}

void RendererDX11::unload() {
	if(device)         { device        ->Release(); device         = NULL; }
	if(context)        { context       ->Release(); context        = NULL; }
	if(swap_chain)     { swap_chain    ->Release(); swap_chain     = NULL; }
	if(rtv)            { rtv           ->Release(); rtv            = NULL; }

	if(texture)        { texture       ->Release(); texture        = NULL; }
	if(srv)            { srv           ->Release(); srv            = NULL; }
	if(sampler_point)  { sampler_point ->Release(); sampler_point  = NULL; }
	if(sampler_linear) { sampler_linear->Release(); sampler_linear = NULL; }
	sampler_current = NULL;

	if(vbuffer)        { vbuffer       ->Release(); vbuffer        = NULL; }
	if(layout)         { layout        ->Release(); layout         = NULL; }
	if(vshader)        { vshader       ->Release(); vshader        = NULL; }
	if(pshader)        { pshader       ->Release(); pshader        = NULL; }
}