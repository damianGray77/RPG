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

	map      = NULL;
	tiles    = NULL;
	chars    = NULL;
	tindices = NULL;
	cindices = NULL;

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

	state  = { 11, 7 };
	interp = { 16, 0 };

	time.init();

	load();

	return true;
}

void Game::load() {
	load_map();
	load_tiles();
	load_chars();
	load_anims();
}

void Game::load_map() {
	mapw = 30;
	maph = 20;
	int mapwh = mapw * maph;
	int i = mapwh;
	while (--i >= 0) {
		bool wall = 0;
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

		const uint8 anim_handle = time.set_interval(ms);

		tanims[anim_count++] = { i, anim_handle };
	}
}

uint32 Game::load_tile(uint32 *tbuf, uint32 **tibuf, const char *file, const tile_type type, const uint16 i, const uint32 start) {
	Bitmap* tile = new Bitmap(file);

	const uint8 w = tile->w;
	const uint8 h = tile->h;

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
	memcpy(tibuf[i] + 1, tile->data, sizeof(uint32) * len);

	tile->unload();
	tile = NULL;

	return end;
}

/// <summary>
/// Copy a background tile to a buffer
/// </summary>
void Game::copy_to_buffer(const uint32 index, const uint32 x, const uint32 y) {
	const uint32 * tile = tindices[index];

	const uint32 sprite_index = (*tile & TILE_MASK_INDEX) >> TILE_POS_INDEX;

	      uint32 * __restrict buffer_ptr = buffer->bits + (stride * y + x);
	const uint32 * __restrict   tile_ptr = tile + 1 + (TILE_XY * sprite_index);
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

	const uint32 sprite_index = (*tile & TILE_MASK_INDEX) >> TILE_POS_INDEX;

	const uint32 clipped_width  = TILE_X - clip_right  - clip_left;
	const uint32 clipped_height = TILE_Y - clip_bottom - clip_top;

	      uint32 * __restrict buffer_ptr = buffer->bits + stride * (y + clip_top) + x + clip_left;
	const uint32 * __restrict   tile_ptr = tile + 1 + TILE_XY * sprite_index + TILE_X * clip_top + clip_left;
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
	const uint32 * __restrict background_ptr = background_tile + 1;
	const uint32 * __restrict    overlay_ptr =    overlay_tile + 1;

	for (uint32 row = 0; row < TILE_Y; ++row) {
		for (uint32 col = 0; col < TILE_X; ++col) {
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
/// Copy an overlay mask tile with a background tile to a buffer
/// </summary>
void Game::copy_to_buffer_clip_masked(const uint32 tindex, const uint32 oindex, const int32 x, const int32 y, const uint32 clip_top, const uint32 clip_left, const uint32 clip_bottom, const uint32 clip_right) {
	const uint32 * background_tile = tindices[tindex];
	const uint32 *    overlay_tile = tindices[oindex];

	const uint32 tile_offset = TILE_X * clip_top + clip_left + 1;

	const uint32 clipped_width  = TILE_X - clip_right  - clip_left;
	const uint32 clipped_height = TILE_Y - clip_bottom - clip_top;

	      uint32 * __restrict     buffer_ptr = buffer->bits + stride * (y + clip_top) + x + clip_left;
	const uint32 * __restrict background_ptr = background_tile + tile_offset;
	const uint32 * __restrict    overlay_ptr =    overlay_tile + tile_offset;

	for (uint32 row = 0; row < clipped_height; ++row) {
		for (uint32 col = 0; col < clipped_width; ++col) {
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

	for (uint32 row = 0; row < sprite_height; ++row) {
		for (uint32 col = 0; col < sprite_width; ++col) {
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

	for (uint32 row = 0; row < clipped_height; ++row) {
		for (uint32 col = 0; col < clipped_width; ++col) {
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

	const uint16 unit = ceil<uint16, float>(delta * 100.0f);

	update_input(unit);

	update_anims();
}

bool Game::is_pressed(const uint8 scancode, const bool turnoff) {
	if (!key_press[scancode]) { return false; }

	if (turnoff) { key_press[scancode] = false; }

	return true;
}

void Game::update_input(const uint16 unit) {
	if(!paused) {
		const bool kup    = key_press[K_UP]    && (state.y > 0    ||  0 != interp.y);
		const bool kleft  = key_press[K_LEFT]  && (state.x > 0    ||  0 != interp.x);
		const bool kdown  = key_press[K_DOWN]  && (state.y < maph || 31 != interp.y);
		const bool kright = key_press[K_RIGHT] && (state.x < mapw || 31 != interp.x);

		if (kup || kdown || kleft || kright) {
			refresh = true;

			if (kup)    { if(0    == state.y &&      0 != interp.y) { interp.y = 0;          } else { interp.y += unit; } }
			if (kleft)  { if(0    == state.x &&      0 != interp.x) { interp.x = 0;          } else { interp.x += unit; } }
			if (kdown)  { if(maph == state.y && TILE_Y != interp.y) { interp.y = TILE_Y - 1; } else { interp.y -= unit; } }
			if (kright) { if(mapw == state.x && TILE_X != interp.x) { interp.x = TILE_X - 1; } else { interp.x -= unit; } }

			     if (interp.x >= TILE_X) { --state.x; }
			else if (interp.x <       0) { ++state.x; }
			     if (interp.y >= TILE_Y) { --state.y; }
			else if (interp.y <       0) { ++state.y; }

			interp.y &= (TILE_Y - 1);
			interp.x &= (TILE_X - 1);

			update_bounds();
		}
	}

	running = !(
		   is_pressed(K_ESCAPE, true)
		|| is_pressed(K_Q,      true)
	);

	if (is_pressed(K_SPACE, true)) {
		paused = !paused;
	}
}

void Game::update_bounds() {
	      int16 minx = (int16)state.x - mid_tiles_x;
	      int16 miny = (int16)state.y - mid_tiles_y;
	const int16 maxx = minx + max_tiles_x - 1;
	const int16 maxy = miny + max_tiles_y - 1;

	minx -= 0 != interp.x;
	miny -= 0 != interp.y;

	bounds.min.x = minx;
	bounds.min.y = miny;
	bounds.max.x = maxx;
	bounds.max.y = maxy;
}

void Game::update_anims() {
	for (uint32 i = 0; i < anim_count; ++i) {
		const TileAnim & ta    = tanims[i];
		      Interval & inter = time.intervals[ta.anim_handle];

		if (!inter.update()) { continue; }

		uint32 *tile = tindices[ta.tile_index];
		uint32  info = *tile;

		const uint32 frames = (info & TILE_MASK_FRAMES) >> TILE_POS_FRAMES;
		      uint32 index  = (info & TILE_MASK_INDEX)  >> TILE_POS_INDEX;
		if (++index >= frames) { index -= frames; }

		info = (info & ~TILE_MASK_INDEX) | (index << TILE_POS_INDEX);

		*tile = info;

		uint32 mapval;
		uint32 tindex, oindex;

		const uint32 ys = MAX(bounds.min.y, 0);
		const uint32 xs = MAX(bounds.min.x, 0);
		const uint32 ye = MIN(bounds.max.y, maph);
		const uint32 xe = MIN(bounds.max.x, mapw);

		uint32 total_tiles = (ye - ys + 1) * (xe - xs + 1);
		uint32 total_dirt  = 0;
		for (uint32 y = ys; y <= ye; ++y) {
			uint32 moff = y * mapw;
			for (uint32 x = xs; x <= xe; ++x) {
				mapval = map[moff + x];
				tindex = (mapval & MAP_MASK_BACK) >> MAP_POS_BACK;
				oindex = (mapval & MAP_MASK_FORE) >> MAP_POS_FORE;
				
				if (tindex == ta.tile_index  || oindex == ta.tile_index) {
					flag_dirt((uint16)x, (uint16)y);
					++total_dirt;
				}
			}
		}
		//if (total_tiles == total_dirt) { refresh = true; }
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

	const int32 posx = state.x - mid_tiles_x;
	const int32 posy = state.y - mid_tiles_y;

	int32 mapposx1 = posx;
	int32 mapposy1 = posy;
	int32 mapposx2 = posx + max_tiles_x;
	int32 mapposy2 = posy + max_tiles_y;

	if (interp.x) {
		const int32 x = interp.x - TILE_X;
		xpos1 = x;
		xpos2 = x + max_px_x;
		l = -x;
		r = interp.x;

		--xe;
		--mapposx1;
		--mapposx2;
	}

	if (interp.y) {
		const int32 y = interp.y - TILE_Y;
		ypos1 = y;
		ypos2 = y + max_px_y;
		t = -y;
		b = interp.y;

		--ye;
		--mapposy1;
		--mapposy2;
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

	/*if (xs > bounds.min.x || xe < bounds.max.x || ys > bounds.min.y || ye < bounds.max.y) {
		const uint32 y1 = (ys * TILE_Y) + interp.y;
		const uint32 y2 = (ye * TILE_Y) + interp.y;

		if (ys > bounds.min.y) {
			memset(buffer->bits, 0x00000000, (y1 * (uint32)_w) * sizeof(uint32));
		}

		if (ye < bounds.max.y) {
			uint32* bptr = buffer->bits + y2 * (uint32)_w;
			memset(bptr, 0x00000000, ((_h - y2) * (uint32)_w) * sizeof(uint32));
		}

		if (xs > bounds.min.x) {
			const uint32 x = (xs * TILE_X) + interp.x;
			for (uint32 y = y1; y < y2; ++y) {
				uint32* bptr = buffer->bits + y * _w;
				memset(bptr, 0x00000000, x * sizeof(uint32));
			}
		}

		if (xe < bounds.max.x) {
			const uint32 x = (xe * TILE_X) + interp.x;
			for (uint32 y = y1; y < y2; ++y) {
				uint32* bptr = buffer->bits + y * _w + x;
				memset(bptr, 0x00000000, (_w - x) * sizeof(uint32));
			}
		}
	}*/

	if(interp.y || interp.x) {
		// left and right edges
		if (interp.x) {
			if (mapx1_inbounds) {
				for (int32 y = ys; y < ye; ++y) {
					moff = (y + posy) * mapw;
					mapval = map[moff + mapposx1];

					ypos = (y * TILE_Y) + interp.y;

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

					ypos = (y * TILE_Y) + interp.y;

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
		if (interp.y) {
			if (mapy1_inbounds) {
				for (int32 x = xs; x < xe; ++x) {
					mapposx = x + posx;
					mapval = map[moff1 + mapposx];

					xpos = (x * TILE_X) + interp.x;

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

					xpos = (x * TILE_X) + interp.x;

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
		if (interp.y && interp.x) {
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
		ypos = (y * TILE_Y) + interp.y;
		moff = (y + posy) * mapw;

		for (int32 x = xs; x < xe; ++x) {
			xpos = (x * TILE_X) + interp.x;
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
	const int32 posx = state.x - mid_tiles_x;
	const int32 posy = state.y - mid_tiles_y;

	const int16 minx = bounds.min.x;
	const int16 miny = bounds.min.y;
	const int16 maxx = bounds.max.x;
	const int16 maxy = bounds.max.y;

	const int8 intx = interp.x;
	const int8 inty = interp.y;

	uint32 interp_top, interp_bottom, interp_left, interp_right;

	if (intx) {
		interp_left  = -intx + TILE_X;
		interp_right =  intx;
	} else {
		interp_left  = 0;
		interp_right = 0;
	}

	if (inty) {
		interp_top    = -inty + TILE_Y;
		interp_bottom =  inty;
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

		const int32 xpos = (dx - posx) * TILE_X + intx;
		const int32 ypos = (dy - posy) * TILE_Y + inty;

		if (on_top || on_bottom || on_right || on_left) {
			const uint32 clip_top    = !on_bottom * interp_top;
			const uint32 clip_bottom = !on_top    * interp_bottom;
			const uint32 clip_left   = !on_right  * interp_left;
			const uint32 clip_right  = !on_left   * interp_right;

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
	bool   map_rendered = render_map();
	bool chars_rendered = render_chars();
	return map_rendered || chars_rendered;
}

const bool Game::render_chars() {
	const uint16 x = mid_tiles_x * TILE_X - 16;
	const uint16 y = mid_tiles_y * TILE_Y - 32;
	copy_to_buffer_masked(cindices, 0ul, x, y);

	return false;
}

const bool Game::render_map() {
	if (refresh) {
		render_map_all();
		refresh = false;
		dirty_len = 0;

		return true;
	} else if (dirty_len) {
		render_map_dirty();
		dirty_len = 0;

		return true;
	}

	++current_frame_id;
	if (0 == current_frame_id) {
		memset(dirty_frame_id, 0, mapw * maph);
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

void Game::flag_dirt(const uint16 x, const uint16 y) {
	if (
		   (int16)x < bounds.min.x
		|| (int16)x > bounds.max.x
		|| (int16)y < bounds.min.y
		|| (int16)y > bounds.max.y
	) { return; }

	const uint32 moff = y * mapw;
	const uint32 index = moff + x;

	if (dirty_frame_id[index] == current_frame_id) { return; }

	dirty_frame_id[index] = current_frame_id;
	dirty_tiles[dirty_len] = { map[index], { x, y } };

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

	key_press = NULL;
}