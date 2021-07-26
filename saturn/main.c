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

#if __has_attribute(__fallthrough__)
#define fallthrough __attribute__((__fallthrough__))
#else
#define fallthrough \
    do              \
    {               \
    } while (0) /* fallthrough */
#endif

static vdp2_scrn_cell_format_t format;

//
void CopyTilemap(const uint8_t *tilemap, size_t offset, size_t width, size_t height)
{
    //return;
    // VRAM_BG
    if ((offset & 0xE000) == 0xE000)
    {
        //return;
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

void VDP_SetPlaneALocation(size_t loc) {}

void VDP_SetPlaneBLocation(size_t loc) {}

void VDP_SeekCRAM(size_t offset) {}

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
#if 0
        // 4bpp=>8bpp
        uint8_t *cpd = (uint8_t *)format.cp_table + (vram_offset * 2);

        for (size_t i = 0; i < len; i++)
        {
            uint8_t px = *data++;
            *cpd++ = (px >> 4) & 0xF;
            *cpd++ = px & 0xF;
        }
#else
        uint8_t *cpd = (uint8_t *)format.cp_table + (vram_offset);

        for (size_t i = 0; i < len; i++)
        {
            *cpd++ = (*data++);
        }
#endif
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

void WaitForVBla()
{
    extern uint16_t demo_length;
    extern uint8_t vbla_routine;

    uint8_t routine = vbla_routine;
    if (vbla_routine != 0x00)
    {
        //Set VDP state
        // VDP_SetVScroll(vid_scrpos_y_dup, vid_bg_scrpos_y_dup);

        //Set screen state
        vbla_routine = 0x00;
    }

    //Run VBlank routine
    switch (routine)
    {
    case 0x02:
        sync_palettes();
        fallthrough;
    case 0x14:
        if (demo_length)
            demo_length--;
        break;
    }

    vdp_sync();
}

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
    vram_cycp.pt[0].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
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
}

int main(void)
{
    init_vdp2();

    while (true)
    {
        GM_Sega();
        vdp_sync();
        GM_Title();
        vdp_sync();
    }

    return 0;
}

void user_init(void)
{
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
                              VDP2_TVMD_VERT_224);

    vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
                                    COLOR_RGB1555(1, 0, 3, 15));

    cpu_intc_mask_set(0);

    vdp2_tvmd_display_set();
}

// Gm_Title
uint8_t emeralds;
uint8_t emerald_list[8];

//Player state
uint32_t score;
uint32_t score_life;
uint8_t last_special;
// LevelTime time;
uint16_t rings;
uint8_t lives;
uint8_t continues;

int16_t demo;
uint16_t demo_length;
