// ----------------------------------- framebf.c -------------------------------------
#include "mbox.h"
#include "../uart/uart.h"
#include "font.h"

//Use RGBA32 (32 bits for each pixel)
#define COLOR_DEPTH 32
//Pixel Order: BGR in memory order (little endian --> RGB in byte order)
#define PIXEL_ORDER 0
//Screen info
unsigned int width, height, pitch;
/* Frame buffer address
* (declare as pointer of unsigned char to access each byte) */
unsigned char *fb;
/**
* Set screen resolution to 1024x768
*/
void framebf_init()
{
    mBuf[0] = 35*4; // Length of message in bytes
    mBuf[1] = MBOX_REQUEST;
    mBuf[2] = MBOX_TAG_SETPHYWH; //Set physical width-height
    mBuf[3] = 8; // Value size in bytes
    mBuf[4] = 0; // REQUEST CODE = 0
    mBuf[5] = 1024; // Value(width)
    mBuf[6] = 768; // Value(height)
    mBuf[7] = MBOX_TAG_SETVIRTWH; //Set virtual width-height
    mBuf[8] = 8;
    mBuf[9] = 0;
    mBuf[10] = 1024;
    mBuf[11] = 768;
    mBuf[12] = MBOX_TAG_SETVIRTOFF; //Set virtual offset
    mBuf[13] = 8;
    mBuf[14] = 0;
    mBuf[15] = 0; // x offset
    mBuf[16] = 0; // y offset
    mBuf[17] = MBOX_TAG_SETDEPTH; //Set color depth
    mBuf[18] = 4;
    mBuf[19] = 0;
    mBuf[20] = COLOR_DEPTH; //Bits per pixel
    mBuf[21] = MBOX_TAG_SETPXLORDR; //Set pixel order
    mBuf[22] = 4;
    mBuf[23] = 0;
    mBuf[24] = PIXEL_ORDER;
    mBuf[25] = MBOX_TAG_GETFB; //Get frame buffer
    mBuf[26] = 8;
    mBuf[27] = 0;
    mBuf[28] = 16; //alignment in 16 bytes
    mBuf[29] = 0; //will return Frame Buffer size in bytes
    mBuf[30] = MBOX_TAG_GETPITCH; //Get pitch
    mBuf[31] = 4;
    mBuf[32] = 0;
    mBuf[33] = 0; //Will get pitch value here
    mBuf[34] = MBOX_TAG_LAST;
    // Call Mailbox
    if (mbox_call(ADDR(mBuf), MBOX_CH_PROP) //mailbox call is successful ?
        && mBuf[20] == COLOR_DEPTH //got correct color depth ?
        && mBuf[24] == PIXEL_ORDER //got correct pixel order ?
        && mBuf[28] != 0 //got a valid address for frame buffer ?
    ) {
    /* Convert GPU address to ARM address (clear higher address bits)
    * Frame Buffer is located in RAM memory, which VideoCore MMU
    * maps it to bus address space starting at 0xC0000000.
    * Software accessing RAM directly use physical addresses
    * (based at 0x00000000)
    */
        mBuf[28] &= 0x3FFFFFFF;
        // Access frame buffer as 1 byte per each address
        fb = (unsigned char *)((unsigned long)mBuf[28]);
        uart_puts("Got allocated Frame Buffer at RAM physical address: ");
        uart_hex(mBuf[28]);
        uart_puts("\n");
        uart_puts("Frame Buffer Size (bytes): ");
        uart_dec(mBuf[29]);
        uart_puts("\n");
        width = mBuf[5]; // Actual physical width
        height = mBuf[6]; // Actual physical height
        pitch = mBuf[33]; // Number of bytes per line
    } else {
        uart_puts("Unable to get a frame buffer with provided setting\n");
    }
}
void drawPixelARGB32(int x, int y, unsigned int attr)
{
    int offs = (y * pitch) + (COLOR_DEPTH/8 * x);
    /* //Access and assign each byte
    *(fb + offs ) = (attr >> 0 ) & 0xFF; //BLUE
    *(fb + offs + 1) = (attr >> 8 ) & 0xFF; //GREEN
    *(fb + offs + 2) = (attr >> 16) & 0xFF; //RED
    *(fb + offs + 3) = (attr >> 24) & 0xFF; //ALPHA
    */
    //Access 32-bit together
    *((unsigned int*)(fb + offs)) = attr;
}

void drawRectARGB32(int x1, int y1, int x2, int y2, unsigned int attr, int fill)
{
    for (int y = y1; y <= y2; y++ )
    for (int x = x1; x <= x2; x++) {
    if ((x == x1 || x == x2) || (y == y1 || y == y2))
        drawPixelARGB32(x, y, attr);
    else if (fill)
        drawPixelARGB32(x, y, attr);
    }
}

// void drawPixel(int x, int y, unsigned char attr)
// {
//     int offs = (y * pitch) + (x * 4);
//     *((unsigned int*)(fb + offs)) = vgapal[attr & 0x0f];
// }

