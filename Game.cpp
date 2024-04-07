#include "pch.h"
#include "Game.h"

Game::Game() {
	_h = 0;
	_w = 0;

	max_tiles_x = 0;
	max_tiles_y = 0;
	mid_tiles_x = 0;
	mid_tiles_y = 0;

	dirty_len   = 0;

	maph = 0;
	mapw = 0;
	max_px_x = 0;
	max_px_y = 0;

	running  = false;
	paused   = false;
	refresh  = false;

	map       = NULL;
	tiles     = NULL;
	tindices  = NULL;

	buffer    = NULL;
	key_press = NULL;
	dirty     = NULL;
	
	dirty_tiles = NULL;
}

bool Game::init() {
	running = true;
	paused  = false;

	map      = new uint32[65535];
	tiles    = new uint32[65535];
	tindices = new uint32*[256];

	state  = { 10, 7 };
	interp = {  0, 0 };

	load();

	return true;
}

void Game::load() {
	load_map();
	load_tiles();
}

void Game::load_map() {
	mapw = 30;
	maph = 20;
	int i = mapw * maph;
	while (--i >= 0) {
		bool wall = 0;
		uint back = 1;
		uint fore = 0 == rand() % 17 ? 2 : 0 == rand() % 17 ? 3 : 0;

		map[i] =
			  (ulong)(wall << 31)
			+ (ulong)(back << 16)
			+ (ulong)(fore <<  0)
		;
	}

	//dirty_len = (maph * mapw + 7) / 8;
	//dirty = new ubyte[dirty_len];
	dirty_len = 0;
	dirty_tiles = new DirtyTile[maph * mapw];

	refresh = true;
}

void Game::load_tiles() {
	size_t size = sizeof(ulong);

	tindices[0] = tiles;
	tiles[0] =
		  ((ulong)TILETYPE_NONE << 30)
		+ ((ulong)32            << 14)
		+ ((ulong)32             << 0)
	;
	memset(tindices[0] + 1, 0, size * 1024); // first tile is a blank black square

	int index = 1025;

	const char* tile_files[4] = {
		  ""
		, "E:\\Projects\\RPG\\Resources\\tile0001.bmp"
		, "E:\\Projects\\RPG\\Resources\\over0001.bmp"
		, "E:\\Projects\\RPG\\Resources\\over0002.bmp"
	};

	int i;

	for (i = 1; i < 2; ++i) {
		index = load_tile(tile_files[i], TILETYPE_BACK, i, index) + 1;
	}

	for (i = 2; i < 4; ++i) {
		index = load_tile(tile_files[i], TILETYPE_MASK, i, index) + 1;
	}
}

int Game::load_tile(const char *file, const tile_type type, const int i, const int start) {
	Bitmap* tile = new Bitmap(file);

	int len = tile->w * tile->h;

	tindices[i] = tiles + start;
	tiles[start] =
		  ((ulong)type    << 30)
		+ ((ulong)tile->w << 14)
		+ ((ulong)tile->h  << 0)
	;
	memcpy(tindices[i] + 1, tile->data, sizeof(ulong) * len);

	int end = start + len;

	tile->unload();
	tile = NULL;

	return end;
}

/// <summary>
/// Copy a background tile to a buffer
/// </summary>
void Game::copy_to_buffer(const int index, const int x, const int y) {
	ulong *tile = tindices[index];

	const size_t size = sizeof(ulong) * TILE_X;

	const ulong boff = _w * y + x;

	ulong *bptr = (ulong *)(buffer->bits + boff);
	ulong *tptr = (ulong *)(tile         + 1);
	for (int ty = 0; ty < TILE_Y; ++ty) {
		memcpy(bptr, tptr, size);

 		bptr += _w;
		tptr += TILE_X;
	}
}

