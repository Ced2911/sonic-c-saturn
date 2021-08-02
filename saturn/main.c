/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>
#include "saturn.h"

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

//Global assets
const uint8_t art_text[] = {
#include "Resource/Art/Text.h"
};

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

//Video state
uint8_t vbla_routine;

uint8_t sprite_count;
uint8_t hbla_pal;
int16_t hbla_pos;
int16_t vid_scrpos_y_dup, vid_bg_scrpos_y_dup, vid_scrpos_x_dup, vid_bg_scrpos_x_dup, vid_bg3_scrpos_y_dup, vid_bg3_scrpos_x_dup;

uint16_t sprite_buffer[BUFFER_SPRITES][4]; //Apparently the last 16 entries of this intrude other memory in the original
                                           //... now how would I emulate that?
int16_t hscroll_buffer[SCREEN_HEIGHT][2];
static smpc_peripheral_digital_t _digital;
ALIGNED2 uint8_t vdp_vram[VRAM_SIZE];

//=========================================================
// Todo
//=========================================================
void VDP_SetHScrollLocation(size_t loc)
{
    // Unused
}

#define VCELL_SCROLL VDP2_VRAM_ADDR(2, 0x02000)
// @Todo
void VDP_SetVScroll(int16_t scroll_a, int16_t scroll_b)
{
    vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG0, scroll_a << 16);
    vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG1, scroll_b << 16);
}

// @Todo
void VDP_SetHIntPosition(int16_t pos) {}

//=========================================================
// stubs
//=========================================================
void VDPSetupGame(){};
void VDP_FillVRAM(uint8_t data, size_t len) {}

void VDP_SetPlaneALocation(size_t loc) {}
void VDP_SetPlaneBLocation(size_t loc) {}
void VDP_SetSpriteLocation(size_t loc) {}
void VDP_SetPlaneSize(size_t w, size_t h) {}

// Cram
void VDP_SeekCRAM(size_t offset) {}
void VDP_WriteCRAM(const uint16_t *data, size_t len) {}
void VDP_FillCRAM(uint16_t data, size_t len) {}

//=========================================================
// VRAM
//=========================================================
size_t vram_offset = 0;

void VDP_SeekVRAM(size_t offset)
{
    vram_offset = offset;
}

void VDP_WriteVRAM(const uint8_t *data, size_t len)
{
    uint32_t screen_cpd_adr = format[0].cp_table;
    //if (1 && vram_offset < VRAM_FG)
    //{
    //}
    // FG
    if (vram_offset >= VRAM_FG && vram_offset < (VRAM_FG + 0x1000))
    {
        for (size_t i = 0; i < len; i += 2)
        {
            SetTileMap(data, vram_offset);
            data += 2;
        }
    }
    // BG
    else if (vram_offset >= VRAM_BG && vram_offset < VRAM_SONIC)
    {
        for (size_t i = 0; i < len; i += 2)
        {
            SetTileMap(data, vram_offset);
            data += 2;
        }
    }
    // SONIC
    // Pattern data - no transformation
    // Duplicate data in vdp1
    //else if (vram_offset >= VRAM_SONIC && vram_offset < VRAM_SPRITES)
    //{
    //}

    // Sprite tbl - unused
    else if (vram_offset >= VRAM_SPRITES && vram_offset < VRAM_HSCROLL)
    {
        memcpy((void *)vdp_vram + vram_offset, data, len);
    }

    // Scroll - unused
    else if (1 && vram_offset == VRAM_HSCROLL)
    {
        memcpy((void *)vdp_vram + vram_offset, data, len);
    }
    else
    {

        // Pattern data - no transformation
        // Duplicate data in vdp1

        // VDP2
        uint8_t *cpd = (uint8_t *)screen_cpd_adr + (vram_offset);
        memcpy(cpd, data, len);

        // VDP1
        uint8_t *tex = (uint8_t *)vdp1_vram_partitions.texture_base + vram_offset;
        memcpy(tex, data, len);
    }

    vram_offset += len;
}

