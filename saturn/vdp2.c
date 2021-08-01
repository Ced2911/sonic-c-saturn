#include "saturn.h"
#include "Video.h"

#define BG_LINE_SCROLL VDP2_VRAM_ADDR(2, 0x00000)
#define FG_LINE_SCROLL VDP2_VRAM_ADDR(2, 0x01000)
#define VCELL_SCROLL VDP2_VRAM_ADDR(2, 0x02000)

//=========================================================
// Screen setup
//=========================================================
vdp2_scrn_cell_format_t format[4] = {
    {
        .scroll_screen = VDP2_SCRN_NBG0,
        .cc_count = VDP2_SCRN_CCC_PALETTE_16,
        .character_size = 1 * 1,
        .pnd_size = 2,
        .auxiliary_mode = 0,
        .plane_size = 1 * 1,
        .cp_table = (uint32_t)VDP2_VRAM_ADDR(0, 0x00000),
        .color_palette = (uint32_t)VDP2_CRAM_MODE_1_OFFSET(0, 0, 0),
        .sf_type = VDP2_SCRN_SF_TYPE_PRIORITY,
        .sf_mode = 1 /** per tile **/,
        .map_bases.plane_a = (uint32_t)VDP2_VRAM_ADDR(0, 0x10000),
        .map_bases.plane_b = (uint32_t)VDP2_VRAM_ADDR(0, 0x10000),
        .map_bases.plane_c = (uint32_t)VDP2_VRAM_ADDR(0, 0x10000),
        .map_bases.plane_d = (uint32_t)VDP2_VRAM_ADDR(0, 0x10000),
    },
    {
        .scroll_screen = VDP2_SCRN_NBG1,
        .cc_count = VDP2_SCRN_CCC_PALETTE_16,
        .character_size = 1 * 1,
        .pnd_size = 2,
        .auxiliary_mode = 0,
        .plane_size = 1 * 1,
        .cp_table = (uint32_t)VDP2_VRAM_ADDR(0, 0x00000),
        .color_palette = (uint32_t)VDP2_CRAM_MODE_1_OFFSET(0, 0, 0),
        .sf_type = VDP2_SCRN_SF_TYPE_PRIORITY,
        .sf_mode = 1 /** per tile **/,
        .map_bases.plane_a = (uint32_t)VDP2_VRAM_ADDR(1, 0x10000),
        .map_bases.plane_b = (uint32_t)VDP2_VRAM_ADDR(1, 0x10000),
        .map_bases.plane_c = (uint32_t)VDP2_VRAM_ADDR(1, 0x10000),
        .map_bases.plane_d = (uint32_t)VDP2_VRAM_ADDR(1, 0x10000),
    },
    {
        .scroll_screen = VDP2_SCRN_NBG2,
        .cc_count = VDP2_SCRN_CCC_PALETTE_16,
        .character_size = 1 * 1,
        .pnd_size = 1,
        .auxiliary_mode = 0,
        .plane_size = 1 * 1,
        .cp_table = (uint32_t)VDP2_VRAM_ADDR(0, 0x00000),
        .color_palette = (uint32_t)VDP2_CRAM_MODE_1_OFFSET(0, 0, 0),
        .map_bases.plane_a = (uint32_t)VDP2_VRAM_ADDR(1, 0x00000),
        .map_bases.plane_b = (uint32_t)VDP2_VRAM_ADDR(1, 0x00000),
        .map_bases.plane_c = (uint32_t)VDP2_VRAM_ADDR(1, 0x00000),
        .map_bases.plane_d = (uint32_t)VDP2_VRAM_ADDR(1, 0x00000),
    },
    {
        .scroll_screen = VDP2_SCRN_NBG3,
        .cc_count = VDP2_SCRN_CCC_PALETTE_16,
        .character_size = 1 * 1,
        .pnd_size = 1,
        .auxiliary_mode = 0,
        .plane_size = 1 * 1,
        .cp_table = (uint32_t)VDP2_VRAM_ADDR(0, 0x00000),
        .color_palette = (uint32_t)VDP2_CRAM_MODE_1_OFFSET(0, 0, 0),
        .map_bases.plane_a = (uint32_t)VDP2_VRAM_ADDR(1, 0x08000),
        .map_bases.plane_b = (uint32_t)VDP2_VRAM_ADDR(1, 0x08000),
        .map_bases.plane_c = (uint32_t)VDP2_VRAM_ADDR(1, 0x08000),
        .map_bases.plane_d = (uint32_t)VDP2_VRAM_ADDR(1, 0x08000),
    }

};

