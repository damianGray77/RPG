#pragma once
#ifndef ENTITY_H
#define ENTITY_H

#define MAX_ENTITIES 256
#define MAX_IDS      65536

#define ENTITY_CHUNK_MASK 0x3F
#define ENTITY_POS_MASK   0x1F
#define ENTITY_SUB_MASK   0x1F
#define ENTITY_CHUNK_X_OFFSET 26
#define ENTITY_CHUNK_Y_OFFSET 20
#define ENTITY_POS_X_OFFSET   15
#define ENTITY_POS_Y_OFFSET   10
#define ENTITY_SUB_X_OFFSET    5
#define ENTITY_SUB_Y_OFFSET    0

// Precompiled bit operations for setting individual properties in the 'position' variable
#define ENTITY_CHUNK_X_MASK  (ENTITY_CHUNK_MASK  << ENTITY_CHUNK_X_OFFSET)
#define ENTITY_CHUNK_Y_MASK  (ENTITY_CHUNK_MASK  << ENTITY_CHUNK_Y_OFFSET)
#define ENTITY_CHUNK_CLEAR  ~(ENTITY_CHUNK_X_MASK | ENTITY_CHUNK_Y_MASK)
#define ENTITY_POS_X_MASK    (ENTITY_POS_MASK    << ENTITY_POS_X_OFFSET)
#define ENTITY_POS_Y_MASK    (ENTITY_POS_MASK    << ENTITY_POS_Y_OFFSET)
#define ENTITY_POS_CLEAR    ~(ENTITY_POS_X_MASK   | ENTITY_POS_Y_MASK)
#define ENTITY_SUB_X_MASK    (ENTITY_SUB_MASK    << ENTITY_SUB_X_OFFSET)
#define ENTITY_SUB_Y_MASK    (ENTITY_SUB_MASK    << ENTITY_SUB_Y_OFFSET)
#define ENTITY_SUB_CLEAR    ~(ENTITY_SUB_X_MASK   | ENTITY_SUB_Y_MASK)

inline uint8 entity_get_chunk_x   (const uint32 position) { return (position >> ENTITY_CHUNK_X_OFFSET) & ENTITY_CHUNK_MASK; }
inline uint8 entity_get_chunk_y   (const uint32 position) { return (position >> ENTITY_CHUNK_Y_OFFSET) & ENTITY_CHUNK_MASK; }
inline uint8 entity_get_position_x(const uint32 position) { return (position >> ENTITY_POS_X_OFFSET)   & ENTITY_POS_MASK;   }
inline uint8 entity_get_position_y(const uint32 position) { return (position >> ENTITY_POS_Y_OFFSET)   & ENTITY_POS_MASK;   }
inline uint8 entity_get_subpos_x  (const uint32 position) { return (position >> ENTITY_SUB_X_OFFSET)   & ENTITY_SUB_MASK;   }
inline uint8 entity_get_subpos_y  (const uint32 position) { return (position >> ENTITY_SUB_Y_OFFSET)   & ENTITY_SUB_MASK;   }
inline void  entity_set_position(uint32& position, const uint8 chunk_x, const uint8 chunk_y, const uint8 pos_x, const uint8 pos_y, const uint8 sub_x, const uint8 sub_y) {
	position =
		  (chunk_x << ENTITY_CHUNK_X_OFFSET)
		| (chunk_y << ENTITY_CHUNK_Y_OFFSET)
		| (pos_x   << ENTITY_POS_X_OFFSET)
		| (pos_y   << ENTITY_POS_Y_OFFSET)
		| (sub_x   << ENTITY_SUB_X_OFFSET)
		| (sub_y   << ENTITY_SUB_Y_OFFSET)
	;
}
inline void entity_set_chunk (uint32& position, uint8 x, uint8 y) { position = (position & ENTITY_CHUNK_CLEAR) | ((x << ENTITY_CHUNK_X_OFFSET) | (y << ENTITY_CHUNK_Y_OFFSET)); }
inline void entity_set_pos   (uint32& position, uint8 x, uint8 y) { position = (position & ENTITY_POS_CLEAR)   | ((x << ENTITY_POS_X_OFFSET)   | (y << ENTITY_POS_Y_OFFSET));   }
inline void entity_set_subpos(uint32& position, uint8 x, uint8 y) { position = (position & ENTITY_SUB_CLEAR)   | ((x << ENTITY_SUB_X_OFFSET)   | (y << ENTITY_SUB_Y_OFFSET));   }

