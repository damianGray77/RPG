#include "pch.h"
#include "Game.h"

#define TILE_POS_TYPE   29
#define TILE_POS_FRAMES 26
#define TILE_POS_INDEX  23
#define TILE_POS_SPEED  14
#define TILE_POS_WIDTH   7
#define TILE_POS_HEIGHT  0

#define TILE_MASK_TYPE   0xe0000000
#define TILE_MASK_FRAMES 0x1c000000
#define TILE_MASK_INDEX  0x03800000
#define TILE_MASK_SPEED  0x007fc000
#define TILE_MASK_WIDTH  0x00003f80
#define TILE_MASK_HEIGHT 0x0000007f

#define MAP_POS_WALL 31
#define MAP_POS_BACK 16
#define MAP_POS_FORE  0

#define MAP_MASK_WALL 0x80000000
#define MAP_MASK_BACK 0x7fff0000
#define MAP_MASK_FORE 0x0000ffff

/*
Tile Format

3b         3b         3b         9b         7b         7b
Type       # Sprites  Cur Sprite Speed (ms) Width      Height
29         26         23         14         7          0

0xe0000000 0x1c000000 0x03800000 0x007fc000 0x00003f80 0x0000007f
*/

Game::Game() {
	_h = 0;
	_w = 0;

	max_tiles_x = 0;
	max_tiles_y = 0;
	mid_tiles_x = 0;
	mid_tiles_y = 0;

	dirty_len  = 0;
	anim_count = 0;
	tile_count = 0;
	char_count = 0;

	maph = 0;
	mapw = 0;
	max_px_x = 0;
	max_px_y = 0;

	running = false;
	paused  = false;
	refresh = false;

	map        = NULL;
	tiles      = NULL;
	chars      = NULL;
	tindices   = NULL;
	cindices   = NULL;
	anim_cells = NULL;

	buffer    = NULL;
	key_press = NULL;

	dirty_frame_id = NULL;
	dirty_tiles    = NULL;
	current_frame_id = 1;
}

bool Game::init() {
	running = false;
	paused  = false;

	map      = new uint32[65535];
	chars    = new uint32[65535];
	tiles    = new uint32[65535];
	cindices = new uint32*[256];
	tindices = new uint32*[256];

	camera = { 0, 0 };

	move_rate = {};

	time.init();

	load();

	return true;
}

void Game::load() {
	load_map();
	load_tiles();
	load_chars();
	load_entites();
	load_anims();
}

void Game::load_map() {
	mapw = 30;
	maph = 20;
	int mapwh = mapw * maph;
	int i = mapwh;
	int x, y;
	while (--i >= 0) {
		x = i % mapw;
		y = i / mapw;
		bool wall = false;
		uint back = 2;
		uint fore = 0 == rand() % 17 ? 3 : 0 == rand() % 17 ? 4 : 0;

		map[i] =
			  (ulong)(wall << MAP_POS_WALL)
			+ (ulong)(back << MAP_POS_BACK)
			+ (ulong)(fore << MAP_POS_FORE)
		;
	}

	dirty_len = 0;
	dirty_tiles    = new Tile  [mapwh];
	dirty_frame_id = new uint16[mapwh];

	refresh = true;
}

void Game::load_tiles() {
	tindices[0] = tiles;
	tiles[0] =
		  (TILETYPE_NONE << TILE_POS_TYPE  )
		+ ( 1ul          << TILE_POS_FRAMES)
		+ ( 0ul          << TILE_POS_INDEX )
		+ ( 0ul          << TILE_POS_SPEED )
		+ (32ul          << TILE_POS_WIDTH )
		+ (32ul          << TILE_POS_HEIGHT)
	;
	memset(tindices[0] + 1, 0, sizeof(uint32) * 1024); // first tile is a blank black square

	uint32 index = 1025;
	
	++tile_count;

	const char* files[5] = {
		  ""
		, "E:\\Projects\\RPG\\Resources\\tile0001.bmp"
		, "E:\\Projects\\RPG\\Resources\\atile0001.bmp"
		, "E:\\Projects\\RPG\\Resources\\over0001.bmp"
		, "E:\\Projects\\RPG\\Resources\\over0002.bmp"
	};

	uint8 i;

	for (i = tile_count; i < 3; ++i) {
		index = load_tile(tiles, tindices, files[i], TILETYPE_BACK, i, index) + 1;
		++tile_count;
	}

	for (i = tile_count; i < 5; ++i) {
		index = load_tile(tiles, tindices, files[i], TILETYPE_MASK, i, index) + 1;
		++tile_count;
	}
}

void Game::load_entites() {
	entities.init();

	       player_id = 0;
	uint8  player_size;
	uint32 player_position;
	uint8  player_velocity;
	uint32 player_sprite;
	
	entity_set_size(player_size, 1, 2);
	entity_set_position(player_position, 0, 0, 7, 12, 16, 0); // pos_y = feet tile
	entity_set_velocity(player_velocity, 0, 0);
	entity_set_sprite(player_sprite, 0, 0, 0, 0);
	entities.add(player_id, player_size, player_position, player_velocity, player_sprite);
	entities.activate(player_id);
}

void Game::load_chars() {
	const char* files[1] = { "E:\\Projects\\RPG\\Resources\\char0001.bmp" };

	uint32 index = 0;
	for (uint8 i = 0; i < 1; ++i) {
		index = load_tile(chars, cindices, files[i], TILETYPE_MASK, i, index) + 1;
		++char_count;
	}
}

void Game::load_anims() {
	for (uint8 i = 0; i < tile_count; ++i) {
		uint32 info = *tindices[i];
		const uint8 sprites = (uint8)((info & TILE_MASK_FRAMES) >> TILE_POS_FRAMES);
		if (1 == sprites) { continue; }

		const float ms = (float)((info & TILE_MASK_SPEED) >> TILE_POS_SPEED);

		const uint8 interval_handle = time.set_interval(ms);

		tanims[anim_count++] = { i, interval_handle };
	}

	rebuild_anim_cells();
}

