#pragma once
#ifndef MAIN_H
#define MAIN_H

#include "Buffer.h"
#include "Win32.h"
#include "Time.h"
#include "Game.h"

void init();
void init_lookups();
void execute_frame();
void run();
void move();
inline bool resize(int, int);
inline void draw();

#endif
