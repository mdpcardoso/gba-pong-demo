
#include <tonc_bios.h>
#include <tonc_input.h>
#include <tonc_irq.h>
#include <tonc_memdef.h>
#include <tonc_memmap.h>
#include <tonc_oam.h>
#include <tonc_types.h>
#include <tonc_video.h>

#include "ball.h"
#include "paddle.h"

#define BORDER 8

OBJ_ATTR obj_buffer[2];
OBJ_AFFINE *const obj_aff_buffer = (OBJ_AFFINE *)obj_buffer;

struct sprite_t {
  u32 width, height;
  u32 x, y;
  u32 x_speed, y_speed;
  u32 pal_bank;
  OBJ_ATTR *oam_buffer;
};

//---------------------------------------------------------------------------------
// Program entry point
//---------------------------------------------------------------------------------
int main(void) {
  //---------------------------------------------------------------------------------
  // Check if sizes are half-words. (Verify both memcpy16)
  memcpy16(pal_obj_mem, paddlePal, paddlePalLen / 2);
  memcpy16(&tile_mem[4][1], paddleTiles, paddleTilesLen / 2);
  memcpy16(&tile_mem[4][5], ballTiles, ballTilesLen / 2);

  // Need to memset bg even if disabled?
  memset16(pal_bg_mem, 0, PAL_BG_SIZE / 2);

  oam_init(obj_buffer, 2);

  struct sprite_t ball = {8, 8, 120, 80, 2, 2, 0, &obj_buffer[1]};

  ball.oam_buffer->attr0 =
      ATTR0_4BPP | ATTR0_SQUARE |
      ATTR0_Y(ball.y); // 4bpp tiles, SQUARE shape, at y coord 0
  ball.oam_buffer->attr1 =
      ATTR1_SIZE_8x8 | ATTR1_X(ball.x); // 8x8 size when using the SQUARE shape
  ball.oam_buffer->attr2 =
      ATTR2_PALBANK(ball.pal_bank) | 5; // Start at [4][5] (4-bit spacing)

  struct sprite_t paddle = {8, 32, BORDER, BORDER, 2, 2, 0, &obj_buffer[0]};

  paddle.oam_buffer->attr0 =
      ATTR0_4BPP | ATTR0_TALL |
      ATTR0_Y(paddle.y); // 4bpp tiles, WIDE shape, at y coord 0
  paddle.oam_buffer->attr1 =
      ATTR1_SIZE_8x32 |
      ATTR1_X(paddle.x); // 64x32 size when using the WIDE shape
  paddle.oam_buffer->attr2 =
      ATTR2_PALBANK(paddle.pal_bank) | 1; // Start at [4][1] (4-bit spacing)

  REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D; // Mode 0, no background

  // the vblank interrupt must be enabled for VBlankIntrWait() to work
  // since the default dispatcher handles the bios flags no vblank handler
  // is required
  IRQ_INIT();
  IRQ_SET(VBLANK);

  while (1) {
    key_poll();

    if (ball.y + ball.y_speed + ball.height >= SCREEN_HEIGHT - BORDER ||
        ball.y + ball.y_speed <= BORDER) {
      ball.y_speed *= -1;
    }

    if (ball.x + ball.x_speed + ball.width >= SCREEN_WIDTH - BORDER) {
      ball.x_speed *= -1;
    } else if (ball.x + ball.x_speed <= paddle.width + BORDER) {

      if (ball.y < paddle.y + paddle.height &&
          ball.y > paddle.y - ball.height) {

        paddle.pal_bank = paddle.pal_bank ? 0 : 1;
        paddle.oam_buffer->attr2 = ATTR2_PALBANK(paddle.pal_bank) | 1;
      } else {
        ball.x = 120;
        ball.y = 80;
      }
      ball.x_speed *= -1;
    }

    obj_set_pos(ball.oam_buffer, ball.x += ball.x_speed,
                ball.y += ball.y_speed);

    if (key_is_down(KEY_DOWN) &&
        (paddle.y + paddle.y_speed <= SCREEN_HEIGHT - BORDER - paddle.height)) {
      paddle.y += paddle.y_speed;
    } else if (key_is_down(KEY_UP) && (paddle.y >= paddle.y_speed + BORDER)) {
      paddle.y -= paddle.y_speed;
    }

    if (key_is_down(KEY_UP | KEY_DOWN)) {
      obj_set_pos(paddle.oam_buffer, paddle.x, paddle.y);
    }

    if (key_hit(KEY_START)) {
      paddle.pal_bank = paddle.pal_bank ? 0 : 1;
      paddle.oam_buffer->attr2 = ATTR2_PALBANK(paddle.pal_bank) | 1;
    }

    VBlankIntrWait();

    oam_copy(oam_mem, obj_buffer, 2); // Buffer the OAM copy
  }
}