void Game::rebuild_anim_cells() {
	if (anim_cells) { free(anim_cells); anim_cells = NULL; }
	if (0 == anim_count) { return; }

	memset(anim_cell_count, 0, sizeof(anim_cell_count));
	memset(anim_cell_start, 0, sizeof(anim_cell_start));

	const uint32 map_size = mapw * maph;

	// pass 1: count cells per animated tile type
	for (uint32 c = 0; c < map_size; ++c) {
		const uint32 mapval = map[c];
		const uint16 back = (uint16)((mapval & MAP_MASK_BACK) >> MAP_POS_BACK);
		const uint16 fore = (uint16)((mapval & MAP_MASK_FORE) >> MAP_POS_FORE);

		for (uint8 a = 0; a < anim_count; ++a) {
			const uint8 ti = tanims[a].tile_index;
			if (back == ti || fore == ti) {
				++anim_cell_count[ti];
			}
		}
	}

	// prefix sum for start offsets
	uint16 offset = 0;
	for (uint8 a = 0; a < anim_count; ++a) {
		const uint8 ti = tanims[a].tile_index;
		anim_cell_start[ti] = offset;
		offset += anim_cell_count[ti];
		anim_cell_count[ti] = 0;
	}

	if (0 == offset) { return; }

	anim_cells = (uint16*)malloc(offset * sizeof(uint16));

	// pass 2: fill cell indices
	for (uint32 c = 0; c < map_size; ++c) {
		const uint32 mapval = map[c];
		const uint16 back = (uint16)((mapval & MAP_MASK_BACK) >> MAP_POS_BACK);
		const uint16 fore = (uint16)((mapval & MAP_MASK_FORE) >> MAP_POS_FORE);

		for (uint8 a = 0; a < anim_count; ++a) {
			const uint8 ti = tanims[a].tile_index;
			if (back == ti || fore == ti) {
				anim_cells[anim_cell_start[ti] + anim_cell_count[ti]] = (uint16)c;
				++anim_cell_count[ti];
			}
		}
	}
}

uint32 Game::load_tile(uint32 *tbuf, uint32 **tibuf, const char *file, const tile_type type, const uint16 i, const uint32 start) {
	Bitmap tile(file);

	const uint8 w = tile.w;
	const uint8 h = tile.h;

	const uint16 len = w * h;

	const uint8  sprites = (w * h) / TILE_XY;
	const uint16 speed   = 250;
	const uint32 end     = start + len + 1;

	tibuf[i] = tbuf + start;
	tbuf[start] =
		  ((uint32)type    << TILE_POS_TYPE  )
		+ ((uint32)sprites << TILE_POS_FRAMES)
		+ (0ul             << TILE_POS_INDEX )
		+ ((uint32)speed   << TILE_POS_SPEED )
		+ ((uint32)w       << TILE_POS_WIDTH )
		+ ((uint32)h       << TILE_POS_HEIGHT)
	;
	memcpy(tibuf[i] + 1, tile.data, sizeof(uint32) * len);

	return end;
}

/// <summary>
/// Copy a background tile to a buffer
/// </summary>
void Game::copy_to_buffer(const uint32 index, const uint32 x, const uint32 y) {
	const uint32 * tile = tindices[index];

	      uint32 * __restrict buffer_ptr = buffer->bits + (stride * y + x);
	const uint32 * __restrict   tile_ptr = tile + 1 + (TILE_XY * tile_frames[index]);
	const size_t row_bytes = sizeof(*buffer_ptr) * TILE_X;

	for (uint32 row = 0; row < TILE_Y; ++row) {
		memcpy(buffer_ptr, tile_ptr, row_bytes);

		buffer_ptr += stride;
		  tile_ptr += TILE_X;
	}
}

/// <summary>
/// Copy a background tile to a buffer
/// </summary>
void Game::copy_to_buffer_clip(const uint32 index, const int32 x, const int32 y, const uint32 clip_top, const uint32 clip_left, const uint32 clip_bottom, const uint32 clip_right) {
	const uint32 * tile = tindices[index];

	const uint32 clipped_width  = TILE_X - clip_right  - clip_left;
	const uint32 clipped_height = TILE_Y - clip_bottom - clip_top;

	      uint32 * __restrict buffer_ptr = buffer->bits + stride * (y + clip_top) + x + clip_left;
	const uint32 * __restrict   tile_ptr = tile + 1 + TILE_XY * tile_frames[index] + TILE_X * clip_top + clip_left;
	const size_t row_bytes = sizeof(*buffer_ptr) * clipped_width;

	for (uint32 row = 0; row < clipped_height; ++row) {
		memcpy(buffer_ptr, tile_ptr, row_bytes);

		buffer_ptr += stride;
		  tile_ptr += TILE_X;
	}
}

/// <summary>
/// Copy an overlay mask tile with a background tile to a buffer
/// </summary>
void Game::copy_to_buffer_masked(const uint32 background_index, const uint32 overlay_index, const uint32 x, const uint32 y) {
	const uint32 * background_tile = tindices[background_index];
	const uint32 *    overlay_tile = tindices[   overlay_index];

	      uint32 * __restrict     buffer_ptr = buffer->bits + stride * y + x;
	const uint32 * __restrict background_ptr = background_tile + 1 + (TILE_XY * tile_frames[background_index]);
	const uint32 * __restrict    overlay_ptr =    overlay_tile + 1 + (TILE_XY * tile_frames[   overlay_index]);

	const __m128i zero     = _mm_setzero_si128();
	const __m128i rgb_mask = _mm_set1_epi32(0x00ffffff);

	for (uint32 row = 0; row < TILE_Y; ++row) {
		for (uint32 col = 0; col < TILE_X; col += 4) {
			const __m128i fore = _mm_loadu_si128((const __m128i*)(overlay_ptr    + col));
			const __m128i back = _mm_loadu_si128((const __m128i*)(background_ptr + col));
			const __m128i mask = _mm_cmpeq_epi32(_mm_and_si128(fore, rgb_mask), zero);

			_mm_storeu_si128((__m128i*)(buffer_ptr + col),
				_mm_or_si128(_mm_and_si128(back, mask), _mm_andnot_si128(mask, fore))
			);
		}

		    buffer_ptr += stride;
		   overlay_ptr += TILE_X;
		background_ptr += TILE_X;
	}
}

