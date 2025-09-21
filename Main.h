#pragma once
#ifndef MAIN_H
#define MAIN_H

#include "Buffer.h"
#include "Time.h"
#include "Game.h"

#if defined(WINDOW_WIN32)
	#include "WindowWin32.h"
#elif defined(WINDOW_MAC)
	#include "WindowMac.h"
#elif defined(WINDOW_LINUX)
	#include "WindowLinux.h"
#endif

#if  defined(RENDERER_GDI)
	#include "RendererGDI.h"
#elif defined(RENDERER_OPENGL)
	#include "RendererOpenGL.h"
#elif defined(RENDERER_DX11)
	#include "RendererDX11.h"
#elif defined(RENDERER_DX12)
	#include "RendererDX12.h"
#elif defined(RENDERER_VULKAN)
	#include "RendererVulkan.h"
#elif defined(RENDERER_METAL)
	#include "RendererMetal.h"
#endif


void init();
void init_lookups();
void execute_frame();
void run();
void sizemove();
inline bool resize(const uint32 width, const uint32 height);
inline void draw();

#endif