// void drawStringWelcome(int x, int y, char *s, unsigned char attr)
// {
//     while (*s) {
//        if (*s == '\r') {
//           x = 0;
//        } else if(*s == '\n') {
//           x = 0; y += FONT_HEIGHT;
//        } else {
// 	  drawCharWelcome(*s, x, y, attr);
//           x += FONT_WIDTH;
//        }
//        s++;
//     }
// }

// void drawChar32x32(unsigned char ch, int x, int y, unsigned char attr)
// {
//     unsigned char *glyph = (unsigned char *)&font + (ch < FONT_NUMGLYPHS ? ch : 0) * FONT_BPG;

//     for (int i=0;i<FONT_HEIGHT;i++) {
// 	for (int j=0;j<FONT_WIDTH;j++) {
// 	    unsigned char mask = 1 << j;
// 	    unsigned char col = (*glyph & mask) ? attr & 0x0f : 0x00;

// 	    drawPixel(x+2*j, y+2*i, col);
//         drawPixel(x+2*j, y+2*i + 1, col);
//        	drawPixel(x+2*j +1, y+2*i, col);
//        	drawPixel(x+2*j+1, y+2*i+1, col);
// 	}
// 	glyph += FONT_BPL;
//     }
// }

// void drawString32x32(int x, int y, char *s, unsigned char attr)
// {
//     while (*s) {
//        if (*s == '\r') {
//           x = 0;
//        } else if(*s == '\n') {
//           x = 0; y += FONT_HEIGHT*2;
//        } else {
// 	  drawChar32x32(*s, x, y, attr);
//           x += FONT_WIDTH*2;
//        }
//        s++;
//     }
// }

// void drawChar(unsigned char ch, int x, int y, unsigned char attr)
// {
//     unsigned char *glyph = (unsigned char *)&font + (ch < FONT_NUMGLYPHS ? ch : 0) * FONT_BPG;

//     for (int i=0;i<FONT_HEIGHT;i++) {
// 	for (int j=0;j<FONT_WIDTH;j++) {
// 	    unsigned char mask = 1 << j;
// 	    unsigned char col = (*glyph & mask) ? attr & 0x0f : (attr & 0xf0) >> 4;

// 	    drawPixel(x+j, y+i, col);
// 	}
// 	glyph += FONT_BPL;
//     }
// }

void draw_imageChar(unsigned char ch, int x, int y, unsigned int attr)
{   
    int index = -1;
    if (ch >= 'A' && ch <= 'Z') {
        index = ch - 'A';
    } else if (ch >= 'a' && ch <= 'z') {
        index = 26 + (ch - 'a');  // 26 upper-case letters before this
    } else if (ch >= '0' && ch <= '9') {
        index = 52 + (ch - '0');  // 26 upper-case + 26 lower-case letters
    } else if (ch == ' '){
        return;
    }
    // } else {
    //     // For special characters; you'll need to define how to handle these
    //     switch (ch) {
    //         case ',':
    //             index = 62;  
    //             break;
    //         case ';':
    //             index = 63;
    //             break;
    //         case '!':
    //             index = 64;
    //             break;
    //         // Add more special characters here...
    //         case '?':
    //             index = 65;
    //             break;
    //         case '\'':
    //             index = 66;
    //             break;
    //         case '"':
    //             index = 67;
    //             break;
    //         case '(':
    //             index = 68;
    //             break;
    //         case ')':
    //             index = 69;
    //             break;
    //         case '@':
    //             index = 70;
    //             break;
    //         case ':':
    //             index = 71;
    //             break;
    //         case '$':
    //             index = 72;
    //             break;
    //         case '#':
    //             index = 73;
    //             break;
    //         case '%':
    //             index = 74;
    //             break;
    //         case '&':
    //             index = 75;
    //         default:
    //             return;  // Return if character not supported
    //     }
    // }

    if (index < 0 || index >= myBitmapallArray_LEN) {
        return;  // Character not found in the array
    }

    // Looping through image array line by line.
    for (int j = 0; j < 172; j++){
        // Looping through image array pixel by pixel of line j.
        for (int i = 0; i < 158; i++){
            //Printing each pixel in correct order of the array and lines, columns.
            unsigned int pixelValue = myBitmapallArray[index][j * 158 + i];
            // Assuming that a white pixel (0xFFFFFFFF) represents the letter and a black pixel (0xFF000000) represents the background
            if (pixelValue == 0x00000000) {
                // Change letter color based on the attr value passed
                pixelValue = attr;
                drawPixelARGB32(i/3 + x, j/3 + y, pixelValue);
            } else if (pixelValue == 0x00ffffff) {
                // Do nothing, effectively ignoring this pixel
                continue;
            }
        }
    }
}

