/**
 * rofi-plugin-template
 *
 * MIT/X11 License
 * Copyright (c) 2017 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <gmodule.h>
#include <gio/gio.h>

#include <rofi/mode.h>
#include <rofi/helper.h>
#include <rofi/mode-private.h>

#include <stdint.h>

G_MODULE_EXPORT Mode mode;

/**
 * The internal data structure holding the private data of the TS Mode.
 */
typedef struct
{
    char *cmd;
    char *prev_input;
    char *translation;
    GPtrArray *history;
    gboolean detailed;
} TSModePrivateData;

/**
 * See rofi/include/widgets/textbox.h
 */
#define MARKUP 8

#define MAX_HISTORY 1000

#define TS_COMMAND_OPTION "-ts-command"

#define RESULT_PLACEHOLDER "{result}"

static char *get_history_path ( void )
{
    const char *dir = g_get_user_data_dir ();
    return g_build_filename ( dir, "rofi", "rofi_ts_history", NULL );
}

static void get_ts ( Mode *sw )
{
    /** 
     * Get the entries to display.
     * this gets called on plugin initialization.
     */
    GError *error = NULL;
    TSModePrivateData *pd = (TSModePrivateData *) mode_get_private_data ( sw );

    pd->cmd = NULL;
    find_arg_str ( TS_COMMAND_OPTION, &pd->cmd );

    pd->prev_input = g_strdup ( "" );
    pd->translation = g_strdup ( "type to trans" );
    pd->detailed = FALSE;

    char *path = get_history_path ();

    if ( !g_file_test ( path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR ) ) {
        pd->history = g_ptr_array_new ();
        g_free ( path );
        return;
    }

    char *content = NULL;
    g_file_get_contents ( path, &content, NULL, &error );
    g_free ( path );

    if ( error != NULL ) {
        g_error ( "Reading history failed: %s", error->message );
        g_error_free ( error );
    }

    g_strstrip ( content );
    char **lines = g_strsplit ( content, "\n", -1 );
    g_free ( content );

    pd->history = g_ptr_array_new_from_null_terminated_array ( (void **)lines, NULL, NULL, g_free );
    g_free ( lines );
}

static unsigned int get_real_index ( TSModePrivateData *pd, unsigned int selected_line ) {
    return pd->history->len - selected_line + 1;
}

static int ts_mode_init ( Mode *sw )
{
    /**
     * Called on startup when enabled (in modi list)
     */
    if ( mode_get_private_data ( sw ) == NULL ) {
        TSModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        mode_set_private_data ( sw, (void *) pd );
        // Load content.
        get_ts ( sw );
    }
    return TRUE;
}
static unsigned int ts_mode_get_num_entries ( const Mode *sw )
{
    const TSModePrivateData *pd = (const TSModePrivateData *) mode_get_private_data ( sw );
    return pd->history->len + 2;
}

static char *ts_escape2pango ( const char *input )
{
    GString *str = g_string_new ( "" );

    gboolean escape_switch = FALSE;
    for ( const char *c = input; *c != '\0'; c++ ) {
        if ( *c == '\x1b') {
            escape_switch = TRUE;
            c++;
        } else if ( escape_switch ) {
            escape_switch = FALSE;
            if ( *c == '1' && *(c + 1) == 'm' ) {
                c++;
                g_string_append ( str, "<b>" );
            } else if ( *c == '2' && *(c + 1) == '2' && *(c + 2) == 'm' ) {
                c += 2;
                g_string_append ( str, "</b>" );
            } else if ( *c == '4' && *(c + 1) == 'm' ) {
                c++;
                g_string_append ( str, "<u>" );
            } else if ( *c == '2' && *(c + 1) == '4' && *(c + 2) == 'm' ) {
                c += 2;
                g_string_append ( str, "</u>" );
            } else if ( *c == '3' && *(c + 1) == '3' && *(c + 2) == 'm') {
                c += 2;
                g_string_append ( str, "<span foreground='yellow'>" );
            } else if ( *c == '0' && *(c + 1) == 'm' ) {
                c++;
                g_string_append ( str, "</span>" );
            } else {
                g_error ( "Unknown escape sequence: %c", *c );
            }
        } else {
            char *escaped_text = g_markup_escape_text ( c, 1 );
            g_string_append ( str, escaped_text );
            g_free ( escaped_text );
        }
    }

    return g_string_free_and_steal ( str );
}

static char *ts_get_brief_message ( const char *translation ) {
    char **lines = g_strsplit ( translation, "\n", -1 );
    unsigned int num_lines = g_strv_length ( lines );

    char *last_line = g_strdup ( num_lines > 1 ? lines[num_lines - 2] : translation );
    g_strfreev ( lines );
    g_strstrip ( last_line );

    return ts_escape2pango ( last_line );
}