/// <summary>
/// Copy a background tile to a buffer
/// </summary>
void Game::copy_to_buffer_clip(const int index, const int x, const int y, const int t, const int l, const int b, const int r) {
	ulong *tile = tindices[index];

	const long boff = _w * (y + t) + x + l;
	const long toff = TILE_X * t + l + 1;

	const int cw = TILE_X - r - l;
	const int ch = TILE_Y - b - t;

	const size_t size = sizeof(ulong) * cw;

	ulong *bptr = (ulong *)(buffer->bits + boff);
	ulong *tptr = (ulong *)(tile         + toff);

	for (int ty = 0; ty < ch; ++ty) {
		memcpy(bptr, tptr, size);

		bptr += _w;
		tptr += TILE_X;
	}
}

/// <summary>
/// Copy an overlay mask tile with a background tile to a buffer
/// </summary>
void Game::copy_to_buffer_masked(const int tindex, const int oindex, const int x, const int y) {
	ulong *tile = tindices[tindex];
	ulong *over = tindices[oindex];

	const ulong boff = _w * y + x;

	ulong *bptr = (ulong*)(buffer->bits + boff);
	ulong *tptr = (ulong*)(tile         + 1);
	ulong *optr = (ulong*)(over         + 1);

	for (int oy = 0; oy < TILE_Y; ++oy) {
		for (int ox = 0; ox < TILE_X; ++ox) {
			const ulong fore = *(optr + ox);
			const ulong back = *(tptr + ox);
			const ulong mask = -(0 == (fore & 0x00ffffff));

			*(bptr + ox) = (back & mask) | (fore & ~mask);
		}

		bptr += _w;
		optr += TILE_X;
		tptr += TILE_X;
	}
}

/// <summary>
/// Copy an overlay mask tile with a background tile to a buffer
/// </summary>
void Game::copy_to_buffer_clip_masked(const int tindex, const int oindex, const int x, const int y, const int t, const int l, const int b, const int r) {
	ulong *tile = tindices[tindex];
	ulong *over = tindices[oindex];

	const long boff = _w * (y + t) + x + l;
	const long ooff = TILE_X * t + l + 1;

	const int cw = TILE_X - r - l;
	const int ch = TILE_Y - b - t;

	ulong* bptr = (ulong*)(buffer->bits + boff);
	ulong* tptr = (ulong*)(tile         + ooff);
	ulong* optr = (ulong*)(over         + ooff);

	for (int oy = 0; oy < ch; ++oy) {
		for (int ox = 0; ox < cw; ++ox) {
			const ulong fore = *(optr + ox);
			const ulong back = *(tptr + ox);
			const ulong mask = -(0 == (fore & 0x00ffffff));

			*(bptr + ox) = (back & mask) | (fore & ~mask);
		}

		bptr += _w;
		optr += TILE_X;
		tptr += TILE_X;
	}
}

/// <summary>
/// Copy an overlay mask tile with an arbitrary width and height to a buffer
/// </summary>
void Game::copy_to_buffer_masked(const int oindex, const int ow, const int oh, const int x, const int y) {
	ulong *over = tindices[oindex];

	const ulong boff = _w * y + x;

	ulong *bptr = (ulong *)(buffer->bits + boff);
	ulong *optr = (ulong *)(over         + 1);

	for (int oy = 0; oy < oh; ++oy) {
		for (int ox = 0; ox < ow; ++ox) {
			const ulong fore = *(optr + ox);
			const ulong back = *(bptr + ox);
			const ulong mask = -(0 == (fore & 0x00ffffff));

			*(bptr + ox) = (back & mask) | (fore & ~mask);
		}

		bptr += _w;
		optr += ow;
	}
}

/// <summary>
/// Copy an overlay mask tile with an arbitrary width and height to a buffer
/// </summary>
void Game::copy_to_buffer_clip_masked(const int oindex, const int ow, const int oh, const int x, const int y, const int t, const int l, const int b, const int r) {
	ulong *over = tindices[oindex];

	const long boff = _w * (y + t) + x + l;
	const long ooff = ow * t + l + 1;

	const int cw = ow - r - l;
	const int ch = oh - b - t;

	ulong *bptr = (ulong *)(buffer->bits + boff);
	ulong *optr = (ulong *)(over         + ooff);

	for (int oy = 0; oy < ch; ++oy) {
		for (int ox = 0; ox < cw; ++ox) {
			const ulong fore = *(optr + ox);
			const ulong back = *(bptr + ox);
			const ulong mask = -(0 == (fore & 0x00ffffff));

			*(bptr + ox) = (back & mask) | (fore & ~mask);
		}

		bptr += _w;
		optr += ow;
	}
}