/// <summary>
/// Copy an overlay mask tile with a background tile to a buffer
/// </summary>
void Game::copy_to_buffer_clip_masked(const uint32 tindex, const uint32 oindex, const int32 x, const int32 y, const uint32 clip_top, const uint32 clip_left, const uint32 clip_bottom, const uint32 clip_right) {
	const uint32 * background_tile = tindices[tindex];
	const uint32 *    overlay_tile = tindices[oindex];

	const uint32 clip_offset = TILE_X * clip_top + clip_left;

	const uint32 clipped_width  = TILE_X - clip_right  - clip_left;
	const uint32 clipped_height = TILE_Y - clip_bottom - clip_top;

	      uint32 * __restrict     buffer_ptr = buffer->bits + stride * (y + clip_top) + x + clip_left;
	const uint32 * __restrict background_ptr = background_tile + 1 + TILE_XY * tile_frames[tindex] + clip_offset;
	const uint32 * __restrict    overlay_ptr =    overlay_tile + 1 + TILE_XY * tile_frames[oindex] + clip_offset;

	const __m128i zero     = _mm_setzero_si128();
	const __m128i rgb_mask = _mm_set1_epi32(0x00ffffff);

	const uint32 simd_width = clipped_width & ~3u;

	for (uint32 row = 0; row < clipped_height; ++row) {
		uint32 col = 0;
		for (; col < simd_width; col += 4) {
			const __m128i fore = _mm_loadu_si128((const __m128i*)(overlay_ptr    + col));
			const __m128i back = _mm_loadu_si128((const __m128i*)(background_ptr + col));
			const __m128i mask = _mm_cmpeq_epi32(_mm_and_si128(fore, rgb_mask), zero);

			_mm_storeu_si128((__m128i*)(buffer_ptr + col),
				_mm_or_si128(_mm_and_si128(back, mask), _mm_andnot_si128(mask, fore))
			);
		}
		for (; col < clipped_width; ++col) {
			const uint32 fore =    overlay_ptr[col];
			const uint32 back = background_ptr[col];
			const uint32 mask = -!(fore & 0x00ffffff);
			buffer_ptr[col] = (back & mask) | (fore & ~mask);
		}

		    buffer_ptr += stride;
		   overlay_ptr += TILE_X;
		background_ptr += TILE_X;
	}
}

/// <summary>
/// Copy an overlay mask tile with an arbitrary width and height to a buffer
/// </summary>
void Game::copy_to_buffer_masked(uint32 **ibuf, const uint32 index, const uint32 x, const uint32 y) {
	uint32 * sprite = ibuf[index];

	const uint32 sprite_width  = (*sprite & TILE_MASK_WIDTH)  >> TILE_POS_WIDTH;
	const uint32 sprite_height = (*sprite & TILE_MASK_HEIGHT) >> TILE_POS_HEIGHT;

	      uint32 * __restrict buffer_ptr = buffer->bits + stride * y + x;
	const uint32 * __restrict sprite_ptr = sprite + 1;

	const __m128i zero     = _mm_setzero_si128();
	const __m128i rgb_mask = _mm_set1_epi32(0x00ffffff);

	const uint32 simd_width = sprite_width & ~3u;

	for (uint32 row = 0; row < sprite_height; ++row) {
		uint32 col = 0;
		for (; col < simd_width; col += 4) {
			const __m128i fore = _mm_loadu_si128((const __m128i*)(sprite_ptr + col));
			const __m128i back = _mm_loadu_si128((const __m128i*)(buffer_ptr + col));
			const __m128i mask = _mm_cmpeq_epi32(_mm_and_si128(fore, rgb_mask), zero);

			_mm_storeu_si128((__m128i*)(buffer_ptr + col),
				_mm_or_si128(_mm_and_si128(back, mask), _mm_andnot_si128(mask, fore))
			);
		}
		for (; col < sprite_width; ++col) {
			const uint32 fore = sprite_ptr[col];
			const uint32 back = buffer_ptr[col];
			const uint32 mask = -!(fore & 0x00ffffff);
			buffer_ptr[col] = (back & mask) | (fore & ~mask);
		}

		buffer_ptr += stride;
		sprite_ptr += sprite_width;
	}
}

/// <summary>
/// Copy an overlay mask tile with an arbitrary width and height to a buffer
/// </summary>
void Game::copy_to_buffer_clip_masked(uint32** ibuf, const uint32 index, const int32 x, const int32 y, const uint32 clip_top, const uint32 clip_left, const uint32 clip_bottom, const uint32 clip_right) {
	uint32 * sprite = ibuf[index];

	const uint32 sprite_width  = (*sprite & TILE_MASK_WIDTH)  >> TILE_POS_WIDTH;
	const uint32 sprite_height = (*sprite & TILE_MASK_HEIGHT) >> TILE_POS_HEIGHT;

	const uint32 clipped_width  = sprite_width  - clip_right  - clip_left;
	const uint32 clipped_height = sprite_height - clip_bottom - clip_top;

	uint32 * __restrict buffer_ptr = buffer->bits + stride * (y + clip_top) + x + clip_left;
	uint32 * __restrict sprite_ptr = sprite + sprite_width * clip_top + clip_left + 1;

	const __m128i zero     = _mm_setzero_si128();
	const __m128i rgb_mask = _mm_set1_epi32(0x00ffffff);

	const uint32 simd_width = clipped_width & ~3u;

	for (uint32 row = 0; row < clipped_height; ++row) {
		uint32 col = 0;
		for (; col < simd_width; col += 4) {
			const __m128i fore = _mm_loadu_si128((const __m128i*)(sprite_ptr + col));
			const __m128i back = _mm_loadu_si128((const __m128i*)(buffer_ptr + col));
			const __m128i mask = _mm_cmpeq_epi32(_mm_and_si128(fore, rgb_mask), zero);

			_mm_storeu_si128((__m128i*)(buffer_ptr + col),
				_mm_or_si128(_mm_and_si128(back, mask), _mm_andnot_si128(mask, fore))
			);
		}
		for (; col < clipped_width; ++col) {
			const uint32 fore = sprite_ptr[col];
			const uint32 back = buffer_ptr[col];
			const uint32 mask = -!(fore & 0x00ffffff);
			buffer_ptr[col] = (back & mask) | (fore & ~mask);
		}

		buffer_ptr += stride;
		sprite_ptr += sprite_width;
	}
}

void Game::update(const float delta) {
	time.update();

	const float unit = delta * 100.0f;

	update_input(unit);
	update_anims();
	update_camera(delta);
	update_dirty();
}

bool Game::is_pressed(const uint8 scancode, const bool turnoff) {
	if (!key_press[scancode]) { return false; }

	if (turnoff) { key_press[scancode] = false; }

	return true;
}

inline void Game::update_speed(int8& rate, const bool key_pos, const bool key_neg) {
	if (key_neg && key_pos) { return; }
	
	if (key_neg) {
		rate = max(rate - (rate > 0 ? DECELRATE : ACCELRATE), -MAXSPEED);
	} else if (key_pos) {
		rate = min(rate + (rate < 0 ? DECELRATE : ACCELRATE),  MAXSPEED);
	} else if (0 != rate) {
		rate = rate > 0
			? max(rate - DECELRATE, 0)
			: min(rate + DECELRATE, 0)
		;
	}
}