static char *ts_get_detailed_message ( const char *translation ) {
    return ts_escape2pango ( translation );
}

static char *ts_get_message ( const Mode *sw )
{
    const TSModePrivateData *pd = (const TSModePrivateData *) mode_get_private_data ( sw );

    const char *translation = pd->translation;
    return pd->detailed ? ts_get_detailed_message ( translation )
                        : ts_get_brief_message ( translation );
}

static void write_history ( TSModePrivateData *pd ) {
    GError *error = NULL;
    char *path = get_history_path ();

    if ( !g_file_test ( path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR ) ) {
        g_mkdir_with_parents ( g_path_get_dirname ( path ), 0755 );
    }

    char *content = g_strdup ( "" );

    for ( unsigned int i = 0; i < pd->history->len; i++ ) {
        char *new_content = g_strconcat ( content, "\n", g_ptr_array_index ( pd->history, i ), NULL );
        g_free ( content );
        content = new_content;
    }

    g_strstrip ( content );
    g_file_set_contents ( path, content, -1, &error );
    g_free ( content );
    g_free ( path );

    if ( error != NULL ) {
        g_error ( "Writing history failed: %s", error->message );
        g_error_free ( error );
    }
}

static char *get_history_line ( const char *input, const char *brief ) {
    return g_strconcat ( input, "->", brief, NULL );
}

static void append_history ( const char *input, TSModePrivateData *pd )
{
    if ( pd->history->len == MAX_HISTORY ) {
        g_ptr_array_remove_index ( pd->history, 0 );
    }

    char *brief = ts_get_brief_message ( pd->translation );
    char *newline = get_history_line ( input, brief );
    g_free ( brief );

    g_ptr_array_add ( pd->history, newline );
    write_history ( pd );
}

static void remove_history_at ( unsigned int index, TSModePrivateData *pd )
{
    g_ptr_array_remove_index ( pd->history, index );
    write_history ( pd );
}

static void run_command ( const char *cmd, const char *data ) {
    if ( cmd == NULL ) {
        return;
    }

    char *user_cmd = helper_string_replace_if_exists ( (char*) cmd, RESULT_PLACEHOLDER, data, NULL );
    char *escaped_cmd = g_shell_quote ( user_cmd );
    g_free ( user_cmd );

    char *command = g_strconcat ( "/bin/sh -c ", escaped_cmd, NULL );
    g_free ( escaped_cmd );

    helper_execute_command ( NULL, command, FALSE, NULL );
    g_free ( command );
}

static ModeMode ts_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    ModeMode           retv  = MODE_EXIT;
    TSModePrivateData *pd = (TSModePrivateData *) mode_get_private_data ( sw );
    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    } else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    } else if ( mretv & MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
    } else if ( ( mretv & MENU_OK ) ) {
        if ( selected_line == 0 ) {
            pd->detailed = !pd->detailed;
            if ( pd->detailed ) {
                append_history ( *input, pd );
            }
            retv = RELOAD_DIALOG;
        } else if ( selected_line == 1 ) {
            char *brief = ts_get_brief_message ( pd->translation );
            char *data = get_history_line ( *input, brief );
            g_free ( brief );

            run_command ( pd->cmd, data );
            g_free ( data );
            retv = MODE_EXIT;
        } else {
            char *history_line = g_ptr_array_index ( pd->history, get_real_index ( pd, selected_line ) );
            run_command ( pd->cmd, history_line );
            retv = MODE_EXIT;
        }
    } else if ( mretv & MENU_ENTRY_DELETE ) {
        if ( selected_line > 1 ) {
            remove_history_at ( get_real_index ( pd, selected_line ), pd );
        }
        retv = RELOAD_DIALOG;
    }
    return retv;
}

static void ts_mode_destroy ( Mode *sw )
{
    TSModePrivateData *pd = (TSModePrivateData *) mode_get_private_data ( sw );
    if ( pd != NULL ) {
        g_free ( pd->prev_input );
        g_free ( pd->translation );
        g_ptr_array_free ( pd->history, TRUE );
        g_free ( pd );
        mode_set_private_data ( sw, NULL );
    }
}

static char *_get_display_value ( const Mode *sw, unsigned int selected_line, G_GNUC_UNUSED int *state, G_GNUC_UNUSED GList **attr_list, int get_entry )
{
    TSModePrivateData *pd = (TSModePrivateData *) mode_get_private_data ( sw );

    *state |= MARKUP;

    if ( !get_entry ) {
        return NULL;
    }

    if ( selected_line == 0 ) {
        return pd->detailed ? g_strdup ( "brief" ) : g_strdup ( "detailed" );
    } else if ( selected_line == 1 ) {
        return g_strdup ( "copy" );
    } else {
        return g_strdup ( g_ptr_array_index ( pd->history, get_real_index ( pd, selected_line ) ) );
    }
}

