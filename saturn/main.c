/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "Game.h"

#include "Video.h"
#include "Palette.h"
#include "PaletteCycle.h"
#include "Level.h"
#include "LevelDraw.h"
#include "LevelScroll.h"
#include "Object/Sonic.h"
#include "PLC.h"
#include "HUD.h"

#include "GM_Sega.h"
#include "GM_Title.h"
#include "GM_Level.h"
#include "GM_Special.h"
#ifdef SCP_SPLASH
#include "GM_SSRG.h"
#endif

#if __has_attribute(__fallthrough__)
#define fallthrough __attribute__((__fallthrough__))
#else
#define fallthrough \
    do              \
    {               \
    } while (0) /* fallthrough */
#endif

#define ORDER_SYSTEM_CLIP_COORDS_INDEX (0)
#define ORDER_LOCAL_COORDS_INDEX (1)
#define ORDER_SPRITE_START_INDEX (2)

static vdp2_scrn_cell_format_t format;

static vdp1_cmdt_list_t *_cmdt_list = NULL;
static vdp1_vram_partitions_t vdp1_vram_partitions;
static uint8_t *vdp1_pal_addr = NULL;

//
void CopyTilemap(const uint8_t *tilemap, size_t offset, size_t width, size_t height)
{
    //return;
    // VRAM_BG
    if ((offset & 0xE000) == 0xE000)
    {
        return;
    }
    // VRAM_FG
    if ((offset & 0xC000) == 0xC000)
    {
        //return;
    }
    uint32_t page_width = VDP2_SCRN_CALCULATE_PAGE_WIDTH(&format);
    uint32_t page_height = VDP2_SCRN_CALCULATE_PAGE_HEIGHT(&format);
    uint32_t page_size = VDP2_SCRN_CALCULATE_PAGE_SIZE(&format);

    uint16_t *pages = (uint16_t *)format.map_bases.plane_a;

    uint32_t page_x;
    uint32_t page_y;

    // http://md.railgun.works/index.php?title=VDP#Patterns
    for (page_y = 0; page_y < height; page_y++)
    {
        for (page_x = 0; page_x < width; page_x++)
        {
            // https://segaretro.org/Sega_Mega_Drive/Planes

            uint16_t v = *((uint16_t *)tilemap);
            uint16_t page_idx = page_x + (page_width * page_y);

            uint16_t tile_idx = v & 0x7FF;
            uint8_t pal_idx = (v >> 13) & 0x3;

            uint32_t pal_adr = (uint32_t)(format.color_palette) + (pal_idx * 32 * 2);
            uint32_t cpd_adr = (uint32_t)(format.cp_table) + (tile_idx << 5);

            uint8_t y_flip = (v & 0x1000) != 0;
            uint8_t x_flip = (v & 0x0800) != 0;

            uint16_t pnd = VDP2_SCRN_PND_CONFIG_0(1, cpd_adr, pal_adr, y_flip, x_flip);

            pages[page_idx] = pnd;

            tilemap += 2;
        }
    }
}

// input
uint8_t Joypad_GetState1() { return 0; }
uint8_t Joypad_GetState2() { return 0; }

// vdp

void VDPSetupGame(){};
void VDP_FillVRAM(uint8_t data, size_t len) {}

// Mock ?
void VDP_SetPlaneALocation(size_t loc) {}
void VDP_SetPlaneBLocation(size_t loc) {}
void VDP_SetSpriteLocation(size_t loc) {}
void VDP_SetPlaneSize(size_t w, size_t h) {}

// @Todo
void VDP_SetHScrollLocation(size_t loc)
{

    //vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG0, scroll_a);
    //vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG1, scroll_b);
}

// @Todo
void VDP_SetVScroll(int16_t scroll_a, int16_t scroll_b)
{
#if 0
    vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG0, FIX16(scroll_a & 0x3FFF));
    vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG1, FIX16(scroll_b & 0x3FFF));
#endif
}

// @Todo
void VDP_SetHIntPosition(int16_t pos) {}

// Cram
void VDP_SeekCRAM(size_t offset) {}
void VDP_WriteCRAM(const uint16_t *data, size_t len) {}
void VDP_FillCRAM(uint16_t data, size_t len) {}

static uint8_t background_color_idx = 0;
void VDP_SetBackgroundColour(uint8_t index)
{
    background_color_idx = index;
}

