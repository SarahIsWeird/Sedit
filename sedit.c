#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include <ncurses/ncurses.h>

/* When [CTRL] is pressed, keynames always have the following format: ^k */
int is_ctrl_pressed(int input, char key) {
    const char *name = keyname(input);
    return name[0] == '^' && name[1] == (key - 32); /* Convert from lower case to upper case */
}

void update_info_bar(bool is_buffer_dirty, bool ask_for_save, size_t line_number, size_t col_number) {
    char line[COLS + 1];
    int i;

    memset(line, 0, COLS);

    move(LINES - 1, 0);

    for (i = 0; i < COLS; ++i) {
        addch(' ');
    }

    if (!ask_for_save) {
        attron(A_BLINK);

        sprintf(line, "%d:%d %s", line_number, col_number + 1, is_buffer_dirty ? "Unsaved" : "  Saved");
        mvprintw(LINES - 1, COLS - strlen(line), line);
    } else {
        attron(A_STANDOUT);
        attroff(A_BLINK);

        mvprintw(LINES - 1, 0, "Save changes? (y/n/c)");

        attroff(A_STANDOUT);
    }

    move(2 + line_number - 1, col_number);
}

int main(int argc, char **argv) {
    FILE *file;

    char *buf = (char*) malloc(1); /* Allocate a byte here so we can consistently use realloc() */
    size_t buf_size = 0;

    char *read_buffer = (char*) malloc(1025);
    read_buffer[1024] = '\0';

    char **line_buf = (char**) malloc(sizeof(char**));
    size_t lines = 1;

    long bytes_read;

    char *split_ptr;

    size_t i;

    WINDOW *edit_win;

    bool running = true;
    bool is_buffer_dirty = false;

    int ch;

    if (argc < 1) { /* No file name given */
        printf("Usage: %s", argv[0]);
        return 1;
    }

    /* file = fopen(argv[1], "r");

    if (file == NULL) {
        printf("Error: %s is not a file.\n", argv[1]);
        return 1;
    }

    while (!feof(file)) {
        buf_size += 1024;
        buf = (char*) realloc(buf, buf_size + 1); /* Account for null byte *//*
        memset(read_buffer, 0, 1024);

        bytes_read = fread(read_buffer, 1, 1024, file);

        if (bytes_read < 1024 && ferror(file)) {
            perror("An error occured while trying to read the file. Do you have the correct permissions?\n");

            fclose(file);

            free(read_buffer);
            free(buf);

            return 1;
        }

        if (feof(file)) {
            buf_size -= 1024 - bytes_read; /* Make buffer fit exactly *//*
        }

        strcat(buf, read_buffer);
    };
    
    fclose(file);

    free(read_buffer);

    line_buf = (char**) malloc(1);

    lines = 0;

    split_ptr = strtok(buf, "\n");

    while (split_ptr != NULL) {
        ++lines;

        line_buf = (char**) realloc(line_buf, sizeof(char**) * lines);

        line_buf[lines - 1] = (char*) malloc(strlen(split_ptr + 1));
        line_buf[lines - 1][strlen(split_ptr)] = 0;

        strcpy(line_buf[lines - 1], split_ptr);

        split_ptr = strtok(NULL, "\n");
    }

    for (i = 0; i < lines; ++i) {
        printf("%s\n", line_buf[i]);
    }

    return 0; */

    initscr(); /* Init wincurses */

    raw(); /* Flags: All input (including F keys), don't echo to screen */
    noecho();
    keypad(stdscr, TRUE);

    edit_win = newwin(LINES - 3, COLS, 2, 0);
    refresh();

    printw("Editing ");
    attron(A_BOLD);
    printw("%s\n", argv[1]);
    attroff(A_BOLD);

    for (i = 0; i < COLS; ++i) {
        addch('~');
    }

    refresh();

    /* for (i = 0; (i < LINES - 3) && (i < lines); ++i) {
        mvwprintw(edit_win, i, 0, line_buf[i]);
    } */

    lines = 1;
    line_buf = (char**) malloc(sizeof(char*));
    line_buf[0] = (char*) malloc(1000);
    memset(line_buf[0], 0, 1000);

    int pos = 0;

    while (running) {
        ch = getch();
        
        if (is_ctrl_pressed(ch, 's')) {
            FILE *output_file = fopen(argv[1], "w");
            
            for (int i = 0; i < lines; ++i) {
                fprintf(output_file, "%s\n", line_buf[i]);
            }

            fclose(output_file);

            is_buffer_dirty = false;
        } else if (is_ctrl_pressed(ch, 'c')) {
            if (is_buffer_dirty) {
                update_info_bar(is_buffer_dirty, true, lines, pos);
                
                ch = getch();

                if (ch == 'y' || ch == 'Y') {
                    FILE *output_file = fopen(argv[1], "w");
            
                    for (int i = 0; i < lines; ++i) {
                        fprintf(output_file, "%s\n", line_buf[i]);
                    }

                    fclose(output_file);

                    running = false;
                } else if (ch == 'n' || ch == 'N') {
                    running = false;
                } else {
                    mvprintw(LINES - 1, 0, "                     ");
                }
            } else {
                running = false;
            }
        } else if (ch == '\n') {
            is_buffer_dirty = true;

            line_buf = (char**) realloc(line_buf, sizeof(char*) * ++lines);
            line_buf[lines - 1] = (char*) malloc(1000);
            memset(line_buf[lines - 1], 0, 1000);

            wmove(edit_win, lines - 1, 0);
            pos = 0;
        } else if (ch == '\b') {
            is_buffer_dirty = true;

            line_buf[lines - 1][--pos] = '\0';
            mvwaddch(edit_win, lines - 1, pos, ' ');
            wmove(edit_win, lines - 1, pos);
        }
        
        else if (ch < 256) {
            is_buffer_dirty = true;

            line_buf[lines - 1][pos++] = ch;
            waddch(edit_win, ch);
        }

        update_info_bar(is_buffer_dirty, false, lines, pos);

        wrefresh(edit_win);
    }

    wrefresh(edit_win);

    refresh();

    for (i = 0; i < lines; ++i) {
        free(line_buf[i]);
    }

    free(line_buf);

    delwin(edit_win);
    endwin();

    return 0;
}