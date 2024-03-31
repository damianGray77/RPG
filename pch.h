#pragma once
#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN

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

#include "Core.h"
#include <assert.h>

#endif