#define ENTITY_VEL_MASK 0x0F
#define ENTITY_VEL_X_OFFSET 4
#define ENTITY_VEL_Y_OFFSET 0

inline int8 entity_get_velocity_x(const uint8 velocity) { return ((velocity >> ENTITY_VEL_X_OFFSET) & ENTITY_VEL_MASK) - 8; }
inline int8 entity_get_velocity_y(const uint8 velocity) { return ((velocity >> ENTITY_VEL_Y_OFFSET) & ENTITY_VEL_MASK) - 8; }
inline void entity_set_velocity(uint8& velocity, const int8 x, const int8 y) {
	velocity =
		  ((x + 8) << ENTITY_VEL_X_OFFSET)
		| ((y + 8) << ENTITY_VEL_Y_OFFSET)
	;
}

#define ENTITY_SIZE_MASK 0x0F
#define ENTITY_SIZE_W_OFFSET 4
#define ENTITY_SIZE_H_OFFSET 0

inline uint8 entity_get_size_width (const uint8 size) { return (size >> ENTITY_SIZE_W_OFFSET) & ENTITY_SIZE_MASK; }
inline uint8 entity_get_size_height(const uint8 size) { return (size >> ENTITY_SIZE_H_OFFSET) & ENTITY_SIZE_MASK; }
inline void  entity_set_size(uint8& size, const uint8 width, const uint8 height) {
	size =
		  (width  << ENTITY_SIZE_W_OFFSET)
		| (height << ENTITY_SIZE_H_OFFSET)
	;
}

#define ENTITY_SPRITE_INDEX_MASK 0xFF
#define ENTITY_ANIM_FRAME_MASK   0x0F
#define ENTITY_DURATION_MASK     0x0F
#define ENTITY_LAST_TICKS_MASK   0xFFFF
#define ENTITY_SPRITE_INDEX_OFFSET 24
#define ENTITY_ANIM_FRAME_OFFSET   20
#define ENTITY_DURATION_OFFSET     16
#define ENTITY_LAST_TICKS_OFFSET    0

// Precompiled bit operations for setting individual properties in the 'sprite' variable
#define ENTITY_ANIM_CLEAR ~(ENTITY_ANIM_FRAME_MASK << ENTITY_ANIM_FRAME_OFFSET) & ~(ENTITY_LAST_TICKS_MASK << ENTITY_LAST_TICKS_OFFSET)

inline uint8  entity_get_sprite_index(const uint32 sprite) { return (sprite >> ENTITY_SPRITE_INDEX_OFFSET) & ENTITY_SPRITE_INDEX_MASK; }
inline uint8  entity_get_anim_frame  (const uint32 sprite) { return (sprite >> ENTITY_ANIM_FRAME_OFFSET)   & ENTITY_ANIM_FRAME_MASK;   }
inline uint8  entity_get_duration    (const uint32 sprite) { return (sprite >> ENTITY_DURATION_OFFSET)     & ENTITY_DURATION_MASK;     }
inline uint16 entity_get_last_ticks  (const uint32 sprite) { return (sprite >> ENTITY_LAST_TICKS_OFFSET)   & ENTITY_LAST_TICKS_MASK;   }
inline void   entity_set_sprite(uint32& sprite, const uint8 index, const uint8 frame, const uint8 duration, const uint16 last_ticks) {
	sprite =
		  (index      << ENTITY_SPRITE_INDEX_OFFSET)
		| (frame      << ENTITY_ANIM_FRAME_OFFSET)
		| (duration   << ENTITY_DURATION_OFFSET)
		| (last_ticks << ENTITY_LAST_TICKS_OFFSET)
	;
}
inline void entity_set_anim(uint32& sprite, uint8 frame, uint16 last_ticks) {
	sprite = (sprite & ENTITY_ANIM_CLEAR) | ((frame << ENTITY_ANIM_FRAME_OFFSET) | (last_ticks << ENTITY_LAST_TICKS_OFFSET));
}

// Entity "Lite"
typedef struct Camera Camera;
struct Camera {
	uint8 chunk_x;
	uint8 chunk_y;

	uint8 position_x;
	uint8 position_y;

	uint8 subpos_x;
	uint8 subpos_y;

	int8 velocity_x;
	int8 velocity_y;

