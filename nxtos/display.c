/* Driver for the NXT's display.
 *
 * This driver provides a high level interface for displaying data. It
 * uses the low-level LCD driver to do actual output.
 */

#include "mytypes.h"
#include "interrupts.h"
#include "systick.h"
#include "aic.h"
#include "lcd.h"
#include "util.h"

/* A simple 8x5 font. This is in a separate file because the embedded
 * font is converted from a .png at compile time.
 */
#include "font.h"

static struct {
  /* The display buffer, which is mirrored to the LCD controller's RAM. */
  U8 buffer[LCD_HEIGHT][LCD_WIDTH];

  /* The position of the text cursor. This is used for easy displaying
   * of text in a console-like manner.
   */
  struct {
    U8 x;
    U8 y;
    bool ignore_lf; /* If the display just wrapped from the right side
                       of the screen, ignore an LF immediately
                       after. */
  } cursor;
} display;

/*
 * General display functions.
 */

/* Mirror the current in-memory screen to the display. */
void display_refresh() {
  lcd_display_data(&display.buffer[0][0]);
}

/* Clear the display. */
void display_clear() {
  memset(&display.buffer[0][0], 0, sizeof(display.buffer));
}


/*
 * Text display functions.
 */
static inline bool is_on_screen(U8 x, U8 y) {
  if (display.cursor.x < LCD_WIDTH && display.cursor.y < LCD_HEIGHT)
    return TRUE;
  else
    return FALSE;
}

static inline const U8 *char_to_font(const char c) {
  if (c >= FONT_START)
    return font[c - FONT_START];
  else
    return font[0]; /* Unprintable characters become spaces. */
}

static inline void update_cursor(bool inc_y) {
  if (!inc_y) {
    display.cursor.x++;

    if (display.cursor.x >= LCD_WIDTH) {
      display.cursor.x = 0;
      display.cursor.y++;
      display.cursor.ignore_lf = TRUE;
    } else {
      display.cursor.ignore_lf = FALSE;
    }
  } else if (display.cursor.ignore_lf) {
    display.cursor.ignore_lf = FALSE;
  } else {
    display.cursor.x = 0;
    display.cursor.y++;
  }

  if (display.cursor.y >= LCD_HEIGHT)
    display.cursor.y = 0;
}

void display_cursor_set_pos(U8 x, U8 y) {
  if (is_on_screen(x, y)) {
    display.cursor.x = x;
    display.cursor.y = y;
  }
}

void display_string(const char *str) {
  while (*str != '\0') {
    if (*str == '\n')
      update_cursor(TRUE);
    else {
      memcpy(&display.buffer[display.cursor.y][display.cursor.x * CELL_WIDTH],
             char_to_font(*str), FONT_WIDTH);
      update_cursor(FALSE);
    }
    str++;
  }
}

void display_hex(U32 val) {
  const char hex[16] = "0123456789ABCDEF";
  char buf[9];
  char *ptr = &buf[8];

  while (val != 0) {
    ptr--;
    *ptr = hex[val & 0xF];
    val >>= 4;
  }
  buf[8] = '\0';

  display_string(ptr);
}

void display_uint(U32 val) {
  char buf[11];
  char *ptr = &buf[10];

  while (val > 0) {
    ptr--;
    *ptr = val % 10 + '0';
    val /= 10;
  }
  buf[10] = '\0';

  display_string(ptr);
}


/*
 * Display initialization.
 */
void display_init() {
  display_clear();
  display_refresh();
  display.cursor.x = 0;
  display.cursor.y = 0;
  display.cursor.ignore_lf = FALSE;
}