/*
	Copyright (c) 2014 Zhang li

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.

	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/*
	Author zhang li
	Email zlvbvbzl@gmail.com
*/

#include "fmodwrap.h"
#include "filefinder.h"
#include <conio.h>
#include <stdio.h>
#include <Windows.h>
#include <vector>
#include <random>
#include <math.h>
#include <share.h>
#include <time.h>

namespace fmodwrap { extern FMOD_SYSTEM *kFmod_system; }

#define KEY_UP     72
#define KEY_DOWN   80
#define KEY_LEFT   75
#define KEY_RIGHT  77
#define KEY_ESC    27
#define KEY_A      'a'
#define KEY_Q      'q'
#define KEY_E      'e'
#define KEY_P      'p'
#define KEY_X      'x'
#define KEY_PGUP   73
#define KEY_PGDOWN 81
#define KEY_SPACE  32
#define KEY_RETURN 13
#define KEY_L      'l'
#define KEY_J      'j'
#define KEY_K      'k'


#define KEY_LIST_PREV     KEY_UP
#define KEY_LIST_NEXT     KEY_DOWN
#define KEY_PLAY_NEXT     KEY_RIGHT
#define KEY_PLAY_PREV     KEY_LEFT
#define KEY_VOLUME_QUIET  KEY_Q
#define KEY_VOLUME_LOUD   KEY_E
#define KEY_QUIT          KEY_ESC
#define KEY_VISUALIZATION KEY_X
#define KEY_ABOUT         KEY_A
#define KEY_PAUSE         KEY_SPACE
#define KEY_PLAY          KEY_RETURN
#define KEY_LOOP          KEY_L

#define VOLUME_LENGTH   20
#define PROCESS_CHAR    '|'
#define VOLUME_CHAR     '-'
#define PROCESS_POSCHAR '|'
#define VOLUME_POSCHAR  '|'

// Dynamic console size
static int con_width = 80;
static int con_height = 25;

static void query_console_size()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    {
        con_width  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        con_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        if (con_width < 40)  con_width = 40;
        if (con_height < 10) con_height = 10;
    }
}

#define MAX_LIST_INDEX  (con_height - 9)
#define MAX_LIST_COUNT  (MAX_LIST_INDEX + 1) 
#define MAX_NAME_LENGTH (con_width - 1)

#define FONT_LIGHT_BLUE       FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define FONT_WHITE            FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED
#define FONT_BLACK            0
#define FONT_LIGHTWHITE       FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY
#define FONT_LIGHT_GREEN      FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define FONT_LIGHT_RED        FOREGROUND_RED | FOREGROUND_INTENSITY
#define FONT_LIGHT_YELLOW     FONT_LIGHT_RED | FONT_LIGHT_GREEN
#define BG_WHITE              BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED
#define BG_LIGHT_GREEN        BACKGROUND_GREEN | BACKGROUND_INTENSITY
#define BG_LIGHT_RED          BACKGROUND_RED | BACKGROUND_INTENSITY
#define BG_LIGHT_YELLOW       BG_LIGHT_RED | BG_LIGHT_GREEN


#define ABOUT_TEXT \
"                                                                                "\
"                              [ CONSOLE PLAYER ]                                "\
"                                                                                "\
"                               Author: Zhang Li                                 "\
"                           Email: zlvbvbzl@gmail.com                            "\
"                                                                                "

#define CONTROLPANEL_TEXT \
"[Up/Down] Select   [E] Volume up\n"                        \
"[Left/Right] Prev/Next [Q] Volume down [X] Visualization\n"\
"[SPACE] Pause       [RETURN] Play       [PgUp/PgDn] Scroll list\n"\
"[L] Loop mode       [A] About           [Esc] Quit"

#define TITLE_TEXT \
"                                                                                "\
"   CONSOLE PLAYER                                                               "\
"________________________________________________________________________________"

#define VIS_CHAR "~!@#$%^&-*()_+|<>{}:\"`=\\[];\',./?"