void Game::update_input(const float unit) {
	if(!paused) {
		uint8 chunk_x, chunk_y;
		uint8   pos_x,   pos_y;
		uint8   sub_x,   sub_y;
		int8    vel_x,   vel_y;
		uint16 player_index = entities.copy_position_and_velocity(player_id, chunk_x, chunk_y, pos_x, pos_y, sub_x, sub_y, vel_x, vel_y);

		const bool up    = is_pressed(K_UP,    false);
		const bool down  = is_pressed(K_DOWN,  false);
		const bool left  = is_pressed(K_LEFT,  false);
		const bool right = is_pressed(K_RIGHT, false);

		if(0 != vel_x || 0 != vel_y || up || down || left || right) {
			update_speed(vel_y, down,  up);
			update_speed(vel_x, right, left);

			const float scaled_unit = unit / (float)MAXSPEED * 1.5f;
			float next_sub_x = sub_x + scaled_unit * vel_x;
			float next_sub_y = sub_y + scaled_unit * vel_y;

			// pos_y = feet tile, sprite is 2 tiles tall (head at pos_y - 1)
			const bool hit_bounds_top    =        0 == pos_y && next_sub_y < 0;
			const bool hit_bounds_bottom = maph - 2 == pos_y && next_sub_y >= (float)(TILE_Y - 1);
			const bool hit_bounds_left   =        0 == pos_x && next_sub_x < 0;
			const bool hit_bounds_right  = mapw - 2 == pos_x && next_sub_x >= (float)(TILE_X - 1);

			// quantize to integer subpos
			if (vel_x > 0) { next_sub_x = (float)ceil <int8, float>(next_sub_x); }
			else           { next_sub_x = (float)floor<int8, float>(next_sub_x); }
			if (vel_y > 0) { next_sub_y = (float)ceil <int8, float>(next_sub_y); }
			else           { next_sub_y = (float)floor<int8, float>(next_sub_y); }

			if (hit_bounds_top  || hit_bounds_bottom) { next_sub_y = sub_y; vel_y = 0; }
			if (hit_bounds_left || hit_bounds_right)  { next_sub_x = sub_x; vel_x = 0; }

			if(sub_y != next_sub_y) {
			         if (next_sub_y >= TILE_Y) { ++pos_y; sub_y = next_sub_y - TILE_Y; }
				else if (next_sub_y < 0)       { --pos_y; sub_y = next_sub_y + TILE_Y; }
				else                           {          sub_y = next_sub_y;          }
			}

			if(sub_x != next_sub_x) {
			         if (next_sub_x >= TILE_X) { ++pos_x; sub_x = next_sub_x - TILE_X; }
				else if (next_sub_x < 0)       { --pos_x; sub_x = next_sub_x + TILE_X; }
				else                           {          sub_x = next_sub_x;          }
			}


			entity_set_position(entities.positions [player_index], chunk_x, chunk_y, pos_x, pos_y, sub_x, sub_y);
			entity_set_velocity(entities.velocities[player_index], vel_x, vel_y);
		}
	}

	running = !(
		   is_pressed(K_ESCAPE, true)
		|| is_pressed(K_Q,      true)
	);

	if (is_pressed(K_SPACE, true)) {
		paused = !paused;
	}

	if (is_pressed(K_C, true)) {
		if (!camera.locked_to_entity) {
			camera.locked_to_entity = true;
			camera.locked_to_entity_id = player_id;

			// compute current and target pixel positions for lerp
			// pixel pos = position * TILE + subpos
			const float cur_px = (float)camera.position_x * TILE_X + (float)camera.subpos_x;
			const float cur_py = (float)camera.position_y * TILE_Y + (float)camera.subpos_y;

			Camera target = {};
			entities.lock_camera(player_id, target);
			const float tgt_px = (float)((int32)target.position_x - mid_tiles_x) * TILE_X + (float)target.subpos_x;
			const float tgt_py = (float)((int32)target.position_y - mid_tiles_y) * TILE_Y + (float)target.subpos_y;

			camera.tween_x.begin((float)cur_px, (float)tgt_px, 1.0f, EASE_OUT);
			camera.tween_y.begin((float)cur_py, (float)tgt_py, 1.0f, EASE_OUT);
		} else {
			camera.locked_to_entity = false;
		}
	}
}

void Game::update_camera(const float delta) {
	if (!camera.locked_to_entity) { return; }

	// compute target in pixel space: pixel pos = position * TILE + subpos
	Camera target = {};
	entities.lock_camera(camera.locked_to_entity_id, target);
	const float target_px = (float)((int32)target.position_x - mid_tiles_x) * TILE_X + (float)target.subpos_x;
	const float target_py = (float)((int32)target.position_y - mid_tiles_y) * TILE_Y + (float)target.subpos_y;

	if (camera.tween_x.active || camera.tween_y.active) {
		camera.tween_x.end = target_px;
		camera.tween_y.end = target_py;

		camera.tween_x.update(delta);
		camera.tween_y.update(delta);

		const float px = camera.tween_x.value();
		const float py = camera.tween_y.value();

		// decompose: position = floor(px / TILE), subpos = remainder
		int32 tile_x = (int32)floorf(px / TILE_X);
		int32 tile_y = (int32)floorf(py / TILE_Y);
		int32 sub_x  = (int32)(px - tile_x * TILE_X);
		int32 sub_y  = (int32)(py - tile_y * TILE_Y);

		camera.position_x = (int16)tile_x;
		camera.position_y = (int16)tile_y;
		camera.subpos_x   = (uint8)sub_x;
		camera.subpos_y   = (uint8)sub_y;
	} else {
		// tween complete or not active — snap to entity
		camera.position_x = (int16)target.position_x - mid_tiles_x;
		camera.position_y = (int16)target.position_y - mid_tiles_y;
		camera.subpos_x   = target.subpos_x;
		camera.subpos_y   = target.subpos_y;
	}

	update_bounds();
}

void Game::update_bounds() {
	const int16 minx = camera.position_x;
	const int16 miny = camera.position_y;
	// with natural subpos convention, posx IS the leading edge tile
	// trailing edge is posx + max_tiles_x when subpos > 0, or posx + max_tiles_x - 1 when aligned
	const int16 maxx = minx + max_tiles_x - 1 + (0 != camera.subpos_x);
	const int16 maxy = miny + max_tiles_y - 1 + (0 != camera.subpos_y);

	bounds.min.x = minx;
	bounds.min.y = miny;
	bounds.max.x = maxx;
	bounds.max.y = maxy;
}