void ClearScreen()
{
}

// Debug...
static void _copy_character_pattern_data(const vdp2_scrn_cell_format_t *format)
{
    uint8_t *cpd;
    cpd = (uint8_t *)format->cp_table;

    for (int i = 0; i < 0x100; i++)
    {
        memset(cpd + (i * 64), i, 64);
    }
}

static void _copy_color_palette(const vdp2_scrn_cell_format_t *format)
{
    uint16_t *color_palette;
    color_palette = (uint16_t *)format->color_palette;

    for (int i = 0; i < 512; i++)
    {
        int idx = i;
        uint32_t b = (i + 0) & 31;
        uint32_t r = (i + 10) & 31;
        uint32_t g = (i + 20) & 31;
        color_palette[i] = COLOR_RGB_DATA | COLOR_RGB888_TO_RGB555(r * 8, g * 8, b * 8);
    }
}

static void _copy_map(const vdp2_scrn_cell_format_t *format)
{
    uint32_t page_width;
    page_width = VDP2_SCRN_CALCULATE_PAGE_WIDTH(format);
    uint32_t page_height;
    page_height = VDP2_SCRN_CALCULATE_PAGE_HEIGHT(format);
    uint32_t page_size;
    page_size = VDP2_SCRN_CALCULATE_PAGE_SIZE(format);

    uint16_t *planes[4];
    planes[0] = (uint16_t *)format->map_bases.plane_a;
    planes[1] = (uint16_t *)format->map_bases.plane_b;
    planes[2] = (uint16_t *)format->map_bases.plane_c;
    planes[3] = (uint16_t *)format->map_bases.plane_d;

    uint16_t *a_pages[4];
    a_pages[0] = &planes[0][0];
    a_pages[1] = &planes[0][1 * (page_size / 2)];
    a_pages[2] = &planes[0][2 * (page_size / 2)];
    a_pages[3] = &planes[0][3 * (page_size / 2)];

    uint16_t num;
    num = 0;

    uint32_t page_x;
    uint32_t page_y;
    for (page_y = 0; page_y < page_height; page_y++)
    {
        for (page_x = 0; page_x < page_width; page_x++)
        {
            uint16_t page_idx;
            page_idx = page_x + (page_width * page_y);

            uint16_t pnd;
            pnd = VDP2_SCRN_PND_CONFIG_4(1, (uint32_t)format->cp_table,
                                         (uint32_t)format->color_palette, 0, 0);

            a_pages[0][page_idx] = pnd | (num << 1);

            num++;
        }
    }
}

size_t vram_offset = 0;

void VDP_SeekVRAM(size_t offset)
{
    vram_offset = offset;
}

void VDP_WriteVRAM(const uint8_t *data, size_t len)
{
    // return;
    // Pattern data
    if (1 && vram_offset < 0xC000)
    {
        uint8_t *cpd = (uint8_t *)format.cp_table + (vram_offset);

        for (size_t i = 0; i < len; i++)
        {
            *cpd++ = (*data++);
        }
    }
    if (1 && vram_offset == VRAM_HSCROLL)
    {
        // @todo...
    }
    vram_offset += len;
}

static void sync_palettes()
{
    extern uint16_t dry_palette[4][16];
    // sync palette
    uint16_t *color_palette = (uint16_t *)format.color_palette;
    // *color_palette++ = COLOR_RGB_DATA | COLOR_RGB888_TO_RGB555(0, 0, 255);
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            // https://segaretro.org/Sega_Mega_Drive/Palettes_and_CRAM
            static const uint8_t col_level[] = {0, 6, 10, 14, 18, 21, 25, 31};
            uint16_t cv = dry_palette[i][j];
            uint8_t r = (cv & 0x00E) >> 1;
            uint8_t g = (cv & 0x0E0) >> 5;
            uint8_t b = (cv & 0xE00) >> 9;
            //*color_palette++ = COLOR_RGB_DATA | (col_level[b] << 10) | (col_level[g] << 5) | col_level[r];
            color_palette[(i * 32) + j] = COLOR_RGB_DATA | (col_level[b] << 10) | (col_level[g] << 5) | col_level[r];
        }
    }

    // Update back color
    uint16_t *back_color = (uint16_t *)VDP2_VRAM_ADDR(3, 0x01FFFE);
    *back_color = ((uint16_t *)format.color_palette)[background_color_idx];
}