const char* FORMATS[] = {
    "MP3", "WAV", "OGG", "WMA",
    "FLAC", "AAC", "M4A",
    "AIFF", "AIF",
    "MID", "MIDI",
    "MOD", "S3M", "IT", "XM",
    "OPUS",
    "#"
};

enum LoopState
{
    LS_NORMAL,
    LS_ONE,
    LS_RAND,
    LS_COUNT,
};

std::vector<char*> sounds;
std::vector<char*> display_names;
fmodwrap::Sound     *sound_playing = NULL;
fmodwrap::Player    *player = NULL;
int     list_cur = 0;
int     current_sound = -1;
int     list_start = 0;
float   fft_rate[1024] = {0};
bool    visualization_isopen = false;
char    vis_char = '|';
bool    needupdatelist = true;
bool    needupdatevolume = true;
bool    needupdateprocbar = true;
bool    needshowhelp = true;
int     sleep_time = 1000;
bool    need_next = true;
bool    force_refresh_procbar = true;
bool    about_is_show = false;
LoopState    one_loop = LS_NORMAL;
char    cur_icon[] = " >";
const char *gs_path = nullptr;

static std::random_device rd;
static std::random_device::result_type seed = rd();
static std::mt19937 randomgen(seed);
static std::uniform_int_distribution<int> gdist(0, INT_MAX);

static int Rand()
{
    return gdist(randomgen);
}

#define SOUND_COUNT ((int)sounds.size())
#define MAX_SOUND_INDEX (SOUND_COUNT - 1)

#define list_end \
    ((list_start + MAX_LIST_INDEX > MAX_SOUND_INDEX ? MAX_SOUND_INDEX : list_start + MAX_LIST_INDEX))

static void set_font_color(WORD color);

// move cursor
static void move_screen_cur(short row, short col)
{
    COORD point = {col, row};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), point);
}

// about - print a centered line with background color, padded to con_width
static void print_centered_line(const char *text)
{
    int len = (int)strlen(text);
    int pad_left = (con_width - len) / 2;
    int pad_right = con_width - len - pad_left;
    if (pad_left < 0) pad_left = 0;
    if (pad_right < 0) pad_right = 0;
    char *line = (char*)_alloca(con_width + 2);
    memset(line, ' ', con_width);
    if (len > con_width) len = con_width;
    memcpy(line + pad_left, text, len);
    line[con_width] = '\n';
    line[con_width + 1] = '\0';
    printf(line);
}

static void switchabout()
{    
    system("cls");
    if (about_is_show)
    {        
        about_is_show = false;       
        if (!visualization_isopen)
        {
            needupdatelist = true;
            needupdatevolume = true;
            needupdateprocbar = true;
            needshowhelp = true;            
        }
        force_refresh_procbar = true;
        return;
    }
    
    about_is_show = true;

    // Vertical centering: about block is 6 lines, center in window
    int top_pad = (con_height - 12) / 2;
    if (top_pad < 0) top_pad = 0;
    for (int i = 0; i < top_pad; i++)
        putchar('\n');

    set_font_color(FONT_BLACK | BG_WHITE);
    print_centered_line("");
    print_centered_line("[ CONSOLE PLAYER ]");
    print_centered_line("");
    print_centered_line("Author: Zhang Li");
    print_centered_line("Email: zlvbvbzl@gmail.com");
    print_centered_line("");
    set_font_color(FONT_WHITE);

    printf("\n\n\n\n");
    print_centered_line("< [A] Return >");
   
    move_screen_cur((short)(con_height - 1), (short)(con_width - 1));
}

// up scroll
static void uplist()
{
	if (list_end + 1 < SOUND_COUNT)
	{
		list_start++;
	}
    else
    {
        list_start = 0;
        list_cur = 0;
    }
    needupdatelist = true;
}

