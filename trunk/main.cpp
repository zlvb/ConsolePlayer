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
#include <math.h>
#include <share.h>

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

#define MAX_LIST_INDEX  16
#define MAX_LIST_COUNT  (MAX_LIST_INDEX + 1) 
#define MAX_NAME_LENGTH 77
#define VOLUME_LENGTH   20
#define PROCESS_CHAR    '|'
#define VOLUME_CHAR     '-'
#define PROCESS_POSCHAR '|'
#define VOLUME_POSCHAR  '|'

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
"[↑↓] Select       [E] Volume up\n"                        \
"[←→] Prev/Next    [Q] Volume down     [X] Visualization\n"\
"[SPACE] Pause       [RETURN] Play       [PgUp/PgDn] Scroll list\n"\
"[L] Loop mode       [A] About           [Esc] Quit"

#define TITLE_TEXT \
"                                                                                "\
"   CONSOLE PLAYER                                                               "\
"________________________________________________________________________________"

#define VIS_CHAR "~!@#$%^&-*()_+|<>{}:\"`=\\[];\',./?"

const char* FORMATS[] = {"MP3", "WAV", "OGG", "WMA", "#"};

std::vector<char*> sounds;
fmodwrap::Sound     *sound_playing = NULL;
fmodwrap::Player    *player = NULL;
int     list_cur = 0;
int     current_sound = -1;
int     list_start = 0;
float   fft_rate[512] = {0};
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
bool    one_loop = false;
char    cur_icon[] = " >";

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

// about
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
    printf("\n\n\n\n\n\n\n\n");
    set_font_color(FONT_BLACK | BG_WHITE);
    printf(ABOUT_TEXT);
    set_font_color(FONT_WHITE);
    printf("\n\n\n\n"
           "                                < [A] Return >\n");
   
    move_screen_cur(24, 79);
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

// search disk for generating play-list
static void create_list()
{
	printf("Scaning folders...\n");
    ff_find(".", add_sound);
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
template <int size>
static void show_processbar(short x, short y)
{
    const int probar_length = size;
    char probar[probar_length + 1];
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
    move_screen_cur(24, 79);
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
	printf(TITLE_TEXT);

	// show play-list
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

        if (playing_line && one_loop)
            putchar('L');
        else
            putchar(' ');
        
        const char *path = sounds[i] + 2; // +2去掉前面的".\"
		char npath[80] = {0};
        _snprintf_s(npath, sizeof(npath), MAX_NAME_LENGTH, " %d %s", i + 1, path);
        printf("%-79.79s", npath);
		
        set_font_color(FONT_WHITE);
	}	
}

static void make_visualizion(char key )
{
    char keys[2] = {key, '\0'};
    if (strstr(VIS_CHAR, keys))
        vis_char = key;

    float *leftchannel = NULL;
    float *rightchannel = NULL;
    leftchannel = player->GetSpectrum(0);
    if (player->sound()->channels() >= 2)
        rightchannel = player->GetSpectrum(1);

    float max = 0.0f;
    for (int j = 0; j < 512; j++)
    {
        if (leftchannel[j] > max)
        {
            max = leftchannel[j];
        }
    }
    if (max > 0.0001f)
    {
        for (int i = 0; i < 512; i++)
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
    const int width = 79;
    const int height = 24;
    char screenlines[width][height];
	
    float ff_rate_step = 512.0f / width;
    for (int i = 0; i < width; i++)
	{
		float val = fft_rate[int(ff_rate_step * i)];
		val = val / -5.0f;
		if (val > height)
			val = height;
		char *line = screenlines[i];
		int iv = height - (int)val;
        if (iv < height)
            memset(line, ' ', height - iv);
		memset(line + height - iv, vis_char, iv);		
	}
        
    for (int i = 0; i < height; i++)
    {
        if (i > height * 0.8f)
            set_font_color(FONT_LIGHT_GREEN);
        else if (i > height * 0.6f)
            set_font_color(FONT_LIGHT_YELLOW);
        else
            set_font_color(FONT_LIGHT_RED);
        char outstr[width+1];
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
        show_processbar<79>(24, 0);
		return;
	}    
	show_list();
	show_processbar<80>(20, 0);
	show_help(21, 0);
    show_volume(21, 40);
}

static void on_play_end(fmodwrap::Player *)
{
    need_next = true;   
}

// which music was playing on last quitting
static void recored_list_log(int curidx)
{
    FILE* pf = _fsopen("list.record", "wb+", _SH_DENYNO);
    fwrite(&curidx, sizeof(curidx), 1, pf);
    fclose(pf);
    pf = NULL;
}

// write the index of playing music to disk
static int restore_list_log()
{
    FILE* pf = _fsopen("list.record", "rb", _SH_DENYNO);
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
    if (force || !one_loop)
        p++;

    current_sound = p % SOUND_COUNT;
    play();  
    if (current_sound > list_end)
        uplist();
    else
        needupdatelist = true;
}

static void prev()
{
    int p = current_sound + MAX_SOUND_INDEX;
    current_sound = p % SOUND_COUNT;
    play();  
    if (current_sound < list_start)
        downlist();
    else
        needupdatelist = true;
}

static void switch_loop()
{
    one_loop = !one_loop;
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
    sprintf_s(title, sizeof(title), "%-0.60s - %s", sounds[current_sound] + 2, "CONSOLE PLAYER");
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
    COORD size = {80, 25};
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), size);
    SMALL_RECT rect = {0, 0, 80, 25};
    SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), false, &rect);
    fmodwrap::Open();
    create_list();
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
        pageup();
        break;
    case KEY_PGDOWN:
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

int main()
{
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
