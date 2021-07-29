#include "saturn.h"
#include "Video.h"

#define ORDER_SYSTEM_CLIP_COORDS_INDEX (0)
#define ORDER_LOCAL_COORDS_INDEX (1)
#define ORDER_SPRITE_START_INDEX (2)

static vdp1_cmdt_list_t *_cmdt_list = NULL;
static uint8_t *vdp1_pal_addr = NULL;
vdp1_vram_partitions_t vdp1_vram_partitions;

// Sprites - @Todo pas optimum...
void draw_sprites()
{
    vdp1_cmdt_t *cmdts = &_cmdt_list->cmdts[ORDER_SPRITE_START_INDEX];
    vdp1_cmdt_t *vdp1_spr = cmdts;

    size_t n_spr = 0;
    uint16_t *sprite_16 = &sprite_buffer[0][0];
    // uint8_t *sprite_addr = (uint8_t *)sprite_16;
    uint8_t *sprite_addr = &vdp_vram[VRAM_SPRITES];

    static const vdp1_cmdt_draw_mode_t draw_mode = {
        .raw = 0x0000,
        .bits.color_mode = 1,
        .bits.trans_pixel_disable = false,
        .bits.pre_clipping_disable = true,
        .bits.end_code_disable = true};

    vdp1_cmdt_color_bank_t color_bank = {.raw = 0};

    for (uint8_t i = 0;;)
    {
        const uint16_t *sprite = sprite_buffer[i];

        //Get sprite values
        uint16_t sprite_y = sprite[0];
        uint16_t sprite_x = sprite[3];
        uint16_t sprite_sl = sprite[1];
        uint8_t sprite_width = (sprite_sl & SPRITE_SL_W_AND) >> SPRITE_SL_W_SHIFT;
        uint8_t sprite_height = (sprite_sl & SPRITE_SL_H_AND) >> SPRITE_SL_H_SHIFT;
        uint8_t sprite_link = (sprite_sl & SPRITE_SL_L_AND) >> SPRITE_SL_L_SHIFT;
        uint8_t sprite_pal = (sprite[2] >> 13) & 3;

        uint16_t sprite_tile = (sprite[2] & 0x7FF);

        //Write sprite
        for (uint8_t x = 0; x < (sprite_width + 1); x++)
        {
            for (uint8_t y = 0; y < (sprite_height + 1); y++)
            {
                uint32_t tex_addr = (uint32_t)vdp1_vram_partitions.texture_base + (sprite_tile * 0x20);

                int16_vec2_t xy = {
                    .x = sprite_x + (x << 3),
                    .y = sprite_y + (y << 3)};

                color_bank.raw = sprite_pal << 5;

                vdp1_cmdt_normal_sprite_set(vdp1_spr);
                vdp1_cmdt_param_vertices_set(vdp1_spr, &xy);
                vdp1_cmdt_param_draw_mode_set(vdp1_spr, draw_mode);
                vdp1_cmdt_param_color_mode0_set(vdp1_spr, color_bank);
                vdp1_cmdt_param_size_set(vdp1_spr, 8, 8);
                vdp1_cmdt_param_char_base_set(vdp1_spr, tex_addr);

                vdp1_spr++;
                sprite_tile++;
                n_spr++;
            }
        }

        //Go to next sprite
        if (sprite_link != 0)
            i = sprite_link;
        else
            break;
    }

    vdp1_cmdt_end_set(&cmdts[n_spr]);
    _cmdt_list->count = n_spr + ORDER_SPRITE_START_INDEX;
    vdp1_sync_cmdt_list_put(_cmdt_list, 0, NULL, NULL);
}

void vdp1_clear_cmdt()
{
    vdp1_cmdt_t *cmdt = &_cmdt_list->cmdts[0];
    vdp1_cmdt_end_set(&cmdt[ORDER_SPRITE_START_INDEX]);
    _cmdt_list->count = ORDER_SPRITE_START_INDEX;
    vdp1_sync_cmdt_list_put(_cmdt_list, 0, NULL, NULL);
    // mandatory or freeze
    vdp_sync();
}

void init_vdp1()
{
    vdp1_vram_partitions_get(&vdp1_vram_partitions);
    vdp1_pal_addr = (uint8_t *)VDP2_CRAM_MODE_1_OFFSET(0, 1, 0x0000);

    _cmdt_list = vdp1_cmdt_list_alloc(2000);

    vdp1_cmdt_t *cmdt;
    cmdt = &_cmdt_list->cmdts[0];
    /*
    static const int16_vec2_t local_coords =
        INT16_VEC2_INITIALIZER(SCREEN_WIDTH / 2,
                               SCREEN_HEIGHT / 2);
*/

    static const int16_vec2_t local_coords = {.x = -128, .y = -128};

    static const int16_vec2_t system_clip_coords =
        INT16_VEC2_INITIALIZER(SCREEN_WIDTH,
                               SCREEN_HEIGHT);

    vdp1_cmdt_system_clip_coord_set(&cmdt[ORDER_SYSTEM_CLIP_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdt[ORDER_SYSTEM_CLIP_COORDS_INDEX],
                               CMDT_VTX_SYSTEM_CLIP, &system_clip_coords);

    vdp1_cmdt_local_coord_set(&cmdt[ORDER_LOCAL_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdt[ORDER_LOCAL_COORDS_INDEX],
                               CMDT_VTX_LOCAL_COORD, &local_coords);

    vdp1_cmdt_end_set(&cmdt[ORDER_SPRITE_START_INDEX]);
    _cmdt_list->count = ORDER_SPRITE_START_INDEX;
}