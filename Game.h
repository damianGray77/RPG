#pragma once
#ifndef GAME_H
#define GAME_H

#include "Buffer.h"
#include "Point2l.h"
#include "Point2ul.h"
#include "Point2b.h"
#include "Bitmap.h"

#define K_ESCAPE       0x01
#define K_1            0x02
#define K_2            0x03
#define K_3            0x04
#define K_4            0x05
#define K_5            0x06
#define K_6            0x07
#define K_7            0x08
#define K_8            0x09
#define K_9            0x0A
#define K_0            0x0B
#define K_MINUS        0x0C
#define K_EQUALS       0x0D
#define K_BACKSPACE    0x0E
#define K_TAB          0x0F
#define K_Q            0x10
#define K_W            0x11
#define K_E            0x12
#define K_R            0x13
#define K_T            0x14
#define K_Y            0x15
#define K_U            0x16
#define K_I            0x17
#define K_O            0x18
#define K_P            0x19
#define K_LEFTBRACKET  0x1A
#define K_RIGHTBRACKET 0x1B
#define K_ENTER        0x1C
#define K_CONTROL      0x1D
#define K_A            0x1E
#define K_S            0x1F
#define K_D            0x20
#define K_F            0x21
#define K_G            0x22
#define K_H            0x23
#define K_J            0x24
#define K_K            0x25
#define K_L            0x26
#define K_SEMICOLON    0x27
#define K_QUOTE        0x28
#define K_TILDE        0x29
#define K_LSHIFT       0x2A
#define K_BACKSLASH    0x2B
#define K_Z            0x2C
#define K_X            0x2D
#define K_C            0x2E
#define K_V            0x2F
#define K_B            0x30
#define K_N            0x31
#define K_M            0x32
#define K_COMMA        0x33
#define K_PERIOD       0x34
#define K_SLASH        0x35
#define K_RSHIFT       0x36
#define K_MULTIPLY     0x37
#define K_ALT          0x38
#define K_SPACE        0x39
#define K_CAPSLOCK     0x3A
#define K_F1           0x3B
#define K_F2           0x3C
#define K_F3           0x3D
#define K_F4           0x3E
#define K_F5           0x3F
#define K_F6           0x40
#define K_F7           0x41
#define K_F8           0x42
#define K_F9           0x43
#define K_F10          0x44
#define K_NUMLOCK      0x45
#define K_SCROLLLOCK   0x46
#define K_HOME         0x47
#define K_UP           0x48
#define K_PAGEUP       0x49
#define K_LEFT         0x4B
#define K_RIGHT        0x4D
#define K_PLUS         0x4E
#define K_END          0x4F
#define K_DOWN         0x50
#define K_PAGEDOWN     0x51
#define K_INSERT       0x52
#define K_DELETE       0x53
#define K_F11          0x57
#define K_F12          0x58
#define K_LWIN         0x7D
#define K_RWIN         0x7E
#define K_MENU         0x7F

#define MAXTI_X 20
#define MAXTI_Y 15
#define TILE_X 32
#define TILE_Y 32

enum bound {
	  TOP
	, LEFT
	, RIGHT
	, BOTTOM
};

enum tile_type {
	  TILETYPE_NONE
	, TILETYPE_BACK
	, TILETYPE_MASK
	, TILETYPE_ALPHA
};

struct DirtyTile {
	ulong    tile;
	Point2ul pos;
};

struct Bounds {
	Point2l min;
	Point2l max;
};

class Game {
public:
	Game();
	~Game() {}

	bool init();
	void load();
	void load_map();
	void load_tiles();
	int load_tile(const char *, const tile_type, const int, const int);

	void update(const float);
	bool render();
	void render_all();
	void render_dirty();

	bool resize(const int, const int);
	void update_input(const int);
	void update_bounds();

	void unload();

	void copy_to_buffer(const int, const int, const int);
	void copy_to_buffer_clip(const int, const int, const int, const int, const int, const int, const int);

	void copy_to_buffer_masked(const int, const int, const int, const int);
	void copy_to_buffer_clip_masked(const int, const int, const int, const int, const int, const int, const int, const int);

	void copy_to_buffer_masked(const int, const int, const int, const int, const int);
	void copy_to_buffer_clip_masked(const int, const int, const int, const int, const int, const int, const int, const int, const int);

	void copy_to_buffer_alpha(const int, const int, const int, const int);
	void copy_to_buffer_clip_alpha(const int, const int, const int, const int, const int, const int, const int, const int);

	bool is_pressed(const ubyte, const bool);

	void flag_dirt(const uint, const uint);


	bool is_tile_dirty(const int, const int);

	bool running;
	bool paused;
	bool refresh;

	int max_tiles_x;
	int max_tiles_y;

	int mid_tiles_x;
	int mid_tiles_y;

	Buffer *buffer;

	ulong *map;

	ulong *tiles;
	ulong **tindices;

	uint mapw;
	uint maph;

	ulong _w;
	ulong _h;

	Bounds  bounds = {};
	Point2l state  = {};
	Point2b interp = {};

	DirtyTile* dirty_tiles;
		
	ubyte *dirty;
	int dirty_len;

	bool *key_press;
};

#endif