// down scroll
static void downlist()
{
	if (list_start - 1 >= 0)
	{
		list_start--;
	}
    else
    {
        list_start = MAX_SOUND_INDEX < MAX_LIST_INDEX? 0 : MAX_SOUND_INDEX - MAX_LIST_INDEX;  
        list_cur = list_end < MAX_LIST_INDEX? list_end : MAX_LIST_INDEX;  
    }
    needupdatelist = true;
}

void pagedown()
{
    if (list_end + MAX_LIST_COUNT > MAX_SOUND_INDEX)
    {
        list_start = (MAX_SOUND_INDEX >= MAX_LIST_INDEX)?(MAX_SOUND_INDEX - MAX_LIST_INDEX):0;
    }
    else
    {  
        list_start += MAX_LIST_COUNT;
    }
    needupdatelist = true;
}

static void pageup()
{
    if (list_start - MAX_LIST_COUNT < 0)
    {
        list_start = 0;
    }
    else
    {  
        list_start -= MAX_LIST_COUNT;
    }
    needupdatelist = true;
}

// add music file to list
static void add_sound(const char *path)
{
    char* upperstr = _strdup(path);
    char* p = upperstr;
    while (*p)
    {
        *p = (char)toupper(*p);
        p++;
    }

    bool supported = false;
    for (int i = 0; i < INFINITE; i++)
    {
        if (FORMATS[i][0] == '#')
            break;

        if (ff_check_suffix(upperstr, FORMATS[i]) == 0)
        {
            supported = true;
            break;
        }
    }

    if (supported)
        sounds.push_back(_strdup(path));		        

    printf("%-0.78s\r",path);

    free(upperstr);
}

// Get display name from file path: strip leading ".\" and directory, keep filename without extension
static char* make_display_name_from_path(const char *path)
{
    // Skip leading ".\" or "./"
    const char *p = path;
    if (p[0] == '.' && (p[1] == '\\' || p[1] == '/'))
        p += 2;

    // Find last separator to get filename
    const char *fname = p;
    for (const char *c = p; *c; c++)
    {
        if (*c == '\\' || *c == '/')
            fname = c + 1;
    }

    // Copy filename, remove extension
    char *name = _strdup(fname);
    char *dot = strrchr(name, '.');
    if (dot)
        *dot = '\0';
    return name;
}

// Convert wide string to console codepage (multibyte)
static char* wide_to_mb(const wchar_t *wstr, int wlen)
{
    // Convert to console's codepage for display
    UINT cp = GetConsoleOutputCP();
    int mblen = WideCharToMultiByte(cp, 0, wstr, wlen, NULL, 0, NULL, NULL);
    if (mblen <= 0) return NULL;
    char *mb = (char*)malloc(mblen + 1);
    WideCharToMultiByte(cp, 0, wstr, wlen, mb, mblen, NULL, NULL);
    mb[mblen] = '\0';
    return mb;
}

// Convert UTF-8 string to console codepage
static char* utf8_to_mb(const char *utf8str)
{
    // UTF-8 -> wchar
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, NULL, 0);
    if (wlen <= 0) return _strdup(utf8str);
    wchar_t *wbuf = (wchar_t*)_alloca(wlen * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, wbuf, wlen);
    // wchar -> console CP
    char *result = wide_to_mb(wbuf, wlen - 1);
    return result ? result : _strdup(utf8str);
}

// Convert tag data to console-displayable string based on its encoding
static char* decode_tag_data(const FMOD_TAG *tag)
{
    if (tag->datatype == FMOD_TAGDATATYPE_STRING)
        return _strdup((const char*)tag->data);
    else if (tag->datatype == FMOD_TAGDATATYPE_STRING_UTF8)
        return utf8_to_mb((const char*)tag->data);
    else if (tag->datatype == FMOD_TAGDATATYPE_STRING_UTF16)
    {
        const wchar_t *wstr = (const wchar_t*)tag->data;
        int wlen = (int)(tag->datalen / sizeof(wchar_t));
        if (wlen > 0 && wstr[0] == 0xFEFF) { wstr++; wlen--; }
        return wide_to_mb(wstr, wlen);
    }
    else if (tag->datatype == FMOD_TAGDATATYPE_STRING_UTF16BE)
    {
        int wlen = (int)(tag->datalen / sizeof(wchar_t));
        wchar_t *wbuf = (wchar_t*)_alloca(wlen * sizeof(wchar_t));
        const unsigned char *src = (const unsigned char*)tag->data;
        for (int i = 0; i < wlen; i++)
            wbuf[i] = (wchar_t)(src[i*2] << 8 | src[i*2+1]);
        const wchar_t *wstr = wbuf;
        if (wlen > 0 && wstr[0] == 0xFEFF) { wstr++; wlen--; }
        return wide_to_mb(wstr, wlen);
    }
    return NULL;
}

