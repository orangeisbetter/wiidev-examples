#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>

#include "textures.h"
#include "textures_tpl.h"

#define FIFO_SIZE (256 * 1024)

#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 480

u32 colors[] = {
    0xff0000ff,
    0xff7f00ff,
    0xffff00ff,
    0x7fff00ff,
    0x00ff00ff,
    0x00ff7fff,
    0x00ffffff,
    0x007fffff,
    0x0000ffff,
    0x7f00ffff,
    0xff00ffff,
    0xff007fff,
};

#define NUM_COLORS (sizeof(colors) / sizeof(*colors))

int main() {
    VIDEO_Init();
    WPAD_Init();

    // Set up video
    GXRModeObj* rmode = VIDEO_GetPreferredMode(NULL);
    void* xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);
    VIDEO_SetBlack(false);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_Flush();

    // Initialize GX
    void* fifoBuffer = MEM_K0_TO_K1(memalign(32, FIFO_SIZE));
    memset(fifoBuffer, 0, FIFO_SIZE);

    GX_Init(fifoBuffer, FIFO_SIZE);
    GX_SetCopyClear((GXColor){ 0x00, 0x00, 0x00, 0xff }, 0x00ffffff);
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetDispCopyYScale((f32)rmode->xfbHeight / (f32)rmode->efbHeight);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering, (rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE);

    GX_SetCullMode(GX_CULL_NONE);
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);

    // Enable blending
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);

    // Create 3D projection matrix
    Mtx44 projection;
    guOrtho(projection, 0, SCREEN_HEIGHT, 0, SCREEN_WIDTH, 0, 100);

    GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);

    // Load texture
    TPLFile textureTPL;
    TPL_OpenTPLFromMemory(&textureTPL, (void*)textures_tpl, textures_tpl_size);

    GXTexObj dvdLogo;
    TPL_GetTexture(&textureTPL, dvd_logo_id, &dvdLogo);

    GX_LoadTexObj(&dvdLogo, GX_TEXMAP0);

    // Set up vertex attributes
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetNumChans(1);
    GX_SetNumTexGens(1);

    // Set up TEV registers for rendering
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

    // Synchronize frames
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    int x = 0;
    int y = 0;
    int w = 150;
    int h = 75;

    int vx = 1;
    int vy = 1;

    int colorIdx = 0;

    while (1) {
        WPAD_ScanPads();

        u32 pressed = WPAD_ButtonsDown(0);

        if (pressed & WPAD_BUTTON_HOME) {
            break;
        }

        // Update positions
        x += vx;
        y += vy;

        // Bounce
        if (x < 0) {
            x = 0;
            vx = -vx;
            colorIdx++;
        }
        if (y < 0) {
            y = 0;
            vy = -vy;
            colorIdx++;
        }
        if (x + w > SCREEN_WIDTH) {
            x = SCREEN_WIDTH - w;
            vx = -vx;
            colorIdx++;
        }
        if (y + h > SCREEN_HEIGHT) {
            y = SCREEN_HEIGHT - h;
            vy = -vy;
            colorIdx++;
        }

        if (colorIdx >= NUM_COLORS) {
            colorIdx = 0;
        }

        // Set up view matrix
        guVector camera = { 0.0f, 0.0f, 0.0f };
        guVector up = { 0.0f, 1.0f, 0.0f };
        guVector look = { 0.0f, 0.0f, -1.0f };

        Mtx view;
        guLookAt(view, &camera, &up, &look);

        // Set up model and modelview matrix
        Mtx model, modelView;
        guMtxIdentity(model);
        guMtxTransApply(model, model, x, y, -50.0f);
        guMtxConcat(view, model, modelView);

        u32 color = colors[colorIdx];

        GX_LoadPosMtxImm(modelView, GX_PNMTX0);

        GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

        GX_Position3s16(0, 0, 0);
        GX_Color1u32(color);
        GX_TexCoord2f32(0.0f, 0.0f);

        GX_Position3s16(w, 0, 0);
        GX_Color1u32(color);
        GX_TexCoord2f32(1.0f, 0.0f);

        GX_Position3s16(w, h, 0);
        GX_Color1u32(color);
        GX_TexCoord2f32(1.0f, 1.0f);

        GX_Position3s16(0, h, 0);
        GX_Color1u32(color);
        GX_TexCoord2f32(0.0f, 1.0f);

        GX_End();

        // Flush all commands to the GPU
        GX_DrawDone();

        // Wait for next frame
        VIDEO_WaitVSync();

        // Copy efb to xfb
        GX_CopyDisp(xfb, GX_TRUE);
    }

    // Set the screen to black when exiting
    VIDEO_SetBlack(true);
    VIDEO_Flush();

    return 0;
}