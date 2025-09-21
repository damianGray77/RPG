#pragma once
#ifndef RENDERERDX11_H
#define RENDERERDX11_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "Vertex3uv.h"

class RendererDX11 {
public:
	bool draw();
	bool resize(const uint32 width, const uint32 height);
	bool init(HWND window, void** bits, const uint32 width, const uint32 height);
	void unload();

	RendererDX11();
private:
	HWND window;
	void** bits;

	uint32 buffer_width;
	uint32 buffer_height;
	uint32 client_width;
	uint32 client_height;

	Vertex3uv quad[4] = {
		  { -1.0f,  1.0f, 0.0f, 0.0f, 0.0f }
		, {  1.0f,  1.0f, 0.0f, 1.0f, 0.0f }
		, { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f }
		, {  1.0f, -1.0f, 0.0f, 1.0f, 1.0f }
	};

	const char* vscode = R"(
		struct VS_IN {
			float3 pos : POSITION;
			float2 uv  : TEXCOORD;
		};
		struct PS_IN {
			float4 pos : SV_POSITION;
			float2 uv  : TEXCOORD;
		};
		PS_IN main(VS_IN input) {
			PS_IN output;
			output.pos = float4(input.pos, 1.0);
			output.uv  = input.uv;
			return output;
		}
	)";

	const char* pscode = R"(
		Texture2D tex0 : register(t0);
		SamplerState samp0 : register(s0);
		float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD) : SV_TARGET {
			return tex0.Sample(samp0, uv);
		}
	)";

	ID3D11Device* device = NULL;
	ID3D11DeviceContext* context = NULL;
	IDXGISwapChain* swap_chain = NULL;
	ID3D11RenderTargetView* rtv = NULL;

	ID3D11Texture2D* texture = NULL;
	ID3D11ShaderResourceView* srv = NULL;

	ID3D11SamplerState* sampler_current = NULL;
	ID3D11SamplerState* sampler_linear = NULL;
	ID3D11SamplerState* sampler_point = NULL;

	ID3D11Buffer* vbuffer = NULL;
	ID3D11InputLayout* layout = NULL;
	ID3D11VertexShader* vshader = NULL;
	ID3D11PixelShader* pshader = NULL;

	HRESULT init_viewport(const uint32 width, const uint32 height);
};


#endif