// Read title, artist and album from all tags (case-insensitive matching)
static void read_song_tags(FMOD_SOUND *fsound, char **out_title, char **out_artist, char **out_album)
{
    *out_title = NULL;
    *out_artist = NULL;
    *out_album = NULL;

    int numtags = 0;
    FMOD_Sound_GetNumTags(fsound, &numtags, NULL);

    // Priority: album artist > artist
    char *album_artist = NULL;
    char *artist = NULL;

    for (int i = 0; i < numtags; i++)
    {
        FMOD_TAG tag;
        if (FMOD_Sound_GetTag(fsound, NULL, i, &tag) != FMOD_OK)
            continue;
        if (!tag.name)
            continue;

        // Title
        if (!*out_title &&
            (_stricmp(tag.name, "TITLE") == 0 || _stricmp(tag.name, "TIT2") == 0))
        {
            *out_title = decode_tag_data(&tag);
        }
        // Album
        else if (!*out_album &&
            (_stricmp(tag.name, "ALBUM") == 0 ||
             _stricmp(tag.name, "TALB") == 0 ||
             _stricmp(tag.name, "WM/AlbumTitle") == 0))
        {
            *out_album = decode_tag_data(&tag);
        }
        // Album artist (higher priority)
        else if (!album_artist &&
            (_stricmp(tag.name, "ALBUMARTIST") == 0 ||
             _stricmp(tag.name, "ALBUM ARTIST") == 0 ||
             _stricmp(tag.name, "TPE2") == 0 ||
             _stricmp(tag.name, "aART") == 0 ||
             _stricmp(tag.name, "WM/AlbumArtist") == 0))
        {
            album_artist = decode_tag_data(&tag);
        }
        // Artist (lower priority fallback)
        else if (!artist &&
            (_stricmp(tag.name, "ARTIST") == 0 ||
             _stricmp(tag.name, "TPE1") == 0 ||
             _stricmp(tag.name, "Author") == 0))
        {
            artist = decode_tag_data(&tag);
        }
    }

    // Prefer album artist, fallback to artist
    if (album_artist && album_artist[0])
    {
        *out_artist = album_artist;
        if (artist) free(artist);
    }
    else
    {
        *out_artist = artist;
        if (album_artist) free(album_artist);
    }
}

// Build display names from tags or filenames
static void build_display_names()
{
    printf("Reading tags...\n");
    for (int i = 0; i < SOUND_COUNT; i++)
    {
        printf("%-0.78s\r", sounds[i]);
        FMOD_SOUND *fsound = NULL;
        // Open sound in non-playing mode just to read tags
        FMOD_RESULT result = FMOD_System_CreateSound(fmodwrap::kFmod_system, sounds[i],
            FMOD_CREATESTREAM | FMOD_OPENONLY, NULL, &fsound);
        
        char *title = NULL;
        char *artist = NULL;
        char *album = NULL;
        if (result == FMOD_OK && fsound)
        {
            read_song_tags(fsound, &title, &artist, &album);
            FMOD_Sound_Release(fsound);
        }

        char *display = NULL;
        if (title && title[0])
        {
            bool has_artist = (artist && artist[0]);
            bool has_album  = (album && album[0]);
            if (has_artist || has_album)
            {
                // Build: [Artist - ] Title [ [Album]]
                size_t len = strlen(title) + 4;
                if (has_artist) len += strlen(artist) + 3; // "Artist - "
                if (has_album)  len += strlen(album) + 3;  // " [Album]"
                display = (char*)malloc(len);
                if (has_artist && has_album)
                    sprintf_s(display, len, "%s - %s [%s]", artist, title, album);
                else if (has_artist)
                    sprintf_s(display, len, "%s - %s", artist, title);
                else
                    sprintf_s(display, len, "%s [%s]", title, album);
            }
            else
            {
                display = _strdup(title);
            }
        }
        else
        {
            display = make_display_name_from_path(sounds[i]);
        }

        display_names.push_back(display);
        if (title) free(title);
        if (artist) free(artist);
        if (album) free(album);
    }
}