void draw_ImageString(int x, int y, char *s, unsigned int attr) {
    int xOffset = 0;  // To keep track of the total width of the characters drawn so far
    int spacing = 25;  // Space between each character, you can adjust this

    while (*s) {  // Iterate through each character in the string
        draw_imageChar(*s, x + xOffset, y, attr);  // Draw the character at the new x position
        xOffset += spacing;  // Move the x position for the next character
        s++;  // Go to the next character in the string
    }
}


// void drawString(int x, int y, char *s, unsigned char attr)
// {
//     while (*s) {
//        if (*s == '\r') {
//           x = 0;
//        } else if(*s == '\n') {
//           x = 0; y += FONT_HEIGHT;
//        } else {
// 	  drawChar(*s, x, y, attr);
//           x += FONT_WIDTH;
//        }
//        s++;
//     }
// }


// void drawCharWelcome(unsigned char ch, int x, int y, unsigned char attr)
// {
//     unsigned char *glyph = (unsigned char *)&font + (ch < FONT_NUMGLYPHS ? ch : 0) * FONT_BPG;

//     for (int i = 0; i < FONT_HEIGHT; i++) {
//         for (int j = 0; j < FONT_WIDTH; j++) {
//             unsigned char mask = 1 << j;
//             unsigned char col = (*glyph & mask) ? attr & 0x0f : (attr & 0xf0) >> 4;

//             drawPixel(x+j, y+i, col);
//             // drawPixel(x+8*j, y+8*i + 1, col);
//             // drawPixel(x+8*j + 1, y+8*i, col);
//             // drawPixel(x+8*j + 1, y+8*i + 1, col);

//             //drawPixel(x+8*j, y+8*i, col);
//             // drawPixel(x+8*j, y+8*i + 2, col);
//             // drawPixel(x+8*j + 2, y+8*i, col);
//             // drawPixel(x+8*j + 2, y+8*i+2, col);
//             // drawPixel(x+8*j + 1, y+8*i + 2, col);
//             // drawPixel(x+8*j + 2, y+8*i + 1, col);
            

//             // drawPixel(x+8*j, y+8*i + 3, col);
//             // drawPixel(x+8*j + 3, y+8*i, col);
//             // drawPixel(x+8*j + 3, y+8*i+3, col);
//             // drawPixel(x+8*j + 1, y+8*i + 3, col);
//             // drawPixel(x+8*j + 3, y+8*i + 1, col);

//             // drawPixel(x+8*j, y+8*i + 4, col);
//             // drawPixel(x+8*j + 4, y+8*i, col);
//             // drawPixel(x+8*j + 4, y+8*i+4, col);
//             // drawPixel(x+8*j + 1, y+8*i + 4, col);
//             // drawPixel(x+8*j + 4, y+8*i + 1, col);

//             // drawPixel(x+8*j, y+8*i + 5, col);
//             // drawPixel(x+8*j + 5, y+8*i, col);
//             // drawPixel(x+8*j + 5, y+8*i+5, col);
//             // drawPixel(x+8*j + 8, y+8*i + 5, col);
//             // drawPixel(x+8*j + 5, y+8*i + 8, col);

//             // drawPixel(x+8*j + 5, y+8*i + 2, col);
//             // drawPixel(x+8*j + 2, y+8*i + 5, col);          

//             // drawPixel(x+8*j  + 5, y+8*i + 3, col);
//             // drawPixel(x+8*j + 3, y+8*i  + 5, col);            

//             // drawPixel(x+8*j + 5, y+8*i + 4, col);
//             // drawPixel(x+8*j + 4, y+8*i + 5, col);
            
//             // drawPixel(x+8*j, y+8*i + 5, col);
//             // drawPixel(x+8*j + 5, y+8*i, col);
//             // drawPixel(x+8*j + 5, y+8*i+5, col);
//             // drawPixel(x+8*j + 8, y+8*i + 5, col);
//             // drawPixel(x+8*j + 5, y+8*i + 8, col);

//             // drawPixel(x+8*j, y+8*i + 6, col);
//             // drawPixel(x+8*j + 6, y+8*i, col);
//             // drawPixel(x+8*j + 6, y+8*i+6, col);
//             // drawPixel(x+8*j + 8, y+8*i + 6, col);
//             // drawPixel(x+8*j + 6, y+8*i + 8, col);

//             // drawPixel(x+8*j, y+8*i + 7, col);
//             // drawPixel(x+8*j + 7, y+8*i, col);
//             // drawPixel(x+8*j + 7, y+8*i+7, col);
//             // drawPixel(x+8*j + 8, y+8*i + 7, col);
//             // drawPixel(x+8*j + 7, y+8*i + 8, col);

//             // drawPixel(x+8*j, y+8*i + 8, col);
//             // drawPixel(x+8*j + 8, y+8*i, col);
//             // drawPixel(x+8*j + 8, y+8*i+8, col);
//             // drawPixel(x+8*j + 8, y+8*i + 8, col);
//             // drawPixel(x+8*j + 8, y+8*i + 8, col);
//         }
//         glyph += FONT_BPL;
//     }
// }