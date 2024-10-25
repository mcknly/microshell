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

#include <string.h>

#include "inc/ush_history.h"
#include "inc/ush_internal.h"

void ush_read_echo_service(struct ush_object *self, char ch)
{
        switch (ch) {
        case '\r':
                self->desc->output_buffer[0] = ch;
                self->desc->output_buffer[1] = '\n';
                self->desc->output_buffer[2] = '\0';
                break;
        case '\x08':
        case '\x7F':
                self->desc->output_buffer[0] = '\x08';
                self->desc->output_buffer[1] = ' ';
                self->desc->output_buffer[2] = '\x08';
                self->desc->output_buffer[3] = '\0';
                break;
        default:
                self->desc->output_buffer[0] = ch;
                self->desc->output_buffer[1] = '\0';
                break;
        }

        ush_state_t next = (ch == '\r' || ch == '\n') ? USH_STATE_PARSE_PREPARE : self->state;
        ush_write_pointer(self, self->desc->output_buffer, next);
}

bool ush_read_char_by_escape_state(struct ush_object *self, char ch)
{
        bool echo = true;

        if (self->ansi_escape_state == 0) {
                /* Save character to history buffer */
                size_t in_pos = self->in_pos;
                self->desc->input_history->buffer[HLINE_IDX(0) + in_pos++] = ch;
                if (in_pos >= self->desc->input_history->length)
                        in_pos = 0;
                self->desc->input_history->buffer[HLINE_IDX(0) + in_pos] = '\0';

                self->desc->input_buffer[self->in_pos++] = ch;
                if (self->in_pos >= self->desc->input_buffer_size)
                        self->in_pos = 0;
                self->desc->input_buffer[self->in_pos] = '\0';
        } else if (self->ansi_escape_state == 1) {
                /* normal or ctrl */
                if (ch == '\x5B' || ch == '\x4F') {
                        self->ansi_escape_state = 2;
                } else {
                        self->ansi_escape_state = 0;
                }
                echo = false;
        } else if (self->ansi_escape_state == 2) {
                if (ch == '\x41' || ch == '\x01') { /* up arrow or ctrl+a */
                        /* up - process if upper limit not reached and there is a historic line to move to */
                        if ((self->history_index < self->desc->input_history->lines - 1) &&
                            (strlen(&self->desc->input_history->buffer[HLINE_IDX(self->history_index + 1)]))) {
                                self->history_index++;
                                ush_history_load_line(self, self->history_index);
                        }
                } else if (ch == '\x42' || ch == '\x1A') { /* down arrow or ctrl+z */
                        /* down - process if lower limit is not reached */
                         if (self->history_index > 0) {
                                self->history_index--;
                                ush_history_load_line(self, self->history_index);
                         }
                } else if (ch == '\x43') {
                        /* right */
                } else if (ch == '\x44') {
                        /* left */
                }
                self->ansi_escape_state = 0;
                echo = false;
        }

        return echo;
}

void ush_read_start(struct ush_object *self)
{
        USH_ASSERT(self != NULL);

        ush_history_save_line(self, 0, true);

        self->in_pos = 0;
        self->desc->input_buffer[self->in_pos] = '\0';
        self->state = USH_STATE_READ_CHAR;
        self->ansi_escape_state = 0;
}
