/*
 * core.h - page loading and layout for the HTML reader.
 *
 * Pure C with no calculator dependencies, so it can be tested on a PC.
 * Reads the page format produced by html2ce.py (format version 3) and turns it
 * into styled, word-wrapped lines with pixel positions. Table rows with 2-3 cells
 * become side-by-side columns.
 */
#ifndef CORE_H
#define CORE_H

#include <stdbool.h>
#include <stdint.h>

#define ARENA_CAP 30000u /* bytes of laid-out lines kept in RAM per page */
#define LINE_CAP  2500u  /* max lines per page                           */

#define VIEW_W 312 /* width of the text area in pixels             */
#define CHAR_W 8   /* width of one character at scale 1            */
#define LINE_H 9   /* height of a normal text line, in pixels      */

#define MAX_COLS    3   /* most columns side by side                    */
#define COL_GUTTER  8   /* pixels between columns                       */
#define COLLINE_CAP 600u /* max lines in one column of a block          */
#define BLOCK_CAP   24u  /* max column blocks per page                  */

/* Per-run style byte used in text lines. */
#define S_BOLD       0x01
#define S_ITAL       0x02
#define S_HEAD_MASK  0x0C /* heading size 0-3 in bits 2-3 */
#define S_LINK       0x10 /* link whose target is on the calculator */
#define S_DEAD       0x20 /* link whose target is not here          */
#define STYLE_HEAD(s) (((s) & S_HEAD_MASK) >> 2)

/* Heading sizes: 0 = body text, 1 = 2x2, 2 = 1x2 (tall), 3 = 1x1 bold. */
static inline uint8_t head_sx(uint8_t h) { return h == 1 ? 2 : 1; }
static inline uint8_t head_sy(uint8_t h) { return (h == 1 || h == 2) ? 2 : 1; }

enum { LK_TEXT = 0, LK_RULE = 1, LK_IMAGE = 2, LK_ROW = 3 };

enum {
    DOC_OK = 0,
    DOC_BAD_HEADER, /* too short or inconsistent header */
    DOC_BAD_MAGIC,  /* not an html2ce page              */
    DOC_BAD_VERSION /* format version we don't know     */
};

typedef struct {
    char     title[41];
    char     prev[9];     /* AppVar name of previous page, "" if none */
    char     next[9];     /* AppVar name of next page, "" if none     */
    uint16_t nlines;
    uint16_t total_h;     /* total height of the page in pixels       */
    uint16_t arena_used;  /* bytes of ARENA_CAP this page needed      */
    bool     truncated;   /* page didn't fit the buffers              */
} Doc;

/* One laid-out line.
 *   LK_TEXT : p[0] = number of segments, then per segment
 *             [style][length][characters...]
 *   LK_RULE : no payload
 *   LK_IMAGE: p[0..7] sprite AppVar name, p[8] width, p[9] height
 *   LK_ROW  : one row of side-by-side columns; use doc_row_cols()/doc_row_cell() */
typedef struct {
    uint8_t        kind;
    uint8_t        center;  /* 1 = centre inside the text area */
    uint8_t        indent;  /* left indent in pixels           */
    uint8_t        height;  /* line height in pixels           */
    uint16_t       y;       /* top of the line within the page */
    const uint8_t *p;
} LineView;

/* Parse and lay out a page. `data` points at the AppVar contents (after the
 * 2-byte size). Replaces any previously laid-out page. */
int doc_layout(Doc *d, const uint8_t *data, uint16_t size);

bool     doc_line(const Doc *d, uint16_t i, LineView *v);
uint16_t doc_y(const Doc *d, uint16_t i);                 /* top of line i              */
uint16_t doc_line_at(const Doc *d, uint16_t y);           /* first line with top >= y   */
uint16_t doc_max_top(const Doc *d, uint16_t view_height); /* last useful first line     */

/* Columns of an LK_ROW line. Each column of a row holds a short stack of lines.
 * doc_row_cell() fills `first` with the top line of column `col`, sets *count to how
 * many lines the column has in this row, and gives the column's left offset and width
 * within the text area; it returns false if the column is empty in this row.
 * doc_cell_next() moves a LineView on to the next line of the same column (its y is
 * the line's absolute position on the page). */
uint8_t doc_row_cols(const LineView *row);
bool    doc_row_cell(const Doc *d, const LineView *row, uint8_t col, LineView *first,
                     uint8_t *count, uint16_t *x, uint16_t *w);
void    doc_cell_next(LineView *v);

#endif