//Game
ALIGNED4 uint8_t buffer0000[0xA400];

uint8_t gamemode; //MSB acts as a title card flag

int16_t demo;
uint16_t demo_length;
uint16_t credits_num;

uint8_t credits_cheat;

uint8_t debug_cheat, debug_mode;

uint8_t jpad2_hold, jpad2_press;   //Joypad 2 state
uint8_t jpad1_hold1, jpad1_press1; //Joypad 1 state
uint8_t jpad1_hold2, jpad1_press2; //Sonic controls

uint32_t vbla_count;

uint8_t vbla_routine;
uint16_t level_id;

uint8_t wtr_state;
uint16_t demo_length;
uint8_t gamemode;

uint8_t jpad2_hold, jpad2_press;   //Joypad 2 state
uint8_t jpad1_hold1, jpad1_press1; //Joypad 1 state
uint8_t jpad1_hold2, jpad1_press2; //Sonic controls

//
extern void GM_Sega();
extern void GM_Title();
extern void GM_Level();
extern void GM_Special();

//Global assets
const uint8_t art_text[] = {
#include "Resource/Art/Text.h"
};
//General game functions
void ReadJoypads()
{
    uint8_t state;

    //Read joypad 1
    state = Joypad_GetState1();
    jpad1_press1 = state & ~jpad1_hold1;
    jpad1_hold1 = state;

    //Read joypad 2
    state = Joypad_GetState2();
    jpad2_press = state & ~jpad2_hold;
    jpad2_hold = state;
}

// Sprites - @Todo pas optimum...
static void draw_sprites()
{
    vdp1_cmdt_t *cmdts = &_cmdt_list->cmdts[ORDER_SPRITE_START_INDEX];
    vdp1_cmdt_t *vdp1_spr = cmdts;

    size_t n_spr = 0;
    uint16_t *sprite_16 = &sprite_buffer[0][0];
    uint8_t *sprite_addr = (uint8_t *)sprite_16;

    static const vdp1_cmdt_draw_mode_t draw_mode = {
        .raw = 0x0000,
        .bits.color_mode = 0,
        .bits.trans_pixel_disable = false,
        .bits.pre_clipping_disable = true,
        .bits.end_code_disable = true};

    vdp1_cmdt_color_bank_t color_bank = {.raw = 0};

    for (uint8_t i = 0;;)
    {
        //const uint16_t *sprite = (const uint16_t *)(sprite_addr + ((uint16_t)i << 3));
        const uint16_t *sprite = sprite_buffer[i];

        //Get sprite values
        uint16_t sprite_y = sprite[0];
        uint16_t sprite_x = sprite[3];
        uint16_t sprite_sl = sprite[1];
        uint8_t sprite_width = (sprite_sl & SPRITE_SL_W_AND) >> SPRITE_SL_W_SHIFT;
        uint8_t sprite_height = (sprite_sl & SPRITE_SL_H_AND) >> SPRITE_SL_H_SHIFT;
        uint8_t sprite_link = (sprite_sl & SPRITE_SL_L_AND) >> SPRITE_SL_L_SHIFT;

        //Write sprite
        int16_vec2_t xy = {.x = sprite_x, .y = sprite_y};

        vdp1_cmdt_normal_sprite_set(vdp1_spr);
        vdp1_cmdt_param_vertices_set(vdp1_spr, &xy);
        vdp1_cmdt_param_draw_mode_set(vdp1_spr, draw_mode);
        vdp1_cmdt_param_color_mode0_set(vdp1_spr, color_bank);
        vdp1_cmdt_param_size_set(vdp1_spr, sprite_width, sprite_height);
        vdp1_cmdt_param_char_base_set(vdp1_spr, NULL);

        //Go to next sprite
        if (sprite_link != 0)
            i = sprite_link;
        else
            break;

        sprite++;
        vdp1_spr++;
        n_spr++;
    }

    vdp1_cmdt_end_set(&cmdts[n_spr]);
    vdp1_sync_cmdt_list_put(_cmdt_list, 0, NULL, NULL);
}

#define NBG0_LINE_SCROLL VDP2_VRAM_ADDR(1, 0x00000)