void Game::update_anims() {
	for (uint32 i = 0; i < anim_count; ++i) {
		const TileAnim & ta    = tanims[i];
		      Interval & inter = time.intervals[ta.interval_handle];

		if (!inter.update()) { continue; }

		// advance frame in separate state array (tile header stays immutable)
		const uint32 info   = *tindices[ta.tile_index];
		const uint8  frames = (uint8)((info & TILE_MASK_FRAMES) >> TILE_POS_FRAMES);
		uint8 frame = tile_frames[ta.tile_index];
		if (++frame >= frames) { frame = 0; }
		tile_frames[ta.tile_index] = frame;

		// dirty only the precomputed cells that use this tile type
		const uint16 start = anim_cell_start[ta.tile_index];
		const uint16 count = anim_cell_count[ta.tile_index];

		for (uint16 c = 0; c < count; ++c) {
			const uint16 cell = anim_cells[start + c];
			flag_dirt(cell % mapw, cell / mapw);
		}
	}
}

void Game::update_dirty() {
	if (refresh) { return; } // full refresh handles everything

	// if camera moved, flag full refresh � every tile shifted
	static int16 prev_cam_x = camera.position_x;
	static int16 prev_cam_y = camera.position_y;
	static int16 prev_sub_x = camera.subpos_x;
	static int16 prev_sub_y = camera.subpos_y;

	if (prev_cam_x != camera.position_x || prev_cam_y != camera.position_y ||
		prev_sub_x != camera.subpos_x || prev_sub_y != camera.subpos_y) {
		refresh = true;
		prev_cam_x = camera.position_x;
		prev_cam_y = camera.position_y;
		prev_sub_x = camera.subpos_x;
		prev_sub_y = camera.subpos_y;
		return;  // full refresh handles everything
	}

	// flag tiles at current and previous position for each entity that is moving
	for (uint8 i = 0; i < entities.active; ++i) {
		const uint8 vel = entities.velocities[i];
		const int8 vx = entity_get_velocity_x(vel);
		const int8 vy = entity_get_velocity_y(vel);
		if (0 == vx && 0 == vy) { continue; }

		const uint32 size = entities.sizes[i];
		const uint8 size_w = entity_get_size_width (size);
		const uint8 size_h = entity_get_size_height(size);
		
		const uint32 position = entities.positions[i];
		const int16 cur_x = entity_get_position_x(position);
		const int16 cur_y = entity_get_position_y(position);
		const int16 sub_x = entity_get_subpos_x(position);
		const int16 sub_y = entity_get_subpos_y(position);

		// derive previous tile position: if subpos crossed a tile boundary
		// this frame, the previous tile was one back in the velocity direction
		const int16 prv_x = (vx > 0 && sub_x < vx) ? cur_x - 1 : (vx < 0 && sub_x > (int16)TILE_X + vx) ? cur_x + 1 : cur_x;
		const int16 prv_y = (vy > 0 && sub_y < vy) ? cur_y - 1 : (vy < 0 && sub_y > (int16)TILE_Y + vy) ? cur_y + 1 : cur_y;

		// flag current position
		for (int y = -(int)size_h; y <= 1; ++y)
			for (int x = -1; x <= (int)size_w; ++x)
				flag_dirt((int16)(cur_x + x), (int16)(cur_y + y));

		// flag previous position if tile changed
		if (prv_x != cur_x || prv_y != cur_y) {
			for (int y = -(int)size_h; y <= 1; ++y)
				for (int x = -1; x <= (int)size_w; ++x)
					flag_dirt((int16)(prv_x + x), (int16)(prv_y + y));
		}
	}
}

const void Game::render_map_all2() {
	for (int32 y = bounds.min.y; y < bounds.max.y; ++y) {
		for (int32 x = bounds.min.x; x < bounds.max.x; ++x) {

		}
	}

	// loop through active area of map {
		// for each distinct tile, render a 'flattened' tile into an atlas
		// add x, y coords for each distinct tile into an array
	// }
	// loop through atlas {
		// loop through tile Y {
			// create a span of tile X
			// loop through index array {
				// render span at x, y
			// }
		// }
	// }
}

