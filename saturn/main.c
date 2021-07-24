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
static vdp2_scrn_cell_format_t format;

//
void CopyTilemap(const uint8_t *tilemap, size_t offset, size_t width, size_t height)
{
    uint32_t page_width = VDP2_SCRN_CALCULATE_PAGE_WIDTH(&format);
    uint32_t page_height = VDP2_SCRN_CALCULATE_PAGE_HEIGHT(&format);
    uint32_t page_size = VDP2_SCRN_CALCULATE_PAGE_SIZE(&format);

    uint16_t *planes[4];
    planes[0] = (uint16_t *)format.map_bases.plane_a;

    uint16_t *a_pages[4];
    a_pages[0] = &planes[0][0];

    uint16_t num;
    num = 0;

    uint32_t page_x;
    uint32_t page_y;

    // http://md.railgun.works/index.php?title=VDP#Patterns
    for (page_y = 0; page_y < height; page_y++)
    {
        for (page_x = 0; page_x < width; page_x++)
        {
            // https://segaretro.org/Sega_Mega_Drive/Planes

            uint16_t v = *((uint16_t *)tilemap);
            uint16_t page_idx;
            page_idx = page_x + (page_width * page_y);

            uint16_t pnd;
            pnd = VDP2_SCRN_PND_CONFIG_2(1, (uint32_t)format.cp_table,
                                         (uint32_t)format.color_palette, v & 12, v & 11);

            a_pages[0][page_idx] = pnd | (v & 0x3FFF);

            tilemap += 2;
        }

        num ^= 1;
    }
}

void VDP_SetPlaneALocation(size_t loc) {}

void VDP_SetPlaneBLocation(size_t loc) {}

void VDP_SeekCRAM(size_t offset) {}

void VDP_SetBackgroundColour(uint8_t index) {}

void ClearScreen() {}

size_t vram_offset = 0;

void VDP_SeekVRAM(size_t offset)
{
    vram_offset = offset;
}

static void
_copy_character_pattern_data(const vdp2_scrn_cell_format_t *format)
{
    uint8_t *cpd;
    cpd = (uint8_t *)format->cp_table;

    for (int i = 0; i < 0x100; i++)
    {
        memset(cpd + (i * 64), i, 64);
    }
}

static void
_copy_color_palette(const vdp2_scrn_cell_format_t *format)
{
    uint16_t *color_palette;
    color_palette = (uint16_t *)format->color_palette;

    for (int i = 0; i < 96; i++)
    {
        int idx = i;
        int r = (((i + 10) % 32) * 8) & 0xFF;
        int g = (((i + 20) % 32) * 8) & 0xFF;
        int b = (((i + 0) % 32) * 8) & 0xFF;
        color_palette[0] = COLOR_RGB_DATA | COLOR_RGB888_TO_RGB555(r, g, b);
    }
}

static void
_copy_map(const vdp2_scrn_cell_format_t *format)
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
            pnd = VDP2_SCRN_PND_CONFIG_1(1, (uint32_t)format->cp_table,
                                         (uint32_t)format->color_palette);

            a_pages[0][page_idx] = pnd | num;

            num ^= 1;
        }

        num ^= 1;
    }
}

void VDP_WriteVRAM(const uint8_t *data, size_t len)
{
    // Pattern data
    if (1 && vram_offset < 0xC000)
    {
        vram_offset += len;

        uint8_t *cpd = (uint8_t *)format.cp_table;
        for (int i = 0; i < len; i++)
        {

            // memcpy(cpd + vram_offset, data, len);
        }
    }
}

void WaitForVBla()
{
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

void init_vdp2()
{

    format.scroll_screen = VDP2_SCRN_NBG0;
    format.cc_count = VDP2_SCRN_CCC_PALETTE_256;
    format.character_size = 1 * 1;
    format.pnd_size = 1;
    format.auxiliary_mode = 0;
    format.plane_size = 1 * 1;
    format.cp_table = (uint32_t)VDP2_VRAM_ADDR(0, 0x00000);
    format.color_palette = (uint32_t)VDP2_CRAM_MODE_0_OFFSET(0, 0, 0);
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