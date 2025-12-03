#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>

#define FIFO_SIZE (256 * 1024)

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

    // Allocate FIFO buffer (for graphics commands)
    void* fifoBuffer = MEM_K0_TO_K1(memalign(32, FIFO_SIZE));
    memset(fifoBuffer, 0, FIFO_SIZE);

    // Initialize GX subsystem
    GX_Init(fifoBuffer, FIFO_SIZE);
    GX_SetCopyClear((GXColor){ 0x00, 0x00, 0x00, 0xff }, 0x00ffffff);
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetDispCopyYScale((f32)rmode->xfbHeight / (f32)rmode->efbHeight);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering, (rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE);

    // Set render settings
    GX_SetCullMode(GX_CULL_NONE);
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);

    // Create 3D projection matrix
    Mtx44 projection;
    guPerspective(projection, 60, 16.0f / 9.0f, 10.0f, 300.0f);
    GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);

    // Set up vertex attributes
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_RGB8, 0);
    GX_SetNumChans(1);
    GX_SetNumTexGens(0);

    // Set up TEV registers for rendering
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    // Synchronize frames
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    // Wait until HOME button is pressed
    while (1) {
        WPAD_ScanPads();

        u32 pressed = WPAD_ButtonsDown(0);

        if (pressed & WPAD_BUTTON_HOME) {
            break;
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
        guMtxTransApply(model, model, 0.0f, 0.0f, -50.0f);
        guMtxConcat(view, model, modelView);

        GX_LoadPosMtxImm(modelView, GX_PNMTX0);

        // Draw the triangle primitive
        GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);

        GX_Position3s16(0, 15, 0);
        GX_Color3u8(0xff, 0x00, 0x00);

        GX_Position3s16(15, -15, 0);
        GX_Color3u8(0x00, 0xff, 0x00);

        GX_Position3s16(-15, -15, 0);
        GX_Color3u8(0x00, 0x00, 0xff);

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