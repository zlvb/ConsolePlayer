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

// 文件名用.c编译起来会稍微快一点点，没别的作用

#include "filefinder.h"
#include <stdio.h>
#include <Windows.h>

BOOL is_root(const char *path) 
{ 
    char root[4] = {0}; 
    memcpy(root,path,1);
    memcpy(root, ":\\", 2);
    return (strcmp(root,path)==0); 
}
// -------------------------------------------------------------------------------------------------
void ff_find(const char *path, on_find_callback onfind) 
{
    char finddir[MAX_PATH]; 
    WIN32_FIND_DATAA wfd; 
    HANDLE hFind;
    strcpy_s(finddir, sizeof(finddir), path); 
    if(!is_root(finddir)) 
        strcat_s(finddir, sizeof(finddir), "\\");
    strcat_s(finddir, sizeof(finddir), "*.*"); 
    hFind=FindFirstFileA(finddir,&wfd); 
    if(hFind==INVALID_HANDLE_VALUE) 
        return; 
    do
    { 
        if(wfd.cFileName[0]=='.')
            continue;
        if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            char finddir[MAX_PATH];
            if(is_root(path))
                sprintf_s(finddir,sizeof(finddir), "%s%s",path,wfd.cFileName);
            else
                sprintf_s(finddir,sizeof(finddir),"%s\\%s",path,wfd.cFileName);
            ff_find(finddir,onfind); 
        }
        else
        {
            char finddir[MAX_PATH];
            if(is_root(path))
                sprintf_s(finddir,sizeof(finddir),"%s%s",path,wfd.cFileName);
            else
                sprintf_s(finddir,sizeof(finddir),"%s\\%s",path,wfd.cFileName);
            onfind(finddir);
        }
    }
    while(FindNextFileA(hFind,&wfd));
    FindClose(hFind);
}
// -------------------------------------------------------------------------------------------------
int ff_check_suffix( const char *filename, const char *suffix )
{
    size_t len_filename = strlen(filename);
    size_t len_suffix = strlen(suffix);
    if (len_filename < len_suffix)
        return -1;
    return memcmp(&(filename[len_filename-len_suffix]), suffix, len_suffix);
}
// -------------------------------------------------------------------------------------------------
void ff_standardize_path( const char *path, char *out)
{
    const char *ptr = path;
    char last = '\0';
    while (*ptr)
    {
        char cur = *ptr;
        if (cur == '/')
            cur = '\\';
        if (last != cur || cur != '\\')
        {
            *out = cur;
            ++out;
        }
        last = cur;
        ++ptr;
    }
    *out = '\0';
}