void ClearScreen()
{
    // Vdp 2
    memset((void *)format[0].map_bases.plane_a, 0, VDP2_SCRN_CALCULATE_PAGE_SIZE(&format[0]));
    memset((void *)format[1].map_bases.plane_a, 0, VDP2_SCRN_CALCULATE_PAGE_SIZE(&format[1]));
    // Sprites
    //memset((void *)vdp1_vram_partitions.texture_base, 0, vdp1_vram_partitions.texture_size);

    //Clear sprite buffer and hscroll buffer
    memset(sprite_buffer, 0, sizeof(sprite_buffer));
    memset(hscroll_buffer, 0, sizeof(hscroll_buffer));

    if (1)
    {
        update_scroll();
        vdp1_clear_cmdt();
    }

    //Reset screen position duplicates
    vid_scrpos_y_dup = 0;
    vid_bg_scrpos_y_dup = 0;
    vid_scrpos_x_dup = 0;
    vid_bg_scrpos_x_dup = 0;
}

//=========================================================
// Impl.
//=========================================================

//General game functions
void ReadJoypads()
{
    uint8_t state;
    uint8_t dir;
    uint8_t btn;

    smpc_peripheral_process();
    smpc_peripheral_digital_port(1, &_digital);

    //Read joypad 1
    dir = (_digital.pressed.raw >> 12) & 0xF;
    btn = (_digital.pressed.raw >> 8) & 0xF;

    state = dir | (btn << 4);
    jpad1_press1 = state & ~jpad1_hold1;
    jpad1_hold1 = state;

    //Read joypad 2
    state = 0;
    jpad2_press = state & ~jpad2_hold;
    jpad2_hold = state;
}

//Interrupts
void WriteVRAMBuffers()
{
    //Read joypad state
    ReadJoypads();
    //Copy palette
    sync_palettes();
    //Copy buffers
    draw_sprites();
    update_scroll();
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
#endif
        draw_sprites();
        update_scroll();

        //Update Sonic's art
        if (sonframe_chg)
        {
            VDP_SeekVRAM(0xF000);
            VDP_WriteVRAM(sgfx_buffer, SONIC_DPLC_SIZE);
            sonframe_chg = false;
        }

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

        VDP_SetHIntPosition(hbla_pos);
#if 0
        //Copy buffers
        VDP_SeekVRAM(VRAM_SPRITES);
        VDP_WriteVRAM((const uint8_t *)sprite_buffer, sizeof(sprite_buffer));
        VDP_SeekVRAM(VRAM_HSCROLL);
        VDP_WriteVRAM((const uint8_t *)hscroll_buffer, sizeof(hscroll_buffer));
#endif
        draw_sprites();
        update_scroll();
        //Run palette cycle
        PCycle_SS();

#if 1
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
        VDP_SetHIntPosition(hbla_pos);
#if 0
        //Copy buffers
        VDP_SeekVRAM(VRAM_SPRITES);
        VDP_WriteVRAM((const uint8_t *)sprite_buffer, sizeof(sprite_buffer));
        VDP_SeekVRAM(VRAM_HSCROLL);
        VDP_WriteVRAM((const uint8_t *)hscroll_buffer, sizeof(hscroll_buffer));

#endif

        draw_sprites();
        update_scroll();

        //Update Sonic's art
        if (sonframe_chg)
        {
            VDP_SeekVRAM(0xF000);
            VDP_WriteVRAM(sgfx_buffer, SONIC_DPLC_SIZE);
            sonframe_chg = false;
        }

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
    gamemode = GameMode_Sega;
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
        .color_mode = VDP1_ENV_COLOR_MODE_PALETTE,
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

    // Vbkak
    vdp_sync_vblank_out_set(vblank_out_handler);
}