// search disk for generating play-list
static void create_list()
{
	printf("Scaning folders...\n");
    ff_find(gs_path, add_sound);
    build_display_names();
	system("cls");
	if (SOUND_COUNT == 0)
	{
		printf("Can not find any music.\nPress any key to exit.");
		while (!_kbhit())
		{
			Sleep(100);
		}
		exit(0);
	}
}

// generate processbar
static void show_processbar(short x, short y, int size)
{
    const int probar_length = size;
    char *probar = (char*)_alloca(probar_length + 1);
    memset(probar, PROCESS_CHAR, probar_length);
    probar[probar_length] = '\0';

	// moving bar-block    
	if (current_sound >= 0 && sound_playing)
	{
        static int pos = 0;
        int new_pos = int(player->elapse() * float(probar_length) / sound_playing->length());
        if (!force_refresh_procbar)
        {
            if (new_pos == pos)
                return;
            pos = new_pos;
            if (pos > probar_length - 1)
                return;
        }
        force_refresh_procbar = false;
        move_screen_cur(x, y);
        probar[pos] = '\0';
        set_font_color(FONT_BLACK | BG_WHITE);
        printf(probar);
        set_font_color(FONT_WHITE);
        probar[pos] = PROCESS_CHAR;
        printf(probar + pos);
	}
    move_screen_cur((short)(con_height - 1), (short)(con_width - 1));
}

static void show_volume(short x, short y)
{
    if (!needupdatevolume)
        return;
    needupdatevolume = false;

    move_screen_cur(x, y);

    // get volume
    float vval = fmodwrap::master_volume() * float(VOLUME_LENGTH);
    putchar('[');
    for (int k = 0; k <= VOLUME_LENGTH; ++k)
    {
        if ((vval - k < 0.00001f && vval >= k) 
            || (k - vval < 0.00001f && vval <= k))
            putchar(VOLUME_POSCHAR);
        else
            putchar(VOLUME_CHAR);
    }
    putchar(']');
}

static void show_help(short x, short y)
{	
    if (!needshowhelp)
        return;
    needshowhelp = false;

    move_screen_cur(x, y);
	printf(CONTROLPANEL_TEXT);	
} 

static void set_font_color(WORD color)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

