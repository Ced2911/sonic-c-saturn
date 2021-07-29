#pragma once

#include <yaul.h>

extern vdp2_scrn_cell_format_t format[4];
extern vdp1_vram_partitions_t vdp1_vram_partitions;

void SetTileMap(const uint8_t *tilemap, size_t offset);

void draw_sprites();
void update_scroll();
void sync_palettes();
void vdp1_clear_cmdt();

void init_vdp2();
void init_vdp1();
void vblank_out_handler(void *work __unused);

// impl
extern uint8_t vdp_vram[0x10000];