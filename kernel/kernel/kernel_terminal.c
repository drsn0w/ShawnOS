/**
 * Basic kernel terminal driver
 * TODO: allow use of alternative video drivers
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kernel/kernel_terminal.h>
#include <kernel/kernel_stdio.h>
#include <drivers/pc/pit.h>
#include <drivers/vga/textmode.h>

char kernel_terminal_escapecodes[ESCAPECODE_LENGTH] =
{
    '\n',
    '\b',
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};


size_t cur_xpos = 0;
size_t cur_ypos = 0;
uint8_t color;
bool is_ansi_escape = false;
uint8_t ansi_escape_length = 0;

uint16_t terminal_buffer[VGA_HEIGHT * VGA_WIDTH];

void kernel_terminal_init(uint16_t refresh_rate) {
    // Initalize driver for device to output to
    vga_textmode_initialize();
    color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);

    // Initialize empty terminal_buffer
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = make_vgaentry(' ', color);
        }
    }

    // Install terminal update tick to timer
    // TODO: un-hardcode use of PIT
    struct pit_routine kernel_terminal_task = {
        10000/refresh_rate,
        kernel_terminal_update_tick
    };
    pit_install_scheduler_routine(kernel_terminal_task);

    // Initially update terminal
    kernel_terminal_update_tick();
}

void kernel_terminal_update_tick() {
    uint16_t i;

    // Get kernel STDOUT buffer
    char stdout_buffer[STDOUT_MAX_BUFFER];
    uint16_t stdout_buffer_length = kernel_buffer_stdout_get(stdout_buffer);

    if(stdout_buffer_length != 0) {
        // Refresh current console
        vga_textmode_clear();

        // Restore old text from our buffer
        vga_textmode_writebuffer(terminal_buffer, VGA_WIDTH*VGA_HEIGHT);

        for(i=0; i<stdout_buffer_length; i++) {
            char cur_char = stdout_buffer[i];

            // Parse char for terminal escape codes
            if (kernel_terminal_check_escapecode(cur_char) || is_ansi_escape) {
                kernel_terminal_handle_escapecode(cur_char);
            } else { // No escape codes

                // Check for overflow and wrap text accordingly
                kernel_terminal_handle_overflow();

                // Check for need to scroll and handle
                // kernel_terminal_handle_scroll();

                kernel_terminal_putentry(cur_char, color, cur_xpos++, cur_ypos);
            }
        }

    }
}

/**
 * Determines whether char is an escape code or not
 * @param c char to test
 */
bool kernel_terminal_check_escapecode(char c) {
    size_t i;
    for (i=0; i<ESCAPECODE_LENGTH; i++) {
        if (c == kernel_terminal_escapecodes[i]) {
            return true;
        }
    }

    // Check for ESC character in the case of an ANSI escapecode
    if (c == 27) {
        is_ansi_escape = true;
        ansi_escape_length = 1;
        return true;
    }

    return false;
}

/**
 * Handle escape code in terminal
 * @param c [description]
 */
void kernel_terminal_handle_escapecode(char c) {
    //TODO: Insert logic to handle all escape codes

    // Handle normal escapes
    switch(c) {
        case '\n':
            cur_ypos++;
            cur_xpos = 0;
            return;
        case '\b':
            kernel_terminal_backspace();
            return;
    }

    // Handle ANSI escapes
    if (is_ansi_escape) {
        switch(ansi_escape_length) {
            case 1: // First char of ANSI escape code should be ESC (27)
                if (c != 27) goto ansi_fail;
                ++ansi_escape_length;
                return;
            case 2: // Second char should be left square bracket
                if (c != '[') goto ansi_fail;
                ++ansi_escape_length;


        }
    }

ansi_fail:
    is_ansi_escape = false;
    ansi_escape_length = 0;
    return;
}

void kernel_terminal_handle_overflow() {
    if (cur_xpos >= VGA_WIDTH) {
        cur_xpos = 0;
        cur_ypos++;
    }
}

/**
 * Removes the last printed character in display and terminal buffer
 */
void kernel_terminal_backspace() {
    if (cur_xpos == 0 && cur_ypos == 0) {
        kernel_terminal_putentry(' ', color, 0, 0);
    } else if (cur_xpos == 0) {
        kernel_terminal_putentry(' ', color, VGA_WIDTH, cur_ypos-1);
    } else {
        kernel_terminal_putentry(' ', color, cur_xpos-1, cur_ypos);
    }
}

/**
 * Put character in both display buffer and local terminal buffer
 * @param c     char to put
 * @param color color of char to put
 * @param x     x coordinate
 * @param y     y coordinate
*/
void kernel_terminal_putentry(char c, uint8_t color, size_t x, size_t y) {
    // Update actual VGA output
    vga_textmode_putentryat(c, color, x, y);

    // Update local buffer
    terminal_buffer[y * VGA_WIDTH + x] = make_vgaentry(c, color);
}