const void Game::render_map_all() {
	int32 xs = 0, xe = max_tiles_x;
	int32 ys = 0, ye = max_tiles_y;

	int32 xpos1 = 0, xpos2 = 0;
	int32 ypos1 = 0, ypos2 = 0;
	int32 t = 0, b = 0;
	int32 l = 0, r = 0;

	const int32 posx = camera.position_x;
	const int32 posy = camera.position_y;

	int32 mapposx1 = posx;
	int32 mapposy1 = posy;
	int32 mapposx2 = posx + max_tiles_x;
	int32 mapposy2 = posy + max_tiles_y;

	if (camera.subpos_x) {
		xpos1 = -camera.subpos_x;
		xpos2 = max_px_x - camera.subpos_x;
		l = camera.subpos_x;
		r = TILE_X - camera.subpos_x;

		++xs;
		// mapposx1 stays as posx — it IS the leading edge tile
		// mapposx2 stays as posx + max_tiles_x — it IS the trailing edge tile
		// xe stays at max_tiles_x — main loop runs xs..xe-1 = 1..max_tiles_x-1
	}

	if (camera.subpos_y) {
		ypos1 = -camera.subpos_y;
		ypos2 = max_px_y - camera.subpos_y;
		t = camera.subpos_y;
		b = TILE_Y - camera.subpos_y;

		++ys;
		// same reasoning for Y
	}

	const bool mapx1_inbounds = mapposx1 >= 0 && mapposx1 < (int)mapw;
	const bool mapx2_inbounds = mapposx2 >= 0 && mapposx2 < (int)mapw;
	const bool mapy1_inbounds = mapposy1 >= 0 && mapposy1 < (int)maph;
	const bool mapy2_inbounds = mapposy2 >= 0 && mapposy2 < (int)maph;

	const int32 moff1 = mapposy1 * mapw;
	const int32 moff2 = mapposy2 * mapw;

	if (ys <= -posy) { ys = -posy; }
	if (ye <= -posy) { ye = -posy; }
	if (ys >= -posy + maph) { ys = -posy + maph; }
	if (ye >= -posy + maph) { ye = -posy + maph; }

	if (xs <= -posx) { xs = -posx; }
	if (xe <= -posx) { xe = -posx; }
	if (xs >= -posx + mapw) { xs = -posx + mapw; }
	if (xe >= -posx + mapw) { xe = -posx + mapw; }

	uint32 mapval;
	uint32 tindex, oindex;
	int32 xpos, ypos, moff;
	int32 mapposx;

	if(camera.subpos_y || camera.subpos_x) {
		// left and right edges
		if (camera.subpos_x) {
			if (mapx1_inbounds) {
				for (int32 y = ys; y < ye; ++y) {
					moff = (y + posy) * mapw;
					mapval = map[moff + mapposx1];

					ypos = y * TILE_Y - camera.subpos_y;

					tindex = (mapval & MAP_MASK_BACK) >> MAP_POS_BACK;
					oindex = (mapval & MAP_MASK_FORE) >> MAP_POS_FORE;
					if (oindex) {
						copy_to_buffer_clip_masked(tindex, oindex, xpos1, ypos, 0ul, l, 0ul, 0ul);
					} else {
						copy_to_buffer_clip(tindex, xpos1, ypos, 0ul, l, 0ul, 0ul);
					}
				}
			}
			if (mapx2_inbounds) {
				for (int32 y = ys; y < ye; ++y) {
					moff = (y + posy) * mapw;
					mapval = map[moff + mapposx2];

					ypos = y * TILE_Y - camera.subpos_y;

					tindex = (mapval & MAP_MASK_BACK) >> MAP_POS_BACK;
					oindex = (mapval & MAP_MASK_FORE) >> MAP_POS_FORE;
					if (oindex) {
						copy_to_buffer_clip_masked(tindex, oindex, xpos2, ypos, 0ul, 0ul, 0ul, r);
					} else {
						copy_to_buffer_clip(tindex, xpos2, ypos, 0ul, 0ul, 0ul, r);
					}
				}
			}
		}

		// top and bottom edges
		if (camera.subpos_y) {
			if (mapy1_inbounds) {
				for (int32 x = xs; x < xe; ++x) {
					mapposx = x + posx;
					mapval = map[moff1 + mapposx];

					xpos = x * TILE_X - camera.subpos_x;

					tindex = (mapval & MAP_MASK_BACK) >> MAP_POS_BACK;
					oindex = (mapval & MAP_MASK_FORE) >> MAP_POS_FORE;
					if (oindex) {
						copy_to_buffer_clip_masked(tindex, oindex, xpos, ypos1, t, 0ul, 0ul, 0ul);
					} else {
						copy_to_buffer_clip(tindex, xpos, ypos1, t, 0ul, 0ul, 0ul);
					}
				}
			}

			if (mapy2_inbounds) {
				for (int32 x = xs; x < xe; ++x) {
					mapposx = x + posx;
					mapval = map[moff2 + mapposx];

					xpos = x * TILE_X - camera.subpos_x;

					tindex = (mapval & MAP_MASK_BACK) >> MAP_POS_BACK;
					oindex = (mapval & MAP_MASK_FORE) >> MAP_POS_FORE;
					if (oindex) {
						copy_to_buffer_clip_masked(tindex, oindex, xpos, ypos2, 0ul, 0ul, b, 0ul);
					} else {
						copy_to_buffer_clip(tindex, xpos, ypos2, 0ul, 0ul, b, 0ul);
					}
				}
			}
		}

		// corners
		if (camera.subpos_y && camera.subpos_x) {
			if (mapy1_inbounds && mapx1_inbounds) {
				mapval = map[moff1 + mapposx1];
			
				tindex = (mapval & MAP_MASK_BACK) >> MAP_POS_BACK;
				oindex = (mapval & MAP_MASK_FORE) >> MAP_POS_FORE;
				if (oindex) {
					copy_to_buffer_clip_masked(tindex, oindex, xpos1, ypos1, t, l, 0ul, 0ul);
				} else {
					copy_to_buffer_clip(tindex, xpos1, ypos1, t, l, 0ul, 0ul);
				}
			}

			if (mapy1_inbounds && mapx2_inbounds) {
				mapval = map[moff1 + mapposx2];

				tindex = (mapval & MAP_MASK_BACK) >> MAP_POS_BACK;
				oindex = (mapval & MAP_MASK_FORE) >> MAP_POS_FORE;
				if (oindex) {
					copy_to_buffer_clip_masked(tindex, oindex, xpos2, ypos1, t, 0ul, 0ul, r);
				} else {
					copy_to_buffer_clip(tindex, xpos2, ypos1, t, 0ul, 0ul, r);
				}
			}

			if (mapy2_inbounds && mapx1_inbounds) {
				mapval = map[moff2 + mapposx1];

				tindex = (mapval & MAP_MASK_BACK) >> MAP_POS_BACK;
				oindex = (mapval & MAP_MASK_FORE) >> MAP_POS_FORE;
				if (oindex) {
					copy_to_buffer_clip_masked(tindex, oindex, xpos1, ypos2, 0ul, l, b, 0ul);
				} else {
					copy_to_buffer_clip(tindex, xpos1, ypos2, 0ul, l, b, 0ul);
				}
			}

			if (mapy2_inbounds && mapx2_inbounds) {
				mapval = map[moff2 + mapposx2];

				tindex = (mapval & MAP_MASK_BACK) >> MAP_POS_BACK;
				oindex = (mapval & MAP_MASK_FORE) >> MAP_POS_FORE;
				if (oindex) {
					copy_to_buffer_clip_masked(tindex, oindex, xpos2, ypos2, 0ul, 0ul, b, r);
				} else {
					copy_to_buffer_clip(tindex, xpos2, ypos2, 0ul, 0ul, b, r);
				}
			}
		}
	}

	// main area
	for (int32 y = ys; y < ye; ++y) {
		ypos = (y * TILE_Y) - camera.subpos_y;
		moff = (y + posy) * mapw;

		for (int32 x = xs; x < xe; ++x) {
			xpos = x * TILE_X - camera.subpos_x;
			mapval = map[moff + x + posx];

			tindex = (mapval & MAP_MASK_BACK) >> MAP_POS_BACK;
			oindex = (mapval & MAP_MASK_FORE) >> MAP_POS_FORE;
			if (oindex) {
				copy_to_buffer_masked(tindex, oindex, xpos, ypos);
			} else {
				copy_to_buffer(tindex, xpos, ypos);
			}
		}
	}
}