static const vdp2_scrn_ls_format_t _ls_format_bg = {
    .scroll_screen = VDP2_SCRN_NBG0,
    .line_scroll_table = BG_LINE_SCROLL,
    .interval = 0,
    .enable = VDP2_SCRN_LS_HORZ};

static const vdp2_scrn_ls_format_t _ls_format_fg = {
    .scroll_screen = VDP2_SCRN_NBG1,
    .line_scroll_table = FG_LINE_SCROLL,
    .interval = 0,
    .enable = VDP2_SCRN_LS_HORZ};

const vdp2_scrn_vcs_format_t _vs_format_bg = {
    .scroll_screen = VDP2_SCRN_NBG0,
    .vcs_table = VCELL_SCROLL,
};
const vdp2_scrn_vcs_format_t _vs_format_fg = {
    .scroll_screen = VDP2_SCRN_NBG1,
    .vcs_table = VCELL_SCROLL,
};

//=========================================================
// Tilemap
//=========================================================

// http://md.railgun.works/index.php?title=VDP#Patterns
// https://segaretro.org/Sega_Mega_Drive/Planes

static void _setTileMap(const uint8_t *tilemap, uint32_t *pages)
{
    const uint32_t screen_pal_adr = format[0].color_palette;
    const uint32_t screen_cpd_adr = format[0].cp_table;

    const uint16_t v = *((uint16_t *)tilemap);

    const uint16_t tile_idx = v & 0x7FF;
    const uint8_t pal_idx = (v >> 13) & 0x3;

    const uint32_t pal_adr = (uint32_t)(screen_pal_adr) + (pal_idx * 32 * 2);
    const uint32_t cpd_adr = (uint32_t)(screen_cpd_adr) + (tile_idx << 5);

    const uint8_t y_flip = (v & 0x1000) != 0;
    const uint8_t x_flip = (v & 0x0800) != 0;

    const uint8_t prio = (v >> 15) ? 1 : 0;

    const uint32_t pnd = VDP2_SCRN_PND_CONFIG_8(1, cpd_adr, pal_adr, y_flip, x_flip, prio, 0);

    *pages = pnd;
    // duplicate the tile a 2nd time
    pages[1 << 11] = pnd;
}

void SetTileMap(const uint8_t *tilemap, size_t offset)
{
    const int is_bg = ((offset >> 13) == 7);
    uint32_t *pages = is_bg ? (uint32_t *)format[1].map_bases.plane_a : (uint32_t *)format[0].map_bases.plane_a;
    pages += ((offset & 0x1FFF) >> 1);
    _setTileMap(tilemap, pages);
}

//
void CopyTilemap(const uint8_t *tilemap, size_t offset, size_t width, size_t height)
{
    const uint32_t page_width = VDP2_SCRN_CALCULATE_PAGE_WIDTH(&format[0]);
    const int is_bg = ((offset >> 13) == 7);

    uint32_t *pages = is_bg ? (uint32_t *)format[1].map_bases.plane_a : (uint32_t *)format[0].map_bases.plane_a;
    pages += ((offset & 0x1FFF) >> 1);

    for (uint32_t page_y = 0; page_y < height; page_y++)
    {
        for (uint32_t page_x = 0; page_x < width; page_x++)
        {
            uint16_t page_idx = page_x + (page_width * page_y);
            _setTileMap(tilemap, &pages[page_idx]);
            tilemap += 2;
        }
    }
}

//=========================================================
// Palettes
//=========================================================

static uint16_t MD_TO_SS_PALETTE(uint16_t cv)
{
    static const uint8_t col_level[] = {0, 6, 10, 14, 18, 21, 25, 31};

    uint8_t r = (cv & 0x00E) >> 1;
    uint8_t g = (cv & 0x0E0) >> 5;
    uint8_t b = (cv & 0xE00) >> 9;

    return COLOR_RGB_DATA | (col_level[b] << 10) | (col_level[g] << 5) | col_level[r];
}

