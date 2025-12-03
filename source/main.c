#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>

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

    // Create a console
    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    printf("Hello World!\n");

    // Wait until HOME button is pressed
    while (1) {
        WPAD_ScanPads();

        u32 pressed = WPAD_ButtonsDown(0);

        if (pressed & WPAD_BUTTON_HOME) {
            break;
        }

        // Wait for next frame
        VIDEO_WaitVSync();
    }

    // Set the screen to black when exiting
    VIDEO_SetBlack(true);
    VIDEO_Flush();

    return 0;
}