const void Game::render_map_dirty() {
	const int32 posx = camera.position_x;
	const int32 posy = camera.position_y;

	const int16 minx = bounds.min.x;
	const int16 miny = bounds.min.y;
	const int16 maxx = bounds.max.x;
	const int16 maxy = bounds.max.y;

	const uint8 intx = camera.subpos_x;
	const uint8 inty = camera.subpos_y;

	uint32 interp_top, interp_bottom, interp_left, interp_right;

	if (intx) {
		interp_left  = intx;
		interp_right = TILE_X - intx;
	} else {
		interp_left  = 0;
		interp_right = 0;
	}

	if (inty) {
		interp_top    = inty;
		interp_bottom = TILE_Y - inty;
	} else {
		interp_top    = 0;
		interp_bottom = 0;
	}

	for (uint32 i = 0; i < dirty_len; ++i) {
		const Tile & dt = dirty_tiles[i];

		const uint32 tindex = (dt.tile & MAP_MASK_BACK) >> MAP_POS_BACK;
		const uint32 oindex = (dt.tile & MAP_MASK_FORE) >> MAP_POS_FORE;

		const int32 dx = (int32)dt.pos.x;
		const int32 dy = (int32)dt.pos.y;

		const bool on_left   = dx < minx || (dx == minx && 0 != intx);
		const bool on_right  = dx > maxx || (dx == maxx && 0 != intx);
		const bool on_top    = dy < miny || (dy == miny && 0 != inty);
		const bool on_bottom = dy > maxy || (dy == maxy && 0 != inty);

		const int32 xpos = (dx - posx) * TILE_X - intx;
		const int32 ypos = (dy - posy) * TILE_Y - inty;

		expand_dirty_px_bounds(xpos, ypos, TILE_X, TILE_Y);

		if (on_top || on_bottom || on_right || on_left) {
			const uint32 clip_top    = on_top    * interp_top;
			const uint32 clip_bottom = on_bottom * interp_bottom;
			const uint32 clip_left   = on_left   * interp_left;
			const uint32 clip_right  = on_right  * interp_right;

			if (oindex) {
				copy_to_buffer_clip_masked(tindex, oindex, xpos, ypos, clip_top, clip_left, clip_bottom, clip_right);
			} else {
				copy_to_buffer_clip(tindex, xpos, ypos, clip_top, clip_left, clip_bottom, clip_right);
			}
		} else {
			if (oindex) {
				copy_to_buffer_masked(tindex, oindex, xpos, ypos);
			} else {
				copy_to_buffer(tindex, xpos, ypos);
			}
		}
	}
}

const bool Game::render() {
	bool      map_rendered = render_map();
	bool entities_rendered = render_entities(map_rendered);
	return map_rendered || entities_rendered;
}

const bool Game::render_entities(const bool map_rendered) {
	bool rendered = false;

	for(int i = 0; i < entities.active; ++i) {
		// skip if entity hasn't moved and map wasn't redrawn underneath
		// zero velocity packs to 0x88: ((0+8)<<4)|(0+8)
		const uint8 vel = entities.velocities[i];
		if (0x88 == vel && !map_rendered) { continue; }

		const uint32 position = entities.positions[i];
		const int16 position_x = (int16)(entity_get_position_x(position) - camera.position_x);
		const int16 position_y = (int16)(entity_get_position_y(position) - camera.position_y);

		// coarse tile-level cull (camera-relative coordinates)
		// position_y is feet; head is size_h tiles above, so allow extra room at the bottom
		const uint8 size_h = entity_get_size_height(entities.sizes[i]);
		if (
			   position_x < -1
			|| position_x > max_tiles_x
			|| position_y < -1
			|| position_y > max_tiles_y + (int16)size_h - 1
		) { continue; }

		const uint32 sprite = entities.sprites[i];
		const uint8  sprite_index = entity_get_sprite_index(sprite);

		const uint32 sprite_header = *cindices[sprite_index];
		const uint32 sprite_w = (sprite_header & TILE_MASK_WIDTH)  >> TILE_POS_WIDTH;
		const uint32 sprite_h = (sprite_header & TILE_MASK_HEIGHT) >> TILE_POS_HEIGHT;

		// pos_y is feet — draw sprite from head (offset up by sprite height minus one tile)
		const int16 screen_x = (int16)(position_x * TILE_X + entity_get_subpos_x(position) - camera.subpos_x);
		const int16 screen_y = (int16)(position_y * TILE_Y + entity_get_subpos_y(position) - camera.subpos_y - (int16)(sprite_h - TILE_Y));

		// fine pixel-level cull
		if (screen_x + (int16)sprite_w <= 0 || screen_x >= (int16)_w ||
		    screen_y + (int16)sprite_h <= 0 || screen_y >= (int16)_h) {
			continue;
		}

		expand_dirty_px_bounds(screen_x, screen_y, sprite_w, sprite_h);

		// clear off-map pixels in the entity's vicinity to prevent smearing
		// (dirty tile rendering only covers in-map tiles; off-map black area
		// is only cleared during full refresh, so moving entities leave trails)
		if (vel) {
			const int32 map_screen_top  = (int32)(-camera.position_y) * TILE_Y - camera.subpos_y;
			const int32 map_screen_left = (int32)(-camera.position_x) * TILE_X - camera.subpos_x;
			const int32 map_screen_bottom = (int32)((int32)maph - camera.position_y) * TILE_Y - camera.subpos_y;
			const int32 map_screen_right  = (int32)((int32)mapw - camera.position_x) * TILE_X - camera.subpos_x;

			// expanded rect covers previous frame's position too
			const int32 margin = 8;
			const int32 ex0 = max((int32)screen_x - margin, (int32)0);
			const int32 ey0 = max((int32)screen_y - margin, (int32)0);
			const int32 ex1 = min((int32)screen_x + (int32)sprite_w + margin, (int32)_w);
			const int32 ey1 = min((int32)screen_y + (int32)sprite_h + margin, (int32)_h);

			// above map
			if (ey0 < map_screen_top && map_screen_top > 0) {
				const int32 cy1 = min(ey1, map_screen_top);
				for (int32 row = ey0; row < cy1; ++row)
					memset(&buffer->bits[row * stride + ex0], 0, (ex1 - ex0) * sizeof(uint32));
				expand_dirty_px_bounds(ex0, ey0, ex1 - ex0, cy1 - ey0);
			}

			// left of map
			if (ex0 < map_screen_left && map_screen_left > 0) {
				const int32 cx1 = min(ex1, map_screen_left);
				const int32 cy0 = max(ey0, max(map_screen_top, (int32)0));
				for (int32 row = cy0; row < ey1; ++row)
					memset(&buffer->bits[row * stride + ex0], 0, (cx1 - ex0) * sizeof(uint32));
				expand_dirty_px_bounds(ex0, cy0, cx1 - ex0, ey1 - cy0);
			}

			// below map
			if (ey1 > map_screen_bottom && map_screen_bottom < (int32)_h) {
				const int32 cy0 = max(ey0, map_screen_bottom);
				for (int32 row = cy0; row < ey1; ++row)
					memset(&buffer->bits[row * stride + ex0], 0, (ex1 - ex0) * sizeof(uint32));
				expand_dirty_px_bounds(ex0, cy0, ex1 - ex0, ey1 - cy0);
			}

			// right of map
			if (ex1 > map_screen_right && map_screen_right < (int32)_w) {
				const int32 cx0 = max(ex0, map_screen_right);
				const int32 cy0 = max(ey0, max(map_screen_top, (int32)0));
				const int32 cy1 = min(ey1, min(map_screen_bottom, (int32)_h));
				for (int32 row = cy0; row < cy1; ++row)
					memset(&buffer->bits[row * stride + cx0], 0, (ex1 - cx0) * sizeof(uint32));
				expand_dirty_px_bounds(cx0, cy0, ex1 - cx0, cy1 - cy0);
			}
		}

		const uint32 clip_top    = (screen_y < 0)                           ? (uint32)(-screen_y)      : 0;
		const uint32 clip_left   = (screen_x < 0)                           ? (uint32)(-screen_x)      : 0;
		const uint32 clip_bottom = (screen_y + (int16)sprite_h > (int16)_h) ? screen_y + sprite_h - _h : 0;
		const uint32 clip_right  = (screen_x + (int16)sprite_w > (int16)_w) ? screen_x + sprite_w - _w : 0;

		if (clip_top || clip_left || clip_bottom || clip_right) {
			copy_to_buffer_clip_masked(cindices, sprite_index, screen_x, screen_y, clip_top, clip_left, clip_bottom, clip_right);
		} else {
			copy_to_buffer_masked(cindices, sprite_index, screen_x, screen_y);
		}

		rendered = true;
	}

	return rendered;
}