static vdp2_scrn_ls_format_t _ls_format = {
    .scroll_screen = VDP2_SCRN_NBG0,
    .line_scroll_table = NBG0_LINE_SCROLL,
    .interval = 0,
    .enable = VDP2_SCRN_LS_HORZ};

static void update_scroll()
{
    fix16_t *dst = (fix16_t *)(NBG0_LINE_SCROLL);
    for (int i = 0; i < 224; i++)
    {
        *dst++ = FIX16(hscroll_buffer[i][1]);
    }
}

//Interrupts
void WriteVRAMBuffers()
{
#if 0
	//Read joypad state
	ReadJoypads();
	
	//Copy palette
	VDP_SeekCRAM(0);
	if (wtr_state)
		VDP_WriteCRAM(&wet_palette[0][0], 0x40);
	else
		VDP_WriteCRAM(&dry_palette[0][0], 0x40);
	
	//Copy buffers
	VDP_SeekVRAM(VRAM_SPRITES);
	VDP_WriteVRAM((const uint8_t*)sprite_buffer, sizeof(sprite_buffer));
	VDP_SeekVRAM(VRAM_HSCROLL);
	VDP_WriteVRAM((const uint8_t*)hscroll_buffer, sizeof(hscroll_buffer));
#else
    draw_sprites();
    sync_palettes();
    update_scroll();
#endif
}

void VBlank()
{
    uint8_t routine = vbla_routine;
    if (vbla_routine != 0x00)
    {
        //Set VDP state
        VDP_SetVScroll(vid_scrpos_y_dup, vid_bg_scrpos_y_dup);

        //Set screen state
        vbla_routine = 0x00;
    }

    //Run VBlank routine
    switch (routine)
    {
    case 0x02:
        WriteVRAMBuffers();
        //Fallthrough
    case 0x14:
        if (demo_length)
            demo_length--;
        break;
    case 0x04:
        WriteVRAMBuffers();
        LoadTilesAsYouMove_BGOnly();
        ProcessDPLC();
        if (demo_length)
            demo_length--;
        break;
    case 0x08:
        //Read joypad state
        ReadJoypads();

        //Copy palette
        sync_palettes();

        //Copy buffers
        VDP_SetHIntPosition(hbla_pos);
#if 0
        VDP_SeekVRAM(VRAM_SPRITES);
        VDP_WriteVRAM((const uint8_t *)sprite_buffer, sizeof(sprite_buffer));
        VDP_SeekVRAM(VRAM_HSCROLL);
        VDP_WriteVRAM((const uint8_t *)hscroll_buffer, sizeof(hscroll_buffer));

        //Update Sonic's art
        if (sonframe_chg)
        {
            VDP_SeekVRAM(0xF000);
            VDP_WriteVRAM(sgfx_buffer, SONIC_DPLC_SIZE);
            sonframe_chg = false;
        }
#endif
        //Copy duplicate plane positions and flags
        scrpos_x_dup.v = scrpos_x.v;
        scrpos_y_dup.v = scrpos_y.v;
        bg_scrpos_x_dup.v = bg_scrpos_x.v;
        bg_scrpos_y_dup.v = bg_scrpos_y.v;
        bg2_scrpos_x_dup.v = bg2_scrpos_x.v;
        bg2_scrpos_y_dup.v = bg2_scrpos_y.v;
        bg3_scrpos_x_dup.v = bg3_scrpos_x.v;
        bg3_scrpos_y_dup.v = bg3_scrpos_y.v;

        fg_scroll_flags_dup = fg_scroll_flags;
        bg1_scroll_flags_dup = bg1_scroll_flags;
        bg2_scroll_flags_dup = bg2_scroll_flags;
        bg3_scroll_flags_dup = bg3_scroll_flags;

        if (hbla_pos >= 96) //Uh?
        {
            //Scroll camera
            LoadTilesAsYouMove();

            //Update level animations and HUD
            AnimateLevelGfx();
            HUD_Update();

            //Process PLCs
            ProcessDPLC2();

            //Decrement demo timer
            if (demo_length)
                demo_length--;
        }
        break;
    case 0x0A:
        //Read joypad state
        ReadJoypads();

        //Copy palette
        VDP_SeekCRAM(0);
        VDP_WriteCRAM(&dry_palette[0][0], 0x40);

#if 0
        //Copy buffers
        VDP_SetHIntPosition(hbla_pos);
        VDP_SeekVRAM(VRAM_SPRITES);
        VDP_WriteVRAM((const uint8_t *)sprite_buffer, sizeof(sprite_buffer));
        VDP_SeekVRAM(VRAM_HSCROLL);
        VDP_WriteVRAM((const uint8_t *)hscroll_buffer, sizeof(hscroll_buffer));

#endif
        //Run palette cycle
        PCycle_SS();

#if 0
        //Update Sonic's art
        if (sonframe_chg)
        {
            VDP_SeekVRAM(0xF000);
            VDP_WriteVRAM(sgfx_buffer, SONIC_DPLC_SIZE);
            sonframe_chg = false;
        }

#endif
        //Decrement demo timer
        if (demo_length)
            demo_length--;
        break;
    case 0x0C:
        //Read joypad state
        ReadJoypads();

        //Copy palette
        sync_palettes();
#if 0
        //Copy buffers
        VDP_SetHIntPosition(hbla_pos);
        VDP_SeekVRAM(VRAM_SPRITES);
        VDP_WriteVRAM((const uint8_t *)sprite_buffer, sizeof(sprite_buffer));
        VDP_SeekVRAM(VRAM_HSCROLL);
        VDP_WriteVRAM((const uint8_t *)hscroll_buffer, sizeof(hscroll_buffer));

        //Update Sonic's art
        if (sonframe_chg)
        {
            VDP_SeekVRAM(0xF000);
            VDP_WriteVRAM(sgfx_buffer, SONIC_DPLC_SIZE);
            sonframe_chg = false;
        }

#endif
        //Copy duplicate plane positions and flags
        scrpos_x_dup.v = scrpos_x.v;
        scrpos_y_dup.v = scrpos_y.v;
        bg_scrpos_x_dup.v = bg_scrpos_x.v;
        bg_scrpos_y_dup.v = bg_scrpos_y.v;
        bg2_scrpos_x_dup.v = bg2_scrpos_x.v;
        bg2_scrpos_y_dup.v = bg2_scrpos_y.v;
        bg3_scrpos_x_dup.v = bg3_scrpos_x.v;
        bg3_scrpos_y_dup.v = bg3_scrpos_y.v;

        fg_scroll_flags_dup = fg_scroll_flags;
        bg1_scroll_flags_dup = bg1_scroll_flags;
        bg2_scroll_flags_dup = bg2_scroll_flags;
        bg3_scroll_flags_dup = bg3_scroll_flags;

        //Scroll camera
        LoadTilesAsYouMove();

        //Update level animations and HUD
        AnimateLevelGfx();
        HUD_Update();

        //Process PLCs
        ProcessDPLC();
        break;
    case 0x12:
        WriteVRAMBuffers();
        ProcessDPLC();
        break;
    }

    //Update music

    //Increment VBlank counter
    vbla_count++;
}