static void show_list()
{
    if (!needupdatelist)
        return;
    needupdatelist = false;

	// move cursor to left-up bound
    move_screen_cur(0, 0);

    // dynamic title
    int tw = con_width;
    char *title_line = (char*)_alloca(tw + 2);

    // Line 1: blank
    memset(title_line, ' ', tw);
    title_line[tw] = '\n'; title_line[tw + 1] = '\0';
    printf(title_line);

    // Line 2: "   CONSOLE PLAYER"
    memset(title_line, ' ', tw);
    const char *ttl = "   CONSOLE PLAYER";
    int ttl_len = (int)strlen(ttl);
    if (ttl_len > tw) ttl_len = tw;
    memcpy(title_line, ttl, ttl_len);
    title_line[tw] = '\n'; title_line[tw + 1] = '\0';
    printf(title_line);

    // Line 3: underline
    memset(title_line, '_', tw);
    title_line[tw] = '\n'; title_line[tw + 1] = '\0';
    printf(title_line);

	// show play-list
    char *fmt = (char*)_alloca(32);
    _snprintf_s(fmt, 32, 31, "%%-%d.%ds\n", tw - 1, tw - 1);

	for (int i = list_start; i <= list_end && i < SOUND_COUNT; i++)
	{		    
        bool cur_on = list_cur == i - list_start;
        bool playing_line = current_sound == i;

        if (cur_on)    
            set_font_color(FONT_BLACK | BG_WHITE);
        else if (playing_line)
            set_font_color(FONT_LIGHT_BLUE);
        else
            set_font_color(FONT_WHITE);

        char lmode = ' ';
        if (playing_line)
        {
            if (one_loop == LS_NORMAL)
            {
                // pass
            }
            else if (one_loop == LS_ONE)
            {
                lmode = 'L';
            }
            else if (one_loop == LS_RAND)
            {
                lmode = 'R';
            }
        }
        else
        {
            // pass
        }
        
        const char *name = display_names[i];
		char *npath = (char*)_alloca(tw + 1);
        memset(npath, 0, tw + 1);
        _snprintf_s(npath, tw + 1, tw, "%c %d %s", lmode, i + 1, name);
        printf(fmt, npath);
		
        set_font_color(FONT_WHITE);
	}	
}

static void make_visualizion(char key )
{
    char keys[2] = {key, '\0'};
    if (strstr(VIS_CHAR, keys) && key != 0)
        vis_char = key;

    
    player->GetSpectrum();
    float *leftchannel = player->Fftparameter()->spectrum[0];
    float *rightchannel = NULL;
    if (player->sound()->channels() >= 2)
        rightchannel = player->Fftparameter()->spectrum[1];

    float max = 0.0f;
    for (int j = 0; j < player->Fftparameter()->length; j++)
    {
        if (leftchannel[j] > max)
        {
            max = leftchannel[j];
        }
    }
    if (max > 0.0001f)
    {
        for (int i = 0; i < player->Fftparameter()->length; i++)
        {
            float val = leftchannel[i];
            if (rightchannel)
                val += rightchannel[i];
            fft_rate[i] = 10.0f * (float)log10(val) * 2.0f;
        }
    }
}

// music visualization
static void show_visualization(short row, short col)
{
    move_screen_cur(row, col);
    const int width = con_width - 1;
    const int height = con_height - 1;
    
    char **screenlines = (char**)_alloca(width * sizeof(char*));
    for (int i = 0; i < width; i++)
        screenlines[i] = (char*)_alloca(height);

    float ff_rate_step = (float)std::size(fft_rate) / width / 2.0f;
    for (int i = 0; i < width; i++)
	{
		float val = fft_rate[int(ff_rate_step * i)];
		val = val / -5.0f;
		if (val > height)
			val = (float)height;
		char *line = screenlines[i];
		int iv = height - (int)val;
        if (iv < height)
            memset(line, ' ', height - iv);
		memset(line + height - iv, vis_char, iv);		
	}
        
    char *outstr = (char*)_alloca(width + 1);
    for (int i = 0; i < height; i++)
    {
        if (i > height * 0.8f)
            set_font_color(FONT_LIGHT_GREEN);
        else if (i > height * 0.6f)
            set_font_color(FONT_LIGHT_YELLOW);
        else
            set_font_color(FONT_LIGHT_RED);
        for (int j = 0; j < width; j++)
        {           
            outstr[j] = screenlines[j][i];
        }
        outstr[width] = '\0';        
        printf("%s\n", outstr);
    }

	set_font_color(FONT_WHITE);    
}

static void render()
{
    if (about_is_show)
    {
        return;
    }
	else if (visualization_isopen)
	{
		show_visualization(0, 0);
        show_processbar((short)(con_height - 1), 0, con_width - 1);
		return;
	}    
    short help_row = (short)(con_height - 4);
    short proc_row = (short)(help_row - 1);
    if (needupdatelist)
        force_refresh_procbar = true;
	show_list();
	show_processbar(proc_row, 0, con_width);
	show_help(help_row, 0);
    show_volume(help_row, 40);
}

