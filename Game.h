#pragma once
#ifndef GAME_H
#define GAME_H

#include "Buffer.h"
#include "Point2.h"
#include "Bitmap.h"
#include "Time.h"

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
#define SPRITE_X_OFFSET 0
#define SPRITE_Y_OFFSET 48
#define TILE_X 32
#define TILE_Y 32
#define TILE_XY 1024 // TILE_X * TILE_Y

enum bound {
	  TOP
	, LEFT
	, RIGHT
	, BOTTOM
};

enum tile_type : uint32 {
	  TILETYPE_NONE
	, TILETYPE_BACK
	, TILETYPE_MASK
	, TILETYPE_ALPHA
};

struct TileAnim {
	uint8  tile_index;
	uint32 anim_handle;
};

struct Tile {
	uint32 tile;
	Point2<uint16> pos;
};

class Game {
public:
	Game();
	~Game() {}

	bool init();
	void load();
	void load_map();
	void load_tiles();
	void load_chars();
	void load_anims();

	uint32 load_tile(uint32* tbuf, uint32** tibuf, const char *file, const tile_type type, const uint16 i, const uint32 start);

	void update(const float);
	const bool render();
	const bool render_map();
	const bool render_chars();
	const void render_map_all();
	const void render_map_all2();
	const void render_map_dirty();

	bool resize(const int, const int);
	void update_input(const float unit);
	void update_bounds();
	void update_anims();

	void unload();

	void copy_to_buffer(const uint32 index, const uint32 x, const uint32 y);
	void copy_to_buffer_clip(const uint32 index, const int32 x, const int32 y, const uint32 t, const uint32 l, const uint32 b, const uint32 r);

	void copy_to_buffer_masked(const uint32 tindex, const uint32 oindex, const uint32 x, const uint32 y);
	void copy_to_buffer_clip_masked(const uint32 tindex, const uint32 oindex, const int32 x, const int32 y, const uint32 t, const uint32 l, const uint32 b, const uint32 r);

	void copy_to_buffer_masked(uint32 **ibuf, const uint32 index, const uint32 x, const uint32 y);
	void copy_to_buffer_clip_masked(uint32 **ibuf, const uint32 index, const int32 x, const int32 y, const uint32 t, const uint32 l, const uint32 b, const uint32 r);

	//void copy_to_buffer_alpha(const int, const int, const int, const int);
	//void copy_to_buffer_clip_alpha(const int, const int, const int, const int, const int, const int, const int, const int);

	bool is_pressed(const uint8 scancode, const bool turnoff);
	float update_speed(const bool positive, const bool negative, float current_rate);

	void flag_dirt(const uint16 x, const uint16 y);

	Time time;

	bool running;
	bool paused;
	bool refresh;

	uint8 max_tiles_x;
	uint8 max_tiles_y;

	uint16 mid_tiles_x;
	uint16 mid_tiles_y;

	uint16 max_px_y;
	uint16 max_px_x;
	
	Buffer *buffer;

	uint32 *map;

	uint32 *chars;
	uint32 *tiles;
	uint32 **cindices;
	uint32 **tindices;
	TileAnim tanims[256] = {};

	uint8 char_count;
	uint8 tile_count;
	uint8 anim_count;

	uint16 mapw;
	uint16 maph;

	Point2<float> move_rate;
	#define ACCELRATE 0.1f
	#define DECELRATE 0.3f
	#define MAXSPEED  2.0f

	uint16 _w;
	uint16 _h;
	uint32 stride;

	Bounds<int16>  bounds = {};
	Point2<uint16> state  = {};
	Point2<int8>   interp = {};

	Tile  *dirty_tiles;
	Tile *active_tiles;
	uint16 dirty_len;

	uint16 *dirty_frame_id;
	uint16 current_frame_id;

	bool *key_press;
};

#endif