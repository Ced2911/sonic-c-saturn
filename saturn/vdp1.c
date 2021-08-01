#include "saturn.h"
#include "Video.h"

#define ORDER_SYSTEM_CLIP_COORDS_INDEX (0)
#define ORDER_LOCAL_COORDS_INDEX (1)
#define ORDER_SPRITE_START_INDEX (2)

static vdp1_cmdt_list_t *_cmdt_list = NULL;
static uint8_t *vdp1_pal_addr = NULL;
vdp1_vram_partitions_t vdp1_vram_partitions;

#define SCANLINE_SPRITES 20
static struct VDP_SpriteCache
{
    const uint16_t *sprite[SCANLINE_SPRITES];
    uint8_t pushind;
    uint16_t pixels;
} vdp_sprite_cache[SCREEN_HEIGHT];

// Sprites - @Todo pas optimum...
void draw_sprites()
{
    vdp1_cmdt_t *cmdts = &_cmdt_list->cmdts[ORDER_SPRITE_START_INDEX];
    vdp1_cmdt_t *vdp1_spr = cmdts;

    size_t n_spr = 0;
    uint16_t *sprite_16 = &sprite_buffer[0][0];
    uint8_t *sprite_addr = (uint8_t *)sprite_16;
    //uint8_t *sprite_addr = &vdp_vram[VRAM_SPRITES];

    static const vdp1_cmdt_draw_mode_t draw_mode = {
        .raw = 0x0000,
        .bits.color_mode = 1,
        .bits.trans_pixel_disable = false,
        .bits.pre_clipping_disable = true,
        .bits.end_code_disable = true};

    vdp1_cmdt_color_bank_t color_bank = {.raw = 0};

#if 1
    uint16_t sprite_cnt_per_ligne[224];
    uint16_t *sprite_lut[128];
    uint16_t sprite_cnt = 0;
    uint8_t sprite_link = 0;

    memset(sprite_cnt_per_ligne, 0, sizeof(sprite_cnt_per_ligne));
    memset(sprite_lut, 0, sizeof(sprite_lut));

    // Build list of sprites
    do
    {
        const uint16_t *sprite = (const uint16_t *)sprite_buffer[sprite_link];
        uint16_t sprite_sl = sprite[1];
        //    uint8_t sprite_width = (sprite_sl & SPRITE_SL_W_AND) >> SPRITE_SL_W_SHIFT;
        //    uint8_t sprite_height = (sprite_sl & SPRITE_SL_H_AND) >> SPRITE_SL_H_SHIFT;
        sprite_link = (sprite_sl & SPRITE_SL_L_AND) >> SPRITE_SL_L_SHIFT;
        // sprite_cnt_per_ligne[sprite_y]++;
        // if (sprite_cnt_per_ligne[sprite_y] <= SCANLINE_SPRITES)
        sprite_lut[sprite_cnt++] = sprite;
    } while (sprite_link != 0);

    // Work in reverse order
    for (uint8_t i = 0; i < sprite_cnt; i++)
    {
        const uint16_t *sprite = sprite_lut[sprite_cnt - i - 1];

        //Get sprite values
        uint16_t sprite_y = sprite[0];
        uint16_t sprite_x = sprite[3];
        uint16_t sprite_sl = sprite[1];
        uint8_t sprite_width = (sprite_sl & SPRITE_SL_W_AND) >> SPRITE_SL_W_SHIFT;
        uint8_t sprite_height = (sprite_sl & SPRITE_SL_H_AND) >> SPRITE_SL_H_SHIFT;
        uint8_t sprite_link = (sprite_sl & SPRITE_SL_L_AND) >> SPRITE_SL_L_SHIFT;
        uint8_t sprite_pal = (sprite[2] >> 13) & 3;
        uint16_t sprite_tile = (sprite[2] & 0x7FF);

        uint8_t sprite_flip_y = (sprite[2] & 0x1000) != 0;
        uint8_t sprite_flip_x = (sprite[2] & 0x0800) != 0;
        uint8_t y = 0;
        for (uint8_t x = 0; x < (sprite_width + 1); x++)
        {
            uint8_t rel_x = sprite_flip_x ? (sprite_width)-x : x;
            uint8_t rel_y = y;

            uint32_t sp_offset = y + (x * (sprite_height + 1));

            uint32_t tex_addr = (uint32_t)vdp1_vram_partitions.texture_base + ((sprite_tile + sp_offset) << 5);

            int16_vec2_t xy = {
                .x = sprite_x + (rel_x << 3),
                .y = sprite_y + (rel_y << 3)};

            // skip some sprites...
            //if (sprite_x > 128)
            {
                //color_bank.raw = sprite_pal << 5;

                color_bank.type_1.data.pr = (sprite[2] >> 15) ? 3 : 2;
                color_bank.type_1.data.dc = sprite_pal << 5;

                vdp1_cmdt_normal_sprite_set(vdp1_spr);
                vdp1_cmdt_param_vertices_set(vdp1_spr, &xy);
                vdp1_cmdt_param_draw_mode_set(vdp1_spr, draw_mode);
                vdp1_cmdt_param_color_mode0_set(vdp1_spr, color_bank);
                vdp1_cmdt_param_size_set(vdp1_spr, 8, 8 * (sprite_height + 1));
                vdp1_cmdt_param_char_base_set(vdp1_spr, tex_addr);
                vdp1_cmdt_param_horizontal_flip_set(vdp1_spr, sprite_flip_x);
                vdp1_cmdt_param_vertical_flip_set(vdp1_spr, sprite_flip_y);

                n_spr++;
                vdp1_spr++;
            }
        }
    }

#else
    for (uint8_t i = 0;;)
    {
        const uint16_t *sprite = (const uint16_t *)sprite_buffer[i];

        //Get sprite values
        uint16_t sprite_y = sprite[0];
        uint16_t sprite_x = sprite[3];
        uint16_t sprite_sl = sprite[1];
        uint8_t sprite_width = (sprite_sl & SPRITE_SL_W_AND) >> SPRITE_SL_W_SHIFT;
        uint8_t sprite_height = (sprite_sl & SPRITE_SL_H_AND) >> SPRITE_SL_H_SHIFT;
        uint8_t sprite_link = (sprite_sl & SPRITE_SL_L_AND) >> SPRITE_SL_L_SHIFT;
        uint8_t sprite_pal = (sprite[2] >> 13) & 3;
        uint16_t sprite_tile = (sprite[2] & 0x7FF);

        uint8_t sprite_flip_y = (sprite[2] & 0x1000) != 0;
        uint8_t sprite_flip_x = (sprite[2] & 0x0800) != 0;

        //Write sprite
        for (uint8_t y = 0; y < (sprite_height + 1); y++)
        {
            for (uint8_t x = 0; x < (sprite_width + 1); x++)
            {
                uint32_t sp_offset = y + (x * (sprite_height + 1));
                uint32_t tex_addr = (uint32_t)vdp1_vram_partitions.texture_base + ((sprite_tile + sp_offset) * 0x20);

                int16_vec2_t xy = {
                    .x = sprite_x + (x << 3),
                    .y = sprite_y + (y << 3)};

                // skip some sprites...
                if (sprite_x > 128)
                {
                    //color_bank.raw = sprite_pal << 5;

                    color_bank.type_1.data.pr = (sprite[2] >> 15) ? 3 : 2;
                    color_bank.type_1.data.dc = sprite_pal << 5;

                    vdp1_cmdt_normal_sprite_set(vdp1_spr);
                    vdp1_cmdt_param_vertices_set(vdp1_spr, &xy);
                    vdp1_cmdt_param_draw_mode_set(vdp1_spr, draw_mode);
                    vdp1_cmdt_param_color_mode0_set(vdp1_spr, color_bank);
                    vdp1_cmdt_param_size_set(vdp1_spr, 8, 8);
                    vdp1_cmdt_param_char_base_set(vdp1_spr, tex_addr);
                    vdp1_cmdt_param_horizontal_flip_set(vdp1_spr, sprite_flip_x);
                    vdp1_cmdt_param_vertical_flip_set(vdp1_spr, sprite_flip_y);

                    n_spr++;
                    vdp1_spr++;
                }
            }
        }

        //Go to next sprite
        if (sprite_link != 0)
            i = sprite_link;
        else
            break;
    }
#endif

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

    const vdp1_env_t vdp1_env = {
        .erase_color = COLOR_RGB1555_INITIALIZER(0, 0, 0, 0),
        .erase_points[0] = {
            0,
            0},
        .erase_points[1] = {SCREEN_WIDTH, SCREEN_HEIGHT},
        .bpp = VDP1_ENV_BPP_16,
        .rotation = VDP1_ENV_ROTATION_0,
        .color_mode = VDP1_ENV_COLOR_MODE_RGB_PALETTE,
        .sprite_type = 0x01};
    vdp1_env_set(&vdp1_env);

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