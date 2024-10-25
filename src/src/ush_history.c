/*
MIT License

Copyright (c) 2021 Marcin Borowicz <marcinbor85@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "inc/ush_history.h"
#include "inc/ush_internal.h"
#include "inc/ush_preconfig.h"

#include <stdarg.h>
#include <string.h>

void ush_history_reset(struct ush_object *self)
{
        /* Reset all history variables in ush_object */
        self->history_buf = 0;
        self->history_index = 0;
        self->history_backspace = 0;
        self->history_pos = 0;

        /* Reset history buffer area */
        memset(&self->desc->input_history->buffer[HLINE_IDX(0)], 0x00,
               self->desc->input_history->lines * self->desc->input_history->length);
}

int ush_history_read_char(struct ush_object *self, char *ch)
{
        int ret = 0;

        /* Start by returning backspace characters for current line */
        if (self->history_backspace > 0) {
                self->history_backspace--;
                *ch = '\x08';
                ret = 1;
        }
        /* Then continue returning history line characters for current line (except for \n and \r) */
        else {
                if (self->history_buf[self->history_pos] == '\n' ||
                    self->history_buf[self->history_pos] == '\r') {
                        self->history_pos++;
                }
                if (self->history_buf[self->history_pos] != 0x00) {
                        *ch = self->history_buf[self->history_pos++];
                        ret = 1;
                }
        }

        return ret;
}

void ush_history_load_line(struct ush_object *self, size_t hline_idx)
{
        USH_ASSERT(self != NULL);

        /* Setup erasing parameters for current line */
        self->history_backspace = strlen(&self->desc->input_history->buffer[HLINE_IDX(0)]);
        memset(&self->desc->input_history->buffer[0], 0x00,
               self->desc->input_history->length);

        /* Setup a history line as character input */
        self->history_buf = &self->desc->input_history->buffer[HLINE_IDX(hline_idx)];
        self->history_pos = 0;
        self->state = USH_STATE_READ_CHAR_BUFFER;
}

void ush_history_save_line(struct ush_object *self, size_t hline_idx, bool new_history_line)
{
        USH_ASSERT(self != NULL);

        /* Only save non-empty lines to history */
        if (strlen(&self->desc->input_history->buffer[HLINE_IDX(0)]) > 1) {

                /* Move out the oldest command line from history buffer? */
                if (new_history_line) {
                        for (size_t i = self->desc->input_history->lines - 1; i > 0; i--) {
                        memcpy(&self->desc->input_history->buffer[HLINE_IDX(i)],
                                &self->desc->input_history->buffer[HLINE_IDX(i - 1)],
                                self->desc->input_history->length);
                        }
                        memset(&self->desc->input_history->buffer[HLINE_IDX(0)], 0x00,
                               self->desc->input_history->length);
                        self->history_index = 0;
                }
                else {
                        /* Copy command line to history line */
                        memcpy(&self->desc->input_history->buffer[HLINE_IDX(hline_idx)],
                               &self->desc->input_history->buffer[HLINE_IDX(0)],
                               self->desc->input_history->length);
                }
        }
}