static uint8_t background_color_idx = 0;
void VDP_SetBackgroundColour(uint8_t index)
{
    background_color_idx = index;
}

void sync_palettes()
{
    // https://segaretro.org/Sega_Mega_Drive/Palettes_and_CRAM

    uint32_t screen_pal_adr = format[0].color_palette;
    extern uint16_t dry_palette[4][16];
    // sync palette
    uint16_t *color_palette = (uint16_t *)screen_pal_adr;
    // *color_palette++ = COLOR_RGB_DATA | COLOR_RGB888_TO_RGB555(0, 0, 255);
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            uint16_t cv = dry_palette[i][j];
            //*color_palette++ = COLOR_RGB_DATA | (col_level[b] << 10) | (col_level[g] << 5) | col_level[r];
            color_palette[(i * 32) + j] = MD_TO_SS_PALETTE(cv);
        }
    }

    // Update back color
    uint16_t *back_color = (uint16_t *)VDP2_VRAM_ADDR(3, 0x01FFFE);
    uint16_t *pal = (uint16_t *)dry_palette;

    *back_color = MD_TO_SS_PALETTE(pal[background_color_idx]);

    vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
                                    (color_rgb1555_t)(*back_color));
}

//=========================================================
// Scroll
//=========================================================
void update_scroll()
{
    int16_t *scroll = &hscroll_buffer[0][0];
    int32_t *fg_dst = (int32_t *)(FG_LINE_SCROLL);
    int32_t *bg_dst = (int32_t *)(BG_LINE_SCROLL);
    for (int i = 0; i < 224; i++)
    {
        // for (int k = 0; k < 8; k++)
        {
            *bg_dst++ = ((-*scroll++) << 16);
            *fg_dst++ = ((-*scroll++) << 16);
        }
    }
}

void init_vdp2()
{
    vdp2_vram_cycp_t vram_cycp;

    vram_cycp.pt[0].t0 = VDP2_VRAM_CYCP_PNDR_NBG0;
    vram_cycp.pt[0].t1 = VDP2_VRAM_CYCP_PNDR_NBG0;
    vram_cycp.pt[0].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[0].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[0].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG1;
    vram_cycp.pt[0].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG1;
    vram_cycp.pt[0].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG2;
    vram_cycp.pt[0].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG2;

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

    vdp2_scrn_cell_format_set(&format[0]);
    vdp2_scrn_cell_format_set(&format[1]);
    //vdp2_scrn_cell_format_set(&format[2]);
    //vdp2_scrn_cell_format_set(&format[3]);

    vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 3);
    vdp2_scrn_priority_set(VDP2_SCRN_NBG1, 3);
    vdp2_scrn_priority_set(VDP2_SCRN_SPRITE, 2);
    // vdp2_scrn_priority_set(VDP2_SCRN_NBG2, 1);
    // vdp2_scrn_priority_set(VDP2_SCRN_NBG3, 0);

    vdp2_sprite_priority_set(0, 3);
    vdp2_sprite_priority_set(1, 1);
    vdp2_sprite_priority_set(2, 2);
    vdp2_sprite_priority_set(3, 3);
    vdp2_sprite_priority_set(4, 4);
    vdp2_sprite_priority_set(5, 5);
    vdp2_sprite_priority_set(6, 6);
    vdp2_sprite_priority_set(7, 7);

    vdp2_scrn_display_set(VDP2_SCRN_NBG0, true);
    vdp2_scrn_display_set(VDP2_SCRN_NBG1, true);
    // vdp2_scrn_display_set(VDP2_SCRN_NBG2, true);
    //vdp2_scrn_display_set(VDP2_SCRN_NBG3, true);

    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
                              VDP2_TVMD_VERT_224);
    vdp2_tvmd_display_set();

    vdp2_scrn_ls_set(&_ls_format_bg);
    vdp2_scrn_ls_set(&_ls_format_fg);
    // vdp2_scrn_vcs_set(&_vs_format_bg);
    // vdp2_scrn_vcs_set(&_vs_format_fg);
}

void vblank_out_handler(void *work __unused)
{
    smpc_peripheral_intback_issue();
}