void Game::update(const float delta) {
	int unit = (int)(delta * 5.0f);

	update_input(unit);

	static int i = 0; i = (i + 1) % 4;
	map[0] =
		  (ulong)(0 << 31)
		+ (ulong)(1 << 16)
		+ (ulong)(i <<  0)
	;
	flag_dirt(0, 0);
}

bool Game::is_pressed(const uint8 scancode, const bool turnoff) {
	if (!key_press[scancode]) { return false; }

	if (turnoff) { key_press[scancode] = false; }

	return true;
}

void Game::update_input(const int unit) {
	if(!paused) {
		bool kup    = key_press[K_UP]    && (state.y > 0    ||  0 != interp.y);
		bool kleft  = key_press[K_LEFT]  && (state.x > 0    ||  0 != interp.x);
		bool kdown  = key_press[K_DOWN]  && (state.y < maph || 31 != interp.y);
		bool kright = key_press[K_RIGHT] && (state.x < mapw || 31 != interp.x);

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
	int16 maxx = minx + max_tiles_x - 1;
	int16 maxy = miny + max_tiles_y - 1;

	minx -= 0 != interp.x;
	miny -= 0 != interp.y;

	bounds.min.x = minx;
	bounds.min.y = miny;
	bounds.max.x = maxx;
	bounds.max.y = maxy;
}

const void Game::render_all() {
	int16 xs = 0, xe = max_tiles_x;
	int16 ys = 0, ye = max_tiles_y;

	int16 xpos1 = 0, xpos2 = 0;
	int16 ypos1 = 0, ypos2 = 0;
	int16 t = 0, b = 0;
	int16 l = 0, r = 0;

	const int16 posx = state.x - mid_tiles_x;
	const int16 posy = state.y - mid_tiles_y;

	int16 mapposx1 = posx;
	int16 mapposy1 = posy;
	int16 mapposx2 = posx + max_tiles_x;
	int16 mapposy2 = posy + max_tiles_y;

	if (interp.x) {
		const int8 x = interp.x - TILE_X;
		xpos1 = x;
		xpos2 = x + max_px_x;
		l = -x;
		r = interp.x;

		--xe;
		--mapposx1;
		--mapposx2;
	}

	if (interp.y) {
		const int8 y = interp.y - TILE_Y;
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

	const int16 moff1 = mapposy1 * maph;
	const int16 moff2 = mapposy2 * maph;

	if (ys <= -posy) { ys = -posy; }
	if (ye <= -posy) { ye = -posy; }
	if (ys >= -posy + maph) { ys = -posy + maph; }
	if (ye >= -posy + maph) { ye = -posy + maph; }

	if (xs <= -posx) { xs = -posx; }
	if (xe <= -posx) { xe = -posx; }
	if (xs >= -posx + mapw) { xs = -posx + mapw; }
	if (xe >= -posx + mapw) { xe = -posx + mapw; }

	uint32 mapval;
	uint16 tindex, oindex;
	int16 xpos, ypos, moff;
	int16 mapposx, mapposy;

	// left and right edges
	if (interp.x) {
		for (int16 y = ys; y < ye; ++y) {
			mapposy = y + posy;

			ypos = (y * TILE_Y) + interp.y;
			moff = mapposy * maph;

			if (mapx1_inbounds) {
				mapval = map[moff + mapposx1];
				tindex = (mapval & 0x7fff0000) >> 16;

				if (tindex) {
					oindex = mapval & 0x0000ffff;

					if (oindex) {
						copy_to_buffer_clip_masked(tindex, oindex, xpos1, ypos, 0, l, 0, 0);
					} else {
						copy_to_buffer_clip(tindex, xpos1, ypos, 0, l, 0, 0);
					}
				}
			}

			if (mapx2_inbounds) {
				mapval = map[moff + mapposx2];
				tindex = (mapval & 0x7fff0000) >> 16;

				if (tindex) {
					oindex = mapval & 0x0000ffff;

					if (oindex) {
						copy_to_buffer_clip_masked(tindex, oindex, xpos2, ypos, 0, 0, 0, r);
					} else {
						copy_to_buffer_clip(tindex, xpos2, ypos, 0, 0, 0, r);
					}
				}
			}
		}
	}

	// top and bottom edges
	if (interp.y) {
		for (int16 x = xs; x < xe; ++x) {
			mapposx = x + posx;

			xpos = (x * TILE_X) + interp.x;

			if (mapy1_inbounds) {
				mapval = map[moff1 + mapposx];
				tindex = (mapval & 0x7fff0000) >> 16;

				if (tindex) {
					oindex = mapval & 0x0000ffff;

					if (oindex) {
						copy_to_buffer_clip_masked(tindex, oindex, xpos, ypos1, t, 0, 0, 0);
					} else {
						copy_to_buffer_clip(tindex, xpos, ypos1, t, 0, 0, 0);
					}
				}
			}

			if (mapy2_inbounds) {
				mapval = map[moff2 + mapposx];
				tindex = (mapval & 0x7fff0000) >> 16;

				if (tindex) {
					oindex = mapval & 0x0000ffff;

					if (oindex) {
						copy_to_buffer_clip_masked(tindex, oindex, xpos, ypos2, 0, 0, b, 0);
					} else {
						copy_to_buffer_clip(tindex, xpos, ypos2, 0, 0, b, 0);
					}
				}
			}
		}
	}

	// corners
	if (interp.y && interp.x) {
		if (mapy1_inbounds && mapx1_inbounds) {
			mapval = map[moff1 + mapposx1];
			tindex = (mapval & 0x7fff0000) >> 16;

			if (tindex) {
				oindex = mapval & 0x0000ffff;

				if (oindex) {
					copy_to_buffer_clip_masked(tindex, oindex, xpos1, ypos1, t, l, 0, 0);
				} else {
					copy_to_buffer_clip(tindex, xpos1, ypos1, t, l, 0, 0);
				}
			}
		}

		if (mapy1_inbounds && mapx2_inbounds) {
			mapval = map[moff1 + mapposx2];
			tindex = (mapval & 0x7fff0000) >> 16;

			if (tindex) {
				oindex = mapval & 0x0000ffff;

				if (oindex) {
					copy_to_buffer_clip_masked(tindex, oindex, xpos2, ypos1, t, 0, 0, r);
				} else {
					copy_to_buffer_clip(tindex, xpos2, ypos1, t, 0, 0, r);
				}
			}
		}

		if (mapy2_inbounds && mapx1_inbounds) {
			mapval = map[moff2 + mapposx1];
			tindex = (mapval & 0x7fff0000) >> 16;

			if (tindex) {
				oindex = mapval & 0x0000ffff;

				if (oindex) {
					copy_to_buffer_clip_masked(tindex, oindex, xpos1, ypos2, 0, l, b, 0);
				} else {
					copy_to_buffer_clip(tindex, xpos1, ypos2, 0, l, b, 0);
				}
			}
		}

		if (mapy2_inbounds && mapx2_inbounds) {
			mapval = map[moff2 + mapposx2];
			tindex = (mapval & 0x7fff0000) >> 16;

			if (tindex) {
				oindex = mapval & 0x0000ffff;

				if (oindex) {
					copy_to_buffer_clip_masked(tindex, oindex, xpos2, ypos2, 0, 0, b, r);
				} else {
					copy_to_buffer_clip(tindex, xpos2, ypos2, 0, 0, b, r);
				}
			}
		}
	}

	// main area
	for (int16 y = ys; y < ye; ++y) {
		mapposy = y + posy;

		ypos = (y * TILE_Y) + interp.y;
		moff = mapposy * maph;

		for (int16 x = xs; x < xe; ++x) {
			mapposx = x + posx;

			mapval = map[moff + mapposx];
			tindex = (mapval & 0x7fff0000) >> 16;

			if (!tindex) { continue; }

			xpos = (x * TILE_X) + interp.x;

			oindex = mapval & 0x0000ffff;
			if (oindex) {
				copy_to_buffer_masked(tindex, oindex, xpos, ypos);
			} else {
				copy_to_buffer(tindex, xpos, ypos);
			}
		}
	}
}

const void Game::render_dirty() {
	const int16 posx = state.x - mid_tiles_x;
	const int16 posy = state.y - mid_tiles_y;

	int16 t, b, l, r;

	if (interp.x) {
		l = -interp.x + TILE_X;
		r =  interp.x;
	} else {
		l = 0;
		r = 0;
	}

	if (interp.y) {
		t = -interp.y + TILE_Y;
		b =  interp.y;
	} else {
		t = 0;
		b = 0;
	}

	DirtyTile dt;
	uint tindex, oindex;

	for (uint16 i = 0; i < dirty_len; ++i) {
		dt = *(dirty_tiles + i);

		const int16 dx = (int16)dt.pos.x;
		const int16 dy = (int16)dt.pos.y;

		const int16 xpos = (dx - posx) * TILE_X + interp.x;
		const int16 ypos = (dy - posy) * TILE_Y + interp.y;

		const bool on_l = dx < bounds.min.x || (dx == bounds.min.x && 0 != interp.x);
		const bool on_r = dx > bounds.max.x || (dx == bounds.max.x && 0 != interp.x);
		const bool on_t = dy < bounds.min.y || (dy == bounds.min.y && 0 != interp.y);
		const bool on_b = dy > bounds.max.y || (dy == bounds.max.y && 0 != interp.y);

		tindex = (dt.tile & 0x7fff0000) >> 16;
		if (tindex) {
			oindex = dt.tile & 0x0000ffff;

			if (on_t || on_b || on_r || on_l) {
				if (on_t) { b = 0; }
				if (on_b) { t = 0; }
				if (on_l) { r = 0; }
				if (on_r) { l = 0; }
				if (!on_t && !on_b) { t = 0; b = 0; }
				if (!on_l && !on_r) { l = 0; r = 0; }

				if (oindex) {
					copy_to_buffer_clip_masked(tindex, oindex, xpos, ypos, t, l, b, r);
				} else {
					copy_to_buffer_clip(tindex, xpos, ypos, t, l, b, r);
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
}

bool Game::render() {
	if (refresh) {
		render_all();
		refresh = false;
		dirty_len = 0;

		return true;
	} else if(dirty_len) {
		render_dirty();
		dirty_len = 0;

		return true;
	}

	return false;
}

bool Game::resize(const int w, const int h) {
	_w = w;
	_h = h;

	max_tiles_x = floor(_w / (float)TILE_X);
	max_tiles_y = floor(_h / (float)TILE_Y);

	mid_tiles_x = floor(max_tiles_x * 0.5f);
	mid_tiles_y = floor(max_tiles_y * 0.5f);

	max_px_y = max_tiles_y * TILE_Y;
	max_px_x = max_tiles_x * TILE_X;

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

	const uint moff = y * maph;

	*(dirty_tiles + dirty_len) = { map[moff + x], { x, y } };

	++dirty_len;
}

void Game::unload() {
	if (NULL != tiles) {
		delete[] tiles;
		tiles = NULL;
	}

	if (NULL != tindices) {
		delete[] tindices;
		tindices = NULL;
	}

	if (NULL != dirty) {
		delete[] dirty;
		dirty = NULL;
	}

	if (NULL != dirty_tiles) {
		delete[] dirty_tiles;
		dirty_tiles = NULL;
	}

	key_press = NULL;
}