const bool Game::render_map() {
	reset_dirty_px_bounds();

	++current_frame_id;
	if (0 == current_frame_id) {
		memset(dirty_frame_id, 0, sizeof(dirty_frame_id) * mapw * maph);
	}

	if (refresh) {
		// clear buffer to black when viewport extends beyond map edges
		const int16 view_right  = camera.position_x + max_tiles_x + (0 != camera.subpos_x);
		const int16 view_bottom = camera.position_y + max_tiles_y + (0 != camera.subpos_y);
		if (camera.position_x < 0 ||
			camera.position_y < 0 ||
			view_right  > (int16)mapw ||
			view_bottom > (int16)maph) {
			memset(buffer->bits, 0, stride * _h * sizeof(uint32));
		}

		render_map_all();
		refresh = false;
		dirty_len = 0;
		expand_dirty_px_bounds(0, 0, _w, _h);

		return true;
	} else if (dirty_len) {
		render_map_dirty();
		dirty_len = 0;

		return true;
	}

	return false;
}

bool Game::resize(const int w, const int h) {
	_w = w;
	_h = h;


	max_tiles_x = floor<uint8, float>(_w / (float)TILE_X);
	max_tiles_y = floor<uint8, float>(_h / (float)TILE_Y);

	mid_tiles_x = floor<uint8, float>(max_tiles_x * 0.5f);
	mid_tiles_y = floor<uint8, float>(max_tiles_y * 0.5f);

	max_px_y = max_tiles_y * TILE_Y;
	max_px_x = max_tiles_x * TILE_X;

	stride = (uint32)w;

	update_bounds();

	return true;
}

void Game::reset_dirty_px_bounds() {
	dirty_px_bounds.min.x = 0x7FFFFFFF;
	dirty_px_bounds.min.y = 0x7FFFFFFF;
	dirty_px_bounds.max.x = (int32)0x80000000;
	dirty_px_bounds.max.y = (int32)0x80000000;
}

void Game::expand_dirty_px_bounds(int32 x, int32 y, int32 w, int32 h) {
	if (x < dirty_px_bounds.min.x) { dirty_px_bounds.min.x = x; }
	if (y < dirty_px_bounds.min.y) { dirty_px_bounds.min.y = y; }

	const int32 x2 = x + w;
	const int32 y2 = y + h;

	if (x2 > dirty_px_bounds.max.x) { dirty_px_bounds.max.x = x2; }
	if (y2 > dirty_px_bounds.max.y) { dirty_px_bounds.max.y = y2; }

	// clamp to buffer
	if (dirty_px_bounds.min.x < 0)           { dirty_px_bounds.min.x = 0;  }
	if (dirty_px_bounds.min.y < 0)           { dirty_px_bounds.min.y = 0;  }
	if (dirty_px_bounds.max.x > (int32)(_w)) { dirty_px_bounds.max.x = _w; }
	if (dirty_px_bounds.max.y > (int32)(_h)) { dirty_px_bounds.max.y = _h; }
}

void Game::flag_dirt(const int16 x, const int16 y) {
	// clamp to map bounds — off-map coordinates snap to the nearest edge tile
	const uint16 cx = (uint16)(x < 0 ? 0 : x >= (int16)mapw ? mapw - 1 : x);
	const uint16 cy = (uint16)(y < 0 ? 0 : y >= (int16)maph ? maph - 1 : y);

	// cull tiles outside visible bounds
	if (cx < bounds.min.x || cx > bounds.max.x ||
	    cy < bounds.min.y || cy > bounds.max.y) { return; }

	const uint32 index = cy * mapw + cx;

	if (dirty_frame_id[index] == current_frame_id) { return; }

	dirty_frame_id[index] = current_frame_id;
	dirty_tiles[dirty_len] = { map[index], { cx, cy } };

	++dirty_len;
}

void Game::unload() {
	if (NULL != tiles) {
		delete[] tiles;
		tiles = NULL;
	}

	if (NULL != chars) {
		delete[] chars;
		chars = NULL;
	}

	if (NULL != tindices) {
		delete[] tindices;
		tindices = NULL;
	}

	if (NULL != cindices) {
		delete[] cindices;
		cindices = NULL;
	}

	if (NULL != dirty_frame_id) {
		delete[] dirty_frame_id;
		dirty_frame_id = NULL;
	}

	if (NULL != dirty_tiles) {
		delete[] dirty_tiles;
		dirty_tiles = NULL;
	}

	if (NULL != anim_cells) {
		free(anim_cells);
		anim_cells = NULL;
	}

	key_press = NULL;
}