#pragma once
#ifndef RENDERERDX11_H
#define RENDERERDX11_H

#define WIN32_LEAN_AND_MEAN

#include "Renderer.h"
#include "Vertex3uv.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <windows.h>

class RendererDX11 {
public:
	HWND window;
	void** bits;

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

	ID3D11Device*           device     = NULL;
	ID3D11DeviceContext*    context    = NULL;
	IDXGISwapChain*         swap_chain = NULL;
	ID3D11RenderTargetView* rtv        = NULL;

	ID3D11Texture2D*          texture = NULL;
	ID3D11ShaderResourceView* srv     = NULL;
	ID3D11SamplerState*       sampler = NULL;

	ID3D11Buffer*       vbuffer = NULL;
	ID3D11InputLayout*  layout  = NULL;
	ID3D11VertexShader* vshader = NULL;
	ID3D11PixelShader*  pshader = NULL;

	bool display_buffer(const uint32 width, const uint32 height);
	void set_display_mode(const uint32 width, const uint32 height);
	bool init_buffer(HWND window, void** bits, const uint32 width, const uint32 height);
	void unload_buffer();

	inline static IRenderer create_renderer(RendererDX11* gdi_renderer) {
		IRenderer renderer;
		renderer.display_buffer_callback   = RendererDX11::display_buffer_callback;
		renderer.set_display_mode_callback = RendererDX11::set_display_mode_callback;
		renderer.init_buffer_callback      = RendererDX11::init_buffer_callback;
		renderer.unload_buffer_callback    = RendererDX11::unload_buffer_callback;
		renderer.self = gdi_renderer;

		return renderer;
	}
private:
	inline static bool display_buffer_callback(const void* self, const uint32 width, const uint32 height) {
		return ((RendererDX11*)self)->display_buffer(width, height);
	}

	inline static void set_display_mode_callback(const void* self, const uint32 width, const uint32 height) {
		((RendererDX11*)self)->set_display_mode(width, height);
	}

	inline static bool init_buffer_callback(const void* self, HWND window, void** bits, const uint32 width, const uint32 height) {
		return ((RendererDX11*)self)->init_buffer(window, bits, width, height);
	}

	inline static void unload_buffer_callback(const void* self) {
		((RendererDX11*)self)->unload_buffer();
	}
};


#endif