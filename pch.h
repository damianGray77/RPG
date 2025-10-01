#pragma once
#ifndef PCH_H
#define PCH_H

#define APP_NAME L"RPG"
#define DEBUG_OUT
#define RENDERER_GDI
//#define RENDERER_DX11
//#define RENDERER_DX12
//#define RENDERER_OPENGL
//#define RENDERER_VULKAN
//#define RENDERER_METAL
#define WINDOW_WIN32
//#define WINDOW_MAC
//#define WINDOW_LINUX

#if defined(RENDERER_GDI)
	#define RENDERER RendererGDI
#elif defined(RENDERER_DX11)
	#define RENDERER RendererDX11
#elif defined(RENDERER_DX12)
	#define RENDERER RendererDX12
#elif defined(RENDERER_OPENGL)
	#define RENDERER RendererOpenGL
#elif defined(RENDERER_VULKAN)
	#define RENDERER RendererVulkan
#elif defined(RENDERER_METAL)
	#define RENDERER RendererMetal
#endif

#if defined(WINDOW_WIN32)
	#define WINDOW WindowWin32
#elif defined(WINDOW_MAC)
	#define WINDOW WindowMac
#elif defined(WINDOW_LINUX)
	#define WINDOW WindowLinux
#endif

enum ClipSide {
	  top
	, left
	, bottom
	, right
};

#include "targetver.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <emmintrin.h>

#include "Core.h"
#include <assert.h>

#endif