static void on_play_end(fmodwrap::Player *)
{
    need_next = true;   
}

// which music was playing on last quitting
static void recored_list_log(int curidx)
{
    std::string listpath(gs_path);
    listpath += "\\list.record";
    FILE* pf = _fsopen(listpath.c_str(), "wb+", _SH_DENYNO);
    fwrite(&curidx, sizeof(curidx), 1, pf);
    fclose(pf);
    pf = NULL;
}

// write the index of playing music to disk
static int restore_list_log()
{
    std::string listpath(gs_path);
    listpath += "\\list.record";
    FILE* pf = _fsopen(listpath.c_str(), "rb", _SH_DENYNO);
    if (pf)
    {
        int curidx = 0;
        fread(&curidx, sizeof(int), 1, pf);
        fclose(pf);
        pf = NULL;
        return curidx - 1;
    }
    return -1;
}

static void cur_down()
{
    if ((size_t)list_cur < MAX_LIST_INDEX && (size_t)list_cur + 1 < SOUND_COUNT)
    {
        ++list_cur;
        needupdatelist = true;
    }
    else
    {
        uplist();
    }
}

static void cur_up()
{
    if (list_cur - 1 >= 0)
    {
        --list_cur;
        needupdatelist = true;
    }
    else
    {
        downlist();    
    }
}

static void play();

static void next(bool force)
{
    int p = current_sound;
    if (force)
    {
        if (one_loop == LS_RAND)
        {
            p = Rand();
        }
        else
        {
            p++;
        }
    }
    else
    {
        if (one_loop == LS_NORMAL)
        {
            p++;
        }
        else if (one_loop == LS_RAND)
        {
            p = Rand();
        }
    }

    current_sound = p % SOUND_COUNT;
    play();  
    if (current_sound > list_end)
        uplist();
    else
        needupdatelist = true;
}

static void prev()
{
    if (one_loop == LS_NORMAL || one_loop == LS_ONE)
    {
        int p = current_sound + MAX_SOUND_INDEX;
        current_sound = p % SOUND_COUNT;
    }
    else if (one_loop == LS_RAND)
    {
        current_sound = Rand() % SOUND_COUNT;
    }
    play();  
    if (current_sound < list_start)
        downlist();
    else
        needupdatelist = true;
}

static void switch_loop()
{
    int c = one_loop;
    one_loop = (LoopState)((++c) % LS_COUNT);
    needupdatelist = true;
}

static void play()
{
    if (!player)
        return;

    player->Unload();
    if (sound_playing)
        fmodwrap::DestroySound(sound_playing);
    recored_list_log(current_sound);
    sound_playing = fmodwrap::CreateSound(sounds[current_sound]);
    if (!sound_playing)
    {
        next(true);
        return;
    }

    player->Load(sound_playing);
    player->Play(false);

    while (list_start + MAX_LIST_INDEX < current_sound)
        uplist();

    while (list_start > current_sound)
        downlist();

    need_next = false;

    char title[256];
    sprintf_s(title, sizeof(title), "%-0.60s - %s", display_names[current_sound], "CONSOLE PLAYER");
    SetConsoleTitleA(title);
}

// pause
static void pause_or_continue()
{
    if(player && SOUND_COUNT > 0)
    {
        player->paused(!player->paused());
    }
    needupdatelist = true;
}

static void play_cur()
{
    if (SOUND_COUNT > 0)
    {
        current_sound = list_cur + list_start;
        play();
    }
    needupdatelist = true;
}

static void volume_loud()
{
    fmodwrap::master_volume(fmodwrap::master_volume() + 0.05f);
    needupdatevolume = true;
}

static void volume_quiet()
{
    fmodwrap::master_volume(fmodwrap::master_volume() - 0.05f);
    needupdatevolume = true;
}

