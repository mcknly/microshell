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

#include "inc/ush_internal.h"
#include "inc/ush_preconfig.h"
#include "inc/ush_history.h"

static void ush_read_process(struct ush_object *self, char ch)
{
        bool echo = true;
        
        switch (ch) {
        case '\x03':
                /* ctrl+c */
                ush_write_pointer(self, "^C", USH_STATE_RESET);
                echo = false;
                break;
        case '\x08':
        case '\x7F':
                /* backspace */
                if (self->in_pos > 0) {
                        self->in_pos--;
                        self->desc->input_buffer[self->in_pos] = '\0';
                } else {
                        echo = false;
                }
                break;
        case '\x09':
                /* tab */
#if USH_CONFIG_ENABLE_FEATURE_AUTOCOMPLETE == 1
                ush_autocomp_start(self);
#endif /* USH_CONFIG_ENABLE_FEATURE_AUTOCOMPLETE */
                echo = false;
                break;
        case '\x1B':
                /* escape */
                self->ansi_escape_state = 1;
                echo = false;
                break;
        case '\x01':
        case '\x1A':
                /* ctrl+a or ctrl+z */
                self->ansi_escape_state = 2;
                echo = ush_read_char_by_escape_state(self, ch);
                break;
        default:
                echo = ush_read_char_by_escape_state(self, ch);
                break;
        }

        if (echo != false)
                ush_read_echo_service(self, ch);
}

bool ush_read_char(struct ush_object *self)
{
        USH_ASSERT(self != NULL);

        char ch;
        if (self->desc->io->read(self, &ch) == 0)
                return false;

        ush_read_process(self, ch);

        return true;
}

bool ush_read_char_buffer(struct ush_object *self)
{
        USH_ASSERT(self != NULL);
        
        char ch;
        if (ush_history_read_char(self, &ch) == 0) {
                self->state = USH_STATE_READ_CHAR;
                return false;
        }

        ush_read_process(self, ch);

        return true;
}