	uint8 locked_to_entity_id;
};

typedef struct Entities Entities;
struct Entities {
	uint8 length;
	uint8 active;

	uint32 positions [MAX_ENTITIES]; // Bit packed into CCCCCCccccccXXXXXYYYYYSSSSSsssss   chunk_x (6) chunk_y (6) position_x (5) position_y (5) sub_x (5) sub_y(5)
	uint32 sprites   [MAX_ENTITIES]; // Bit packed into iiiiiiiiaaaaddddllllllllllllllll   sprite_index (8) animation_frame (4) duration (4) last_frame_ticks (16)
	uint8  velocities[MAX_ENTITIES]; // Bit packed into XXXXYYYY                           velocity_x (4) velocity_y (4)
	uint8  sizes     [MAX_ENTITIES]; // Bit packed into XXXXYYYY                           size_x (4) size_y (4)
	uint16 ids       [MAX_ENTITIES];

	int16 indices[MAX_IDS]; // Sparse map of entity ids -> array indexes

	void init() {
		length = 0;
		active = 0;
		for (uint32 i = 0; i < MAX_IDS; ++i) { indices[i] = -1; }
		memset(ids,        0, sizeof(ids)       );
		memset(sizes,      0, sizeof(sizes)     );
		memset(positions,  0, sizeof(positions) );
		memset(velocities, 0, sizeof(velocities));
		memset(sprites,    0, sizeof(sprites)   );
	}

	bool add(const uint16 id, const uint8 size, const uint32 position, const uint8 velocity, const uint32 sprite) {
		if (length >= MAX_ENTITIES) { return false; }

		indices[id] = length;
		
		ids       [length] = id;
		sizes     [length] = size;
		positions [length] = position;
		velocities[length] = velocity;
		sprites   [length] = sprite;
		++length;

		return true;
	}

	void remove(const uint16 id) {
		int16 i = indices[id];
		if (-1 == i) { return; } // not found

		indices[id] = -1;

		if (i < active) { --active; } // if the entity being removed is active, decrement active count
		
		--length;
		if (i == length) { return; } // last entity, nothing to swap

		ids       [i] = ids       [length];
		sizes     [i] = sizes     [length];
		positions [i] = positions [length];
		velocities[i] = velocities[length];
		sprites   [i] = sprites   [length];

		indices[ids[i]] = i;
	}

	inline void swap_entity(const uint16 id1, const uint16 id2) {
		int16 i1 = indices[id1];
		int16 i2 = indices[id2];
		if (-1 == i1 || -1 == i2 || i1 == i2) { return; } // not found or same entity

		swap<int16> (indices   [id1], indices   [id2]);
		swap<uint16>(ids       [i1],  ids       [i2] );
		swap<uint8> (sizes     [i1],  sizes     [i2] );
		swap<uint32>(sprites   [i1],  sprites   [i2] );
		swap<uint32>(positions [i1],  positions [i2] );
		swap<uint8> (velocities[i1],  velocities[i2] );
	}

	inline void activate(const uint16 id) {
		int16 i = indices[id];
		if (-1 == i || i < active) { return; } // not found or already active

		// swap entity into the first inactive slot (active block start)
		if (i != active) { swap_entity(id, ids[active]); }

		++active;
	}

	inline void deactivate(const uint16 id) {
		int16 i = indices[id];
		if (-1 == i || i >= active) { return; } // not found or already inactive

		--active;
		
		// move entity to the first inactive slot
		if (i != active) { swap_entity(id, ids[active]); }
	}

	inline int16 copy_position_and_velocity(const uint16 id, uint8& chunk_x, uint8& chunk_y, uint8& pos_x, uint8& pos_y, uint8& sub_x, uint8& sub_y, int8& vel_x, int8& vel_y) {
		int16 i = indices[id];
		if (-1 == i) { return - 1; } // not found
		const uint32 position = positions[i];
		const uint8  velocity = velocities[i];

		chunk_x = entity_get_chunk_x   (position);
		chunk_y = entity_get_chunk_y   (position);
		pos_x   = entity_get_position_x(position);
		pos_y   = entity_get_position_y(position);
		sub_x   = entity_get_subpos_x  (position);
		sub_y   = entity_get_subpos_y  (position);
		vel_x   = entity_get_velocity_x(velocity);
		vel_y   = entity_get_velocity_y(velocity);

		return i;
	}
};

#endif