static void update( char key )
{
    fmodwrap::Update();
    if (player && need_next)
        next(false);
    else if (player && player->playing() && visualization_isopen)
        make_visualizion(key);

    // Detect console size change
    int old_w = con_width, old_h = con_height;
    query_console_size();
    if (old_w != con_width || old_h != con_height)
    {
        // Clamp list_cur to new MAX_LIST_INDEX
        if (list_cur > MAX_LIST_INDEX)
            list_cur = MAX_LIST_INDEX;
        system("cls");
        needupdatelist = true;
        needupdatevolume = true;
        needupdateprocbar = true;
        needshowhelp = true;
        force_refresh_procbar = true;
    }

    Sleep(sleep_time);
    if (visualization_isopen)
        sleep_time = 10;
    else
        sleep_time = 5;
}

static void open_close_vis()
{
    visualization_isopen = !visualization_isopen;    
    if (!about_is_show)
    {
        system("cls");
        needupdatelist = true;
        needupdatevolume = true;
        needupdateprocbar = true;
        needshowhelp = true;
        force_refresh_procbar = true;
    }
}

static void init()
{
    SetConsoleTitleA("CONSOLE PLAYER");

    // Query current console size
    query_console_size();

    if (con_width <= 80 && con_height <= 25)
    {
        // Small console (likely legacy conhost): set to 80x25 and lock size
        SMALL_RECT minRect = {0, 0, 0, 0};
        SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &minRect);
        COORD size = {80, 25};
        SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), size);
        SMALL_RECT rect = {0, 0, 79, 24};
        SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &rect);

        HWND hwnd = GetConsoleWindow();
        if (hwnd)
        {
            DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
            dwStyle &= ~(WS_SIZEBOX);
            dwStyle &= ~(WS_MAXIMIZEBOX);
            SetWindowLong(hwnd, GWL_STYLE, dwStyle);
        }
        con_width = 80;
        con_height = 25;
    }
    // else: wide terminal (Windows Terminal etc.), use current size as-is

    fmodwrap::Open();
    create_list();

    // Re-query after create_list, terminal size may have changed
    query_console_size();
    player = fmodwrap::CreatePlayer();
    player->set_endcallback(on_play_end);
    fmodwrap::master_volume(0.5f);
    current_sound = restore_list_log();    
}

// on quit
static void final()
{
    fmodwrap::DestroyPlayer(player);
    if (sound_playing)
        fmodwrap::DestroySound(sound_playing);
    fmodwrap::Close();

    for each (char* path in sounds)
    {
        free(path);
    }
    for each (char* name in display_names)
    {
        free(name);
    }
}

// on key down
static char process_input()
{
    char key = -1;
    if (_kbhit())
    {
        key = (char)_getch();
        sleep_time = 0;
    }
    switch (key)
    {
    case KEY_LIST_PREV:
        cur_up();
        break;
    case KEY_LIST_NEXT:
        cur_down();
        break;
    case KEY_VOLUME_LOUD:
        volume_loud();
        break;
    case KEY_VOLUME_QUIET:
        volume_quiet();
        break;
    case KEY_PLAY:
        play_cur();
        break;
    case KEY_PAUSE:
        pause_or_continue();
        break;
    case KEY_VISUALIZATION:
        open_close_vis();
        break;
    case KEY_PLAY_NEXT:
        next(true);
        break;
    case KEY_PLAY_PREV:
        prev();
        break;
    case KEY_ABOUT:
        switchabout();
        break;
    case KEY_PGUP:
    case KEY_J:
        pageup();
        break;
    case KEY_PGDOWN:
    case KEY_K:
        pagedown();
        break;
    case KEY_LOOP:
        switch_loop();
        break;
    default:
        break;
    }
    return key;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        gs_path = ".";
    }
    else
    {
        gs_path = argv[1];
    }
    
    init();
	for (;;)
	{
		render();
        char key = process_input();
        if (key == KEY_QUIT)
            break;
        update(key);
	}
    final();
}