/**
 * @param sw The mode object.
 * @param tokens The tokens to match against.
 * @param index  The index in this plugin to match against.
 *
 * Match the entry.
 *
 * @param returns try when a match.
 */
static int ts_token_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int index )
{
    TSModePrivateData *pd = (TSModePrivateData *) mode_get_private_data ( sw );

    if ( index == 0 || index == 1 ) {
        return TRUE;
    } else {
        return helper_token_match ( tokens, g_ptr_array_index ( pd->history, get_real_index ( pd, index ) ) );
    }
}

extern void rofi_view_reload ( void );

static void get_translation_callback ( GObject *source_object, GAsyncResult *res, gpointer user_data )
{
    GSubprocess *process = G_SUBPROCESS ( source_object );
    GError *error = NULL;
    GInputStream *stdout_stream = g_subprocess_get_stdout_pipe ( process );
    GInputStream *stderr_stream = g_subprocess_get_stderr_pipe ( process );

    char **translation = (char **) user_data;

    g_subprocess_wait_check_finish ( process, res, &error );

    if ( error != NULL ) {
        g_error ( "Process errored with: %s", error->message );
        g_error_free ( error );
    }

    const unsigned int stdout_bufsize = 4096;
    const unsigned int stderr_bufsize = 4096;
    char *stdout_buf = g_malloc0 ( stdout_bufsize );
    char *stderr_buf = g_malloc0 ( stderr_bufsize );

    g_input_stream_read_all ( stdout_stream, stdout_buf, stdout_bufsize, NULL, NULL, &error );
    if ( error != NULL ) {
        g_error ( "Reading stdout failed: %s", error->message );
        g_error_free ( error );
    }

    g_input_stream_close ( stdout_stream, NULL, &error );
    if ( error != NULL ) {
        g_error ( "Closing stdout failed: %s", error->message );
        g_error_free ( error );
    }

    g_input_stream_read_all ( stderr_stream, stderr_buf, stderr_bufsize, NULL, NULL, &error );
    if ( error != NULL ) {
        g_error ( "Reading stderr failed: %s", error->message );
        g_error_free ( error );
    }

    g_input_stream_close ( stderr_stream, NULL, &error );
    if ( error != NULL ) {
        g_error ( "Closing stderr failed: %s", error->message );
        g_error_free ( error );
    }

    if ( *translation != NULL ) {
        g_free ( *translation );
    }
    *translation = g_strconcat ( stderr_buf, "\n", stdout_buf, NULL );

    g_debug ( "Stdout: %s", stdout_buf );
    g_debug ( "Stderr: %s", stderr_buf );
    g_debug ( "Translation: %s", *translation );

    g_free ( stdout_buf );
    g_free ( stderr_buf );

    rofi_view_reload ();
}

static void get_translation ( const char *input, char **translation )
{
    GError *error = NULL;
    const char *executable = "trans";

    GPtrArray *argv = g_ptr_array_new ();
    g_ptr_array_add ( argv, (char*) executable );
    g_ptr_array_add ( argv, (char*) input );
    g_ptr_array_add ( argv, NULL );

    GSubprocess *process = g_subprocess_newv ( (const gchar**)(argv->pdata), G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE, &error );
    g_ptr_array_free ( argv, TRUE );

    if ( error != NULL ) {
        g_error ( "Spawning child failed: %s", error->message );
        g_error_free ( error );
    }

    g_subprocess_wait_check_async ( process, NULL, get_translation_callback, translation );
}

static char *ts_preprocess_input ( Mode *sw, const char *input )
{
    TSModePrivateData *pd = (TSModePrivateData *) mode_get_private_data ( sw );

    if ( g_strcmp0 ( input, pd->prev_input ) != 0 ) {
        g_free ( pd->prev_input );
        pd->prev_input = g_strdup ( input );
        pd->detailed = FALSE;

        get_translation ( input, &pd->translation );
    }

    return g_strdup ( input );
}


Mode mode =
{
    .abi_version        = ABI_VERSION,
    .name               = "ts",
    .cfg_name_key       = "display-ts",
    ._init              = ts_mode_init,
    ._get_num_entries   = ts_mode_get_num_entries,
    ._result            = ts_mode_result,
    ._destroy           = ts_mode_destroy,
    ._token_match       = ts_token_match,
    ._get_display_value = _get_display_value,
    ._get_message       = ts_get_message,
    ._get_completion    = NULL,
    ._preprocess_input  = ts_preprocess_input,
    .private_data       = NULL,
    .free               = NULL,
};
