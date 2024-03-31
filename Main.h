#pragma once
#ifndef MAIN_H
#define MAIN_H

#include "Buffer.h"
#include "Win32.h"
#include "FPS.h"
#include "Game.h"

void init();
void init_lookups();
void run();
inline bool resize(int, int);
inline void draw();

Buffer buffer;

#endif