void WaitForVBla()
{
    VBlank();
    vdp_sync();
}

void HBlank()
{
}

void init_vdp1()
{
    vdp1_vram_partitions_get(&vdp1_vram_partitions);
    vdp1_pal_addr = (uint8_t *)VDP2_CRAM_MODE_1_OFFSET(0, 1, 0x0000);

    _cmdt_list = vdp1_cmdt_list_alloc(2000);

    vdp1_cmdt_t *cmdt;
    cmdt = &_cmdt_list->cmdts[0];

    static const int16_vec2_t local_coords =
        INT16_VEC2_INITIALIZER(SCREEN_WIDTH / 2,
                               SCREEN_HEIGHT / 2);

    static const int16_vec2_t system_clip_coords =
        INT16_VEC2_INITIALIZER(SCREEN_WIDTH,
                               SCREEN_HEIGHT);

    vdp1_cmdt_system_clip_coord_set(&cmdt[ORDER_SYSTEM_CLIP_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdt[ORDER_SYSTEM_CLIP_COORDS_INDEX],
                               CMDT_VTX_SYSTEM_CLIP, &system_clip_coords);

    vdp1_cmdt_local_coord_set(&cmdt[ORDER_LOCAL_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdt[ORDER_LOCAL_COORDS_INDEX],
                               CMDT_VTX_LOCAL_COORD, &local_coords);
}

void init_vdp2()
{

    format.scroll_screen = VDP2_SCRN_NBG0;
    format.cc_count = VDP2_SCRN_CCC_PALETTE_16;
    format.character_size = 1 * 1;
    format.pnd_size = 1;
    format.auxiliary_mode = 0;
    format.plane_size = 1 * 1;
    format.cp_table = (uint32_t)VDP2_VRAM_ADDR(0, 0x00000);
    format.color_palette = (uint32_t)VDP2_CRAM_MODE_1_OFFSET(0, 0, 0);
    format.map_bases.plane_a = (uint32_t)VDP2_VRAM_ADDR(0, 0x08000);
    format.map_bases.plane_b = (uint32_t)VDP2_VRAM_ADDR(0, 0x08000);
    format.map_bases.plane_c = (uint32_t)VDP2_VRAM_ADDR(0, 0x08000);
    format.map_bases.plane_d = (uint32_t)VDP2_VRAM_ADDR(0, 0x08000);

    vdp2_vram_cycp_t vram_cycp;

    vram_cycp.pt[0].t0 = VDP2_VRAM_CYCP_PNDR_NBG0;
    vram_cycp.pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[0].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[0].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[0].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

    vram_cycp.pt[1].t0 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[1].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[1].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[1].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[1].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[1].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[1].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[1].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

    vram_cycp.pt[2].t0 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

    vram_cycp.pt[3].t0 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

    vdp2_vram_cycp_set(&vram_cycp);

    _copy_character_pattern_data(&format);
    _copy_color_palette(&format);
    _copy_map(&format);

    vdp2_scrn_cell_format_set(&format);
    vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
    vdp2_scrn_display_set(VDP2_SCRN_NBG0, /* transparent = */ false);

    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
                              VDP2_TVMD_VERT_224);
    vdp2_tvmd_display_set();

    vdp2_scrn_ls_set(&_ls_format);
}
#include "Game.h"
int main(void)
{
    init_vdp2();
    init_vdp1();
#if 0
    while (true)
    {
        GM_Sega();
        vdp_sync();
        GM_Title();
        vdp_sync();
    }
#else
    while (1)
    {
        switch (gamemode & 0x7F)
        {
        case GameMode_Sega:
            GM_Sega();
            break;
        case GameMode_Title:
            GM_Title();
            break;
        case GameMode_Level:
        case GameMode_Demo:
            GM_Level();
            break;
        case GameMode_Special:
            GM_Special();
            break;
#ifdef SCP_SPLASH
        case GameMode_SSRG:
            GM_SSRG();
            break;
#endif
        default:
            VDPSetupGame();
            gamemode = GameMode_Sega;
            break;
        }
    }
#endif
    return 0;
}

void user_init(void)
{
    static const struct vdp1_env vdp1_env = {
        .bpp = VDP1_ENV_BPP_16,
        .rotation = VDP1_ENV_ROTATION_0,
        .color_mode = VDP1_ENV_COLOR_MODE_RGB_PALETTE,
        .sprite_type = 0,
        .erase_color = COLOR_RGB1555(0, 0, 0, 0),
        .erase_points = {
            {0, 0},
            {SCREEN_WIDTH, SCREEN_HEIGHT}}};

    // VDP2
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
                              VDP2_TVMD_VERT_224);

    vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
                                    COLOR_RGB1555(1, 0, 3, 15));

    // VDP1
    vdp1_env_set(&vdp1_env);

    cpu_intc_mask_set(0);

    vdp2_tvmd_display_set();
}

// Gm_Title
#include "Video.h"

#include "Constants.h"
#include "Palette.h"
#include "LevelScroll.h"

#include <string.h>

//Video state
uint8_t vbla_routine;

uint8_t sprite_count;
uint8_t hbla_pal;
int16_t hbla_pos;
int16_t vid_scrpos_y_dup, vid_bg_scrpos_y_dup, vid_scrpos_x_dup, vid_bg_scrpos_x_dup, vid_bg3_scrpos_y_dup, vid_bg3_scrpos_x_dup;

uint16_t sprite_buffer[BUFFER_SPRITES][4]; //Apparently the last 16 entries of this intrude other memory in the original
                                           //... now how would I emulate that?
int16_t hscroll_buffer[SCREEN_HEIGHT][2];
