/***
A useful set of Windows API functions.

 * Enumerating and accessing windows, including sending keys.
 * Enumerating processes and querying their program name, memory used, etc.
 * Reading and Writing to the Registry
 * Copying and moving files, and showing drive information.
 * Launching processes and opening documents.
 * Monitoring filesystem changes.

@author Steve Donovan  (steve.j.donovan@gmail.com)
@copyright 2011
@license MIT/X11
@module winapi
*/
#line 16 "winapi.l.c"
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#ifdef __GNUC__
#include <winable.h> /* GNU GCC specific */
#endif
#include "Winnetwk.h"
#include <psapi.h>


#define WBUFF 2048
#define MAX_SHOW 100
#define THREAD_STACK_SIZE (1024 * 1024)
#define MAX_PROCESSES 1024
#define MAX_KEYS 512
#define FILE_BUFF_SIZE 2048
#define MAX_WATCH 20
#define MAX_WPATH 1024

#define TIMEOUT(timeout) timeout == 0 ? INFINITE : timeout

static wchar_t wbuff[WBUFF];

typedef LPCWSTR WStr;

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#ifdef WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif
typedef const char *Str;
typedef const char *StrNil;
typedef int Int;
typedef double Number;
typedef int Boolean;


#line 43 "winapi.l.c"

#include "wutils.h"

static WStr wstring(Str text) {
  return wstring_buff(text,wbuff,sizeof(wbuff));
}

/// Text encoding.
// @section encoding

/// set the current text encoding.
// @param e one of `CP_ACP` (Windows code page; default) and `CP_UTF8`
// @function set_encoding
static int l_set_encoding(lua_State *L) {
  int e = luaL_checkinteger(L,1);
  #line 56 "winapi.l.c"
  set_encoding(e);
  return 0;
}

/// get the current text encoding.
// @return either `CP_ACP` or `CP_UTF8`
// @function get_encoding
static int l_get_encoding(lua_State *L) {
  lua_pushinteger(L, get_encoding());
  return 1;
}

/// encode a string in another encoding.
// Note: currently there's a limit of about 2K on the string buffer.
// @param e_in `CP_ACP`, `CP_UTF8` or `CP_UTF16`
// @param e_out likewise
// @param text the string
// @function encode
static int l_encode(lua_State *L) {
  int e_in = luaL_checkinteger(L,1);
  int e_out = luaL_checkinteger(L,2);
  const char *text = luaL_checklstring(L,3,NULL);
  #line 75 "winapi.l.c"
  int ce = get_encoding();
  LPCWSTR ws;
  if (e_in != -1) {
    set_encoding(e_in);
    ws = wstring(text);
  } else {
    ws = (LPCWSTR)text;
  }
  if (e_out != -1) {
    set_encoding(e_out);
    push_wstring(L,ws);
  } else {
    lua_pushlstring(L,(LPCSTR)ws,wcslen(ws)*sizeof(WCHAR));
  }
  set_encoding(ce);
  return 1;
}

/// expand # unicode escapes in a string.
// @param text ASCII text with #XXXX, where XXXX is four hex digits. ## means # itself.
// @return text as UTF-8
// @see testu.lua
// @function utf8_expand
static int l_utf8_expand(lua_State *L) {
  const char *text = luaL_checklstring(L,1,NULL);
  #line 99 "winapi.l.c"
  int len = strlen(text), i = 0, enc = get_encoding();
  WCHAR wch;
  LPWSTR P = wbuff;
  if (len > sizeof(wbuff)) {
    return push_error_msg(L,"string too big");
  }
  while (i <= len) {
    if (text[i] == '#') {
      ++i;
      if (text[i] == '#') {
        wch = '#';
      } else
      if (len-i >= 4) {
        char hexnum[5];
        strncpy(hexnum,text+i,4);
        hexnum[4] = '\0';
        wch = strtol(hexnum,NULL,16);
        i += 3;
      } else {
        return push_error_msg(L,"bad # escape");
      }
    } else {
      wch = (WCHAR)text[i];
    }
    *P++ = wch;
    ++i;
  }
  *P++ = 0;
  set_encoding(CP_UTF8);
  push_wstring(L,wbuff);
  set_encoding(enc);
  return 1;
}

// forward reference to Process constructor
static int push_new_Process(lua_State *L,Int pid, HANDLE ph);

/// a class representing a Window.
// @type Window
#line 141 "winapi.l.c"

typedef struct {
  HWND hwnd;

} Window;


#define Window_MT "Window"

Window * Window_arg(lua_State *L,int idx) {
  Window *this = (Window *)luaL_checkudata(L,idx,Window_MT);
  luaL_argcheck(L, this != NULL, idx, "Window expected");
  return this;
}

static void Window_ctor(lua_State *L, Window *this, HWND h);

static int push_new_Window(lua_State *L,HWND h) {
  Window *this = (Window *)lua_newuserdata(L,sizeof(Window));
  luaL_getmetatable(L,Window_MT);
  lua_setmetatable(L,-2);
  Window_ctor(L,this,h);
  return 1;
}


static void Window_ctor(lua_State *L, Window *this, HWND h) {
    #line 142 "winapi.l.c"
    this->hwnd = h;
  }

  static lua_State *sL;

  static BOOL CALLBACK enum_callback(HWND hwnd,LPARAM data) {
    push_ref(sL,(Ref)data);
    push_new_Window(sL,hwnd);
    lua_call(sL,1,0);
    return TRUE;
  }

  /// the handle of this window.
  // @function get_handle
  static int l_Window_get_handle(lua_State *L) {
    Window *this = Window_arg(L,1);
    #line 157 "winapi.l.c"
    lua_pushnumber(L,(DWORD_PTR)this->hwnd);
    return 1;
  }

  /// get the window text.
  // @function get_text
  static int l_Window_get_text(lua_State *L) {
    Window *this = Window_arg(L,1);
    #line 164 "winapi.l.c"
    GetWindowTextW(this->hwnd,wbuff,sizeof(wbuff));
    return push_wstring(L,wbuff);
  }

  /// set the window text.
  // @function set_text
  static int l_Window_set_text(lua_State *L) {
    Window *this = Window_arg(L,1);
    const char *text = luaL_checklstring(L,2,NULL);
    #line 171 "winapi.l.c"
    SetWindowTextW(this->hwnd,wstring(text));
    return 0;
  }

  /// change the visibility, state etc
  // @param flags one of `SW_SHOW`, `SW_MAXIMIZE`, etc
  // @function show
  static int l_Window_show(lua_State *L) {
    Window *this = Window_arg(L,1);
    int flags = luaL_optinteger(L,2,SW_SHOW);
    #line 179 "winapi.l.c"
    ShowWindow(this->hwnd,flags);
    return 0;
  }

  /// get the position in pixels
  // @return left position
  // @return top position
  // @function get_position
  static int l_Window_get_position(lua_State *L) {
    Window *this = Window_arg(L,1);
    #line 188 "winapi.l.c"
    RECT rect;
    GetWindowRect(this->hwnd,&rect);
    lua_pushinteger(L,rect.left);
    lua_pushinteger(L,rect.top);
    return 2;
  }

  /// get the bounds in pixels
  // @return width
  // @return height
  // @function get_bounds
  static int l_Window_get_bounds(lua_State *L) {
    Window *this = Window_arg(L,1);
    #line 200 "winapi.l.c"
    RECT rect;
    GetWindowRect(this->hwnd,&rect);
    lua_pushinteger(L,rect.right - rect.left);
    lua_pushinteger(L,rect.bottom - rect.top);
    return 2;
  }

  /// is this window visible?
  // @function is_visible
  static int l_Window_is_visible(lua_State *L) {
    Window *this = Window_arg(L,1);
    #line 210 "winapi.l.c"
    lua_pushboolean(L,IsWindowVisible(this->hwnd));
    return 1;
  }

  /// destroy this window.
  // @function destroy
  static int l_Window_destroy(lua_State *L) {
    Window *this = Window_arg(L,1);
    #line 217 "winapi.l.c"
    DestroyWindow(this->hwnd);
    return 0;
  }

  /// resize this window.
  // @param x0 left
  // @param y0 top
  // @param w width
  // @param h height
  // @function resize
  static int l_Window_resize(lua_State *L) {
    Window *this = Window_arg(L,1);
    int x0 = luaL_checkinteger(L,2);
    int y0 = luaL_checkinteger(L,3);
    int w = luaL_checkinteger(L,4);
    int h = luaL_checkinteger(L,5);
    #line 228 "winapi.l.c"
    MoveWindow(this->hwnd,x0,y0,w,h,TRUE);
    return 0;
  }

  /// send a message.
  // @param msg the message
  // @param wparam
  // @param lparam
  // @return the result
  // @function send_message
  static int l_Window_send_message(lua_State *L) {
    Window *this = Window_arg(L,1);
    int msg = luaL_checkinteger(L,2);
    double wparam = luaL_checknumber(L,3);
    double lparam = luaL_checknumber(L,4);
    #line 239 "winapi.l.c"
    lua_pushinteger(L,SendMessage(this->hwnd,msg,(WPARAM)wparam,(LPARAM)lparam));
    return 1;
  }

  /// enumerate all child windows.
  // @param a callback which to receive each window object
  // @function enum_children
  static int l_Window_enum_children(lua_State *L) {
    Window *this = Window_arg(L,1);
    int callback = 2;
    #line 247 "winapi.l.c"
    Ref ref;
    sL = L;
    ref = make_ref(L,callback);
    EnumChildWindows(this->hwnd,&enum_callback,ref);
    release_ref(L,ref);
    return 0;
  }

  /// get the parent window.
  // @function get_parent
  static int l_Window_get_parent(lua_State *L) {
    Window *this = Window_arg(L,1);
    #line 258 "winapi.l.c"
    return push_new_Window(L,GetParent(this->hwnd));
  }

  /// get the name of the program owning this window.
  // @function get_module_filename
  static int l_Window_get_module_filename(lua_State *L) {
    Window *this = Window_arg(L,1);
    #line 264 "winapi.l.c"
    int sz = GetWindowModuleFileNameW(this->hwnd,wbuff,sizeof(wbuff));
    wbuff[sz] = 0;
    return push_wstring(L,wbuff);
  }

  /// get the window class name.
  // Useful to find all instances of a running program, when you
  // know the class of the top level window.
  // @function get_class_name
  static int l_Window_get_class_name(lua_State *L) {
    Window *this = Window_arg(L,1);
    #line 274 "winapi.l.c"
    GetClassNameW(this->hwnd,wbuff,sizeof(wbuff));
    return push_wstring(L,wbuff);
  }

  /// bring this window to the foreground.
  // @function set_foreground
  static int l_Window_set_foreground(lua_State *L) {
    Window *this = Window_arg(L,1);
    #line 281 "winapi.l.c"
    lua_pushboolean(L,SetForegroundWindow(this->hwnd));
    return 1;
  }

  /// get the associated process of this window
  // @function get_process
  static int l_Window_get_process(lua_State *L) {
    Window *this = Window_arg(L,1);
    #line 288 "winapi.l.c"
    DWORD pid;
    DWORD res = GetWindowThreadProcessId(this->hwnd,&pid);
    return push_new_Process(L,pid,NULL);
  }

  /// this window as string (up to 100 chars).
  // @function __tostring
  static int l_Window___tostring(lua_State *L) {
    Window *this = Window_arg(L,1);
    #line 296 "winapi.l.c"
    int ret;
    int sz = GetWindowTextW(this->hwnd,wbuff,sizeof(wbuff));
    if (sz > MAX_SHOW) {
      wbuff[MAX_SHOW] = '\0';
    }
    ret = push_wstring(L,wbuff);
    if (ret == 2) { // we had a conversion error
      lua_pushliteral(L,"");
    }
    return 1;
  }

  static int l_Window___eq(lua_State *L) {
    Window *this = Window_arg(L,1);
    Window *other = Window_arg(L,2);
    #line 309 "winapi.l.c"
    lua_pushboolean(L,this->hwnd == other->hwnd);
    return 1;
  }

#line 313 "winapi.l.c"

static const struct luaL_reg Window_methods [] = {
     {"get_handle",l_Window_get_handle},
   {"get_text",l_Window_get_text},
   {"set_text",l_Window_set_text},
   {"show",l_Window_show},
   {"get_position",l_Window_get_position},
   {"get_bounds",l_Window_get_bounds},
   {"is_visible",l_Window_is_visible},
   {"destroy",l_Window_destroy},
   {"resize",l_Window_resize},
   {"send_message",l_Window_send_message},
   {"enum_children",l_Window_enum_children},
   {"get_parent",l_Window_get_parent},
   {"get_module_filename",l_Window_get_module_filename},
   {"get_class_name",l_Window_get_class_name},
   {"set_foreground",l_Window_set_foreground},
   {"get_process",l_Window_get_process},
   {"__tostring",l_Window___tostring},
   {"__eq",l_Window___eq},
  {NULL, NULL}  /* sentinel */
};

static void Window_register (lua_State *L) {
  luaL_newmetatable(L,Window_MT);
  luaL_register(L,NULL,Window_methods);
  lua_pushvalue(L,-1);
  lua_setfield(L,-2,"__index");
  lua_pop(L,1);
}


#line 315 "winapi.l.c"

/// Manipulating Windows.
// @section Windows

/// find a window based on classname and caption
// @param cname class name (may be nil)
// @param wname caption (may be nil)
// @return @{Window}
// @function find_window
static int l_find_window(lua_State *L) {
  const char *cname = lua_tostring(L,1);
  const char *wname = lua_tostring(L,2);
  #line 324 "winapi.l.c"
  HWND hwnd;
  hwnd = FindWindow(cname,wname);
  return push_new_Window(L,hwnd);
}

/// makes a function that matches against window text
// @param text
// @function make_name_matcher

/// makes a function that matches against window class name
// @param text
// @function make_class_matcher

/// find a window using a condition function.
// @param match will return true when its argument is the desired window
// @return @{Window}
// @function find_window_ex

/// return all windows matching a condition.
// @param match will return true when its argument is the desired window
// @return a list of window objects
// @function find_all_windows

/// find a window matching the given text.
// @param text the pattern to match against the caption
// @return a window object.
// @function find_window_match

/// current foreground window.
// An example of setting the caption is @{caption.lua}
// @return @{Window}
// @function get_foreground_window
static int l_get_foreground_window(lua_State *L) {
  return push_new_Window(L, GetForegroundWindow());
}

/// the desktop window.
// @usage winapi.get_desktop_window():get_bounds()
// @return @{Window}
// @function get_desktop_window
static int l_get_desktop_window(lua_State *L) {
  return push_new_Window(L, GetDesktopWindow());
}

/// enumerate over all top-level windows.
// @param callback a function to receive each window object
// @function enum_windows
static int l_enum_windows(lua_State *L) {
  int callback = 1;
  #line 372 "winapi.l.c"
  Ref ref;
  sL = L;
  ref  = make_ref(L,callback);
  EnumWindows(&enum_callback,ref);
  release_ref(L,ref);
  return 0;
}

/// route callback dispatch through a message window.
// You need to do this when using Winapi in a GUI application,
// since it ensures that Lua callbacks happen in the GUI thread.
// @function use_gui
static int l_use_gui(lua_State *L) {
  make_message_window();
  return 0;
}

static INPUT *add_input(INPUT *pi, WORD vkey, BOOL up) {
  pi->type = INPUT_KEYBOARD;
  pi->ki.dwFlags =  up ? KEYEVENTF_KEYUP : 0;
  pi->ki.wVk = vkey;
  return pi+1;
}

// The Windows SendInput() is a low-level function, and you have to
// simulate things like uppercase directly. Repeated characters need
// an explicit 'key up' keystroke to work.
// see http://stackoverflow.com/questions/2167156/sendinput-isnt-sending-the-correct-shifted-characters
// this is a case where we have to convert the parameter directly, since
// it may be an integer (virtual key code) or string of characters.

/// send a string or virtual key to the active window.
// @{input.lua} shows launching a process, waiting for it to be
// ready, and sending it some keys
// @param text either a key (like winapi.VK_SHIFT) or a string
// @return number of keys sent, or nil if an error
// @return any error string
// @function send_to_window
static int l_send_to_window(lua_State *L) {
  const char *text;
  int vkey, len = MAX_KEYS;
  UINT res;
  SHORT last_vk = 0;
  INPUT *input, *pi;
  if (lua_isnumber(L,1)) {
    INPUT inp;
    ZeroMemory(&inp,sizeof(INPUT));
    vkey = lua_tointeger(L,1);
    add_input(&inp,vkey,lua_toboolean(L,2));
    SendInput(1,&inp,sizeof(INPUT));
    return 0;
  } else {
    text = lua_tostring(L,1);
    if (text == NULL) {
      return push_error_msg(L,"not a string or number");
    }
  }
  input = (INPUT *)malloc(sizeof(INPUT)*len);
  pi = input;
  ZeroMemory(input, sizeof(INPUT)*len);
  for(; *text; ++text) {
    SHORT vk = VkKeyScan(*text);
    if (last_vk == vk) {
      pi = add_input(pi,last_vk & 0xFF,TRUE);
    }
    if (vk & 0x100) pi = add_input(pi,VK_SHIFT,FALSE);
    pi = add_input(pi,vk & 0xFF,FALSE);
    if (vk & 0x100) pi = add_input(pi,VK_SHIFT,TRUE);
    last_vk = vk;
  }
  res = SendInput(((DWORD_PTR)pi-(DWORD_PTR)input)/sizeof(INPUT), input, sizeof(INPUT));
  free(input);
  if (res > 0) {
    lua_pushinteger(L,res);
    return 1;
  } else {
    return push_error(L);
  }
  return 0;
}

/// tile a group of windows.
// @param parent @{Window} (can use the desktop)
// @param horiz tile vertically by default
// @param kids a table of window objects
// @param bounds a bounds table (left,top,right,bottom) - can be nil
// @function tile_windows
static int l_tile_windows(lua_State *L) {
  Window *parent = Window_arg(L,1);
  int horiz = lua_toboolean(L,2);
  int kids = 3;
  int bounds = 4;
  #line 460 "winapi.l.c"
  RECT rt;
  HWND *kids_arr;
  int i,n_kids;
  LPRECT lpRect = NULL;
  if (! lua_isnoneornil(L,bounds)) {
    lua_pushvalue(L,bounds);
     lua_getfield(L,-1,"left"); rt.left=lua_tointeger(L,-1); lua_pop(L,1);
     lua_getfield(L,-1,"top"); rt.top=lua_tointeger(L,-1); lua_pop(L,1);
     lua_getfield(L,-1,"right"); rt.right=lua_tointeger(L,-1); lua_pop(L,1);
     lua_getfield(L,-1,"bottom"); rt.bottom=lua_tointeger(L,-1); lua_pop(L,1);
    lua_pop(L,1);
    lpRect = &rt;
  }
  n_kids = lua_objlen(L,kids);
  kids_arr = (HWND *)malloc(sizeof(HWND)*n_kids);
  for (i = 0; i < n_kids; ++i) {
    Window *w;
    lua_rawgeti(L,kids,i+1);
    w = Window_arg(L,-1);
    kids_arr[i] = w->hwnd;
  }
  TileWindows(parent->hwnd,horiz ? MDITILE_HORIZONTAL : MDITILE_VERTICAL, lpRect, n_kids, kids_arr);
  free(kids_arr);
  return 0;
}

/// Miscellaneous functions.
// @section miscellaneous

/// sleep and use no processing time.
// @param millisec sleep period
// @function sleep
static int l_sleep(lua_State *L) {
  int millisec = luaL_checkinteger(L,1);
  #line 493 "winapi.l.c"
  Sleep(millisec);
  return 0;
}

/// show a message box.
// @param caption for dialog
// @param msg the message
// @param btns (default 'ok') one of 'ok','ok-cancel','yes','yes-no',
// "abort-retry-ignore", "retry-cancel", "yes-no-cancel"
// @param icon (default 'information') one of 'information','question','warning','error'
// @return a string giving the pressed button: one of 'ok','yes','no','cancel',
// 'try','abort' and 'retry'
// @see message.lua
// @function show_message
static int l_show_message(lua_State *L) {
  const char *caption = luaL_checklstring(L,1,NULL);
  const char *msg = luaL_checklstring(L,2,NULL);
  const char *btns = luaL_optlstring(L,3,"ok",NULL);
  const char *icon = luaL_optlstring(L,4,"information",NULL);
  #line 508 "winapi.l.c"
  int res, type;
  WCHAR capb [512];
  type = mb_const(btns) | mb_const(icon);
  wstring_buff(caption,capb,sizeof(capb));
  res = MessageBoxW( NULL, wstring(msg), capb, type);
  lua_pushstring(L,mb_result(res));
  return 1;
}

/// make a beep sound.
// @param type (default 'ok'); one of 'information','question','warning','error'
// @function beep
static int l_beep(lua_State *L) {
  const char *icon = luaL_optlstring(L,1,"ok",NULL);
  #line 521 "winapi.l.c"
  return push_bool(L, MessageBeep(mb_const(icon)));
}

/// copy a file.
// @param src source file
// @param dest destination file
// @param fail_if_exists if true, then cannot copy onto existing file
// @function copy_file
static int l_copy_file(lua_State *L) {
  const char *src = luaL_checklstring(L,1,NULL);
  const char *dest = luaL_checklstring(L,2,NULL);
  int fail_if_exists = luaL_optinteger(L,3,0);
  #line 530 "winapi.l.c"
  return push_bool(L, CopyFile(src,dest,fail_if_exists));
}

/// output text to the system debugger.
// A uility such as [DebugView](http://technet.microsoft.com/en-us/sysinternals/bb896647)
// can show the output
// @param str text
// @function output_debug_string
static int l_output_debug_string(lua_State *L) {
   const char *str = luaL_checklstring(L,1,NULL);
   #line 539 "winapi.l.c"
   OutputDebugString(str);
   return 0;
}

/// move a file.
// @param src source file
// @param dest destination file
// @function move_file
static int l_move_file(lua_State *L) {
  const char *src = luaL_checklstring(L,1,NULL);
  const char *dest = luaL_checklstring(L,2,NULL);
  #line 548 "winapi.l.c"
  return push_bool(L, MoveFile(src,dest));
}

#define wconv(name) (name ? wstring_buff(name,w##name,sizeof(w##name)) : NULL)

/// execute a shell command.
// @param verb the action (e.g. 'open' or 'edit') can be nil.
// @param file the command
// @param parms any parameters (optional)
// @param dir the working directory (optional)
// @param show the window show flags (default is SW_SHOWNORMAL)
// @function shell_exec
static int l_shell_exec(lua_State *L) {
  const char *verb = lua_tostring(L,1);
  const char *file = luaL_checklstring(L,2,NULL);
  const char *parms = lua_tostring(L,3);
  const char *dir = lua_tostring(L,4);
  int show = luaL_optinteger(L,5,SW_SHOWNORMAL);
  #line 561 "winapi.l.c"
  WCHAR wverb[128], wfile[MAX_WPATH], wdir[MAX_WPATH], wparms[MAX_WPATH];
  int res = (DWORD_PTR)ShellExecuteW(NULL,wconv(verb),wconv(file),wconv(parms),wconv(dir),show) > 32;
  return push_bool(L, res);
}

/// copy text onto the clipboard.
// @param text the text
// @function set_clipboard
static int l_set_clipboard(lua_State *L) {
  const char *text = luaL_checklstring(L,1,NULL);
  #line 570 "winapi.l.c"
  HGLOBAL glob;
  LPWSTR p;
  int bufsize = 3*strlen(text);
  if (! OpenClipboard(NULL)) {
    return push_error(L);
  }
  EmptyClipboard();
  glob = GlobalAlloc(GMEM_MOVEABLE, bufsize);
  p = (LPWSTR)GlobalLock(glob);
  wstring_buff(text,p,bufsize);
  GlobalUnlock(glob);
  if (SetClipboardData(CF_UNICODETEXT,glob) == NULL) {
    CloseClipboard();
    return push_error(L);
  }
  CloseClipboard();
  return 0;
}

/// get the text on the clipboard.
// @return the text
// @function get_clipboard
static int l_get_clipboard(lua_State *L) {
  HGLOBAL glob;
  LPCWSTR p;
  if (! OpenClipboard(NULL)) {
    return push_error(L);
  }
  glob = GetClipboardData(CF_UNICODETEXT);
  if (glob == NULL) {
    CloseClipboard();
    return push_error(L);
  }
  p = GlobalLock(glob);
  push_wstring(L,p);
  GlobalUnlock(glob);
  CloseClipboard();
  return 1;
}

/// A class representing a Windows process.
// this example was [helpful](http://msdn.microsoft.com/en-us/library/ms682623%28VS.85%29.aspx)
// @type Process
#line 617 "winapi.l.c"

typedef struct {
  HANDLE hProcess;
  int pid;

} Process;


#define Process_MT "Process"

Process * Process_arg(lua_State *L,int idx) {
  Process *this = (Process *)luaL_checkudata(L,idx,Process_MT);
  luaL_argcheck(L, this != NULL, idx, "Process expected");
  return this;
}

static void Process_ctor(lua_State *L, Process *this, Int pid, HANDLE ph);

static int push_new_Process(lua_State *L,Int pid, HANDLE ph) {
  Process *this = (Process *)lua_newuserdata(L,sizeof(Process));
  luaL_getmetatable(L,Process_MT);
  lua_setmetatable(L,-2);
  Process_ctor(L,this,pid,ph);
  return 1;
}


static void Process_ctor(lua_State *L, Process *this, Int pid, HANDLE ph) {
    #line 618 "winapi.l.c"
    if (ph) {
      this->pid = pid;
      this->hProcess = ph;
    } else {
      this->pid = pid;
      this->hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
                                  PROCESS_VM_READ,
                                  FALSE, pid );
      }
  }

  /// get the name of the process.
  // @param full true if you want the full path; otherwise returns the base name.
  // @function get_process_name
  static int l_Process_get_process_name(lua_State *L) {
    Process *this = Process_arg(L,1);
    int full = lua_toboolean(L,2);
    #line 633 "winapi.l.c"
    HMODULE hMod;
    DWORD cbNeeded;
    wchar_t modname[MAX_PATH];

    if (EnumProcessModules(this->hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
      if (full) {
        GetModuleFileNameExW(this->hProcess, hMod, modname, sizeof(modname));
      } else {
        GetModuleBaseNameW(this->hProcess, hMod, modname, sizeof(modname));
      }
      return push_wstring(L,modname);
    } else {
      return push_error(L);
    }
  }

  /// kill the process.
  // @{test-spawn.lua} kills a launched process after a certain amount of output.
  // @function kill
  static int l_Process_kill(lua_State *L) {
    Process *this = Process_arg(L,1);
    #line 653 "winapi.l.c"
    TerminateProcess(this->hProcess,0);
    return 0;
  }

  /// get the working size of the process.
  // @return minimum working set size
  // @return maximum working set size.
  // @function get_working_size
  static int l_Process_get_working_size(lua_State *L) {
    Process *this = Process_arg(L,1);
    #line 662 "winapi.l.c"
    SIZE_T minsize, maxsize;
    GetProcessWorkingSetSize(this->hProcess,&minsize,&maxsize);
    lua_pushnumber(L,minsize/1024);
    lua_pushnumber(L,maxsize/1024);
    return 2;
  }

  /// get the start time of this process.
  // @return a table in the same format as os.time() and os.date() expects.
  // @function get_start_time
  static int l_Process_get_start_time(lua_State *L) {
    Process *this = Process_arg(L,1);
    #line 673 "winapi.l.c"
    FILETIME create,exit,kernel,user,local;
    SYSTEMTIME time;
    GetProcessTimes(this->hProcess,&create,&exit,&kernel,&user);
    FileTimeToLocalFileTime(&create,&local);
    FileTimeToSystemTime(&local,&time);
    #define set(name,val) lua_pushinteger(L,val); lua_setfield(L,-2,#name);
    lua_newtable(L);
    set(year,time.wYear);
    set(month,time.wMonth);
    set(day,time.wDay);
    set(hour,time.wHour);
    set(min,time.wMinute);
    set(sec,time.wSecond);
    #undef set
    return 1;
  }

  // MS likes to be different: the 64-bit value encoded in FILETIME
  // is defined as the number of 100-nsec intervals since Jan 1, 1601 UTC
  static double fileTimeToMillisec(FILETIME *ft) {
    ULARGE_INTEGER ui;
    ui.LowPart = ft->dwLowDateTime;
    ui.HighPart = ft->dwHighDateTime;
    return (double) (ui.QuadPart/10000);
  }

  /// elapsed run time of this process.
  // @return user time in msec
  // @return system time in msec
  // @function get_run_times
  static int l_Process_get_run_times(lua_State *L) {
    Process *this = Process_arg(L,1);
    #line 704 "winapi.l.c"
    FILETIME create,exit,kernel,user;
    GetProcessTimes(this->hProcess,&create,&exit,&kernel,&user);
    lua_pushnumber(L,fileTimeToMillisec(&user));
    lua_pushnumber(L,fileTimeToMillisec(&kernel));
    return 2;
  }

  static int push_wait_result(lua_State *L, DWORD res) {
    if (res == WAIT_OBJECT_0) {
        lua_pushvalue(L,1);
        lua_pushliteral(L,"OK");
        return 2;
    } else if (res == WAIT_TIMEOUT) {
        lua_pushvalue(L,1);
        lua_pushliteral(L,"TIMEOUT");
        return 2;
    } else {
        return push_error(L);
    }
  }

  /// wait for this process to finish.
  // @param timeout optional timeout in millisec; defaults to waiting indefinitely.
  // @return this process object
  // @return either "OK" or "TIMEOUT"
  // @function wait
  static int l_Process_wait(lua_State *L) {
    Process *this = Process_arg(L,1);
    int timeout = luaL_optinteger(L,2,0);
    #line 731 "winapi.l.c"
    return push_wait_result(L, WaitForSingleObject(this->hProcess, TIMEOUT(timeout)));
  }

  /// wait for this process to become idle and ready for input.
  // Only makes sense for processes with windows (will return immediately if not)
  // @param timeout optional timeout in millisec
  // @return this process object
  // @return either "OK" or "TIMEOUT"
  // @function wait_for_input_idle
  static int l_Process_wait_for_input_idle(lua_State *L) {
    Process *this = Process_arg(L,1);
    int timeout = luaL_optinteger(L,2,0);
    #line 741 "winapi.l.c"
    return push_wait_result(L, WaitForInputIdle(this->hProcess, TIMEOUT(timeout)));
  }

  /// exit code of this process.
  // (Only makes sense if the process has in fact finished.)
  // @return exit code
  // @function get_exit_code
  static int l_Process_get_exit_code(lua_State *L) {
    Process *this = Process_arg(L,1);
    #line 749 "winapi.l.c"
    DWORD code;
    GetExitCodeProcess(this->hProcess, &code);
    lua_pushinteger(L,code);
    return 1;
  }

  /// close this process handle.
  // @function close
  static int l_Process_close(lua_State *L) {
    Process *this = Process_arg(L,1);
    #line 758 "winapi.l.c"
    CloseHandle(this->hProcess);
    this->hProcess = NULL;
    return 0;
  }

  static int l_Process___gc(lua_State *L) {
    Process *this = Process_arg(L,1);
    #line 764 "winapi.l.c"
    if (this->hProcess != NULL)
      CloseHandle(this->hProcess);
    return 0;
  }
#line 768 "winapi.l.c"

static const struct luaL_reg Process_methods [] = {
     {"get_process_name",l_Process_get_process_name},
   {"kill",l_Process_kill},
   {"get_working_size",l_Process_get_working_size},
   {"get_start_time",l_Process_get_start_time},
   {"get_run_times",l_Process_get_run_times},
   {"wait",l_Process_wait},
   {"wait_for_input_idle",l_Process_wait_for_input_idle},
   {"get_exit_code",l_Process_get_exit_code},
   {"close",l_Process_close},
   {"__gc",l_Process___gc},
  {NULL, NULL}  /* sentinel */
};

static void Process_register (lua_State *L) {
  luaL_newmetatable(L,Process_MT);
  luaL_register(L,NULL,Process_methods);
  lua_pushvalue(L,-1);
  lua_setfield(L,-2,"__index");
  lua_pop(L,1);
}


#line 770 "winapi.l.c"

/// Working with processes.
// @section Processes

/// create a process object from the id.
// @param pid the process id
// @return @{Process}
// @function process_from_id
static int l_process_from_id(lua_State *L) {
  int pid = luaL_checkinteger(L,1);
  #line 778 "winapi.l.c"
  return push_new_Process(L,pid,NULL);
}

/// process id of current process.
// @return integer id
// @function get_current_pid
static int l_get_current_pid(lua_State *L) {
  lua_pushinteger(L,GetCurrentProcessId());
  return 1;
}

/// process object of the current process.
// @return @{Process}
// @function get_current_process
static int l_get_current_process(lua_State *L) {
  return push_new_Process(L,0,GetCurrentProcess());
}

/// get all process ids in the system.
// @{test-processes.lua} is a simple `ps` equivalent.
// @return an array of process ids.
// @function get_processes
static int l_get_processes(lua_State *L) {
  DWORD processes[MAX_PROCESSES], cbNeeded, nProcess;
  int i, k = 1;

  if (! EnumProcesses (processes,sizeof(processes),&cbNeeded)) {
    return push_error(L);
  }

  nProcess = cbNeeded/sizeof (DWORD);
  lua_newtable(L);
  for (i = 0; i < nProcess; i++) {
    if (processes[i] != 0) {
      lua_pushinteger(L,processes[i]);
      lua_rawseti(L,-2,k++);
    }
  }
  return 1;
}

/// wait for a group of processes.
// @{process-wait.lua} shows a number of processes launched
// in parallel
// @param processes an array of @{Process} objects
// @param all wait for all processes to finish (default false)
// @param timeout wait upto this time in msec (default infinite)
// @function wait_for_processes
static int l_wait_for_processes(lua_State *L) {
  int processes = 1;
  int all = lua_toboolean(L,2);
  int timeout = luaL_optinteger(L,3,0);
  #line 827 "winapi.l.c"
  int status, i;
  Process *p;
  int n = lua_objlen(L,processes);
  HANDLE handles[MAXIMUM_WAIT_OBJECTS];
  if (n > MAXIMUM_WAIT_OBJECTS) {
    return push_error_msg(L,"cannot wait on so many processes");
  }
  for (i = 0; i < n; i++) {
    lua_rawgeti(L,processes,i+1);
    p = Process_arg(L,-1);
    handles[i] = p->hProcess;
  }
  status = WaitForMultipleObjects(n, handles, all, TIMEOUT(timeout));
  status -= WAIT_OBJECT_0 + 1;
  if (status < 1 || status > n) {
    return push_error(L);
  } else {
    lua_pushinteger(L,status);
    return 1;
  }
}

// These functions are all run in background threads, and a little bit of poor man's
// OOP helps here. This is the base struct for describing threads with callbacks,
// which may have an associated buffer and handle.

#define callback_data_ \
  lua_State *L; \
  Ref callback; \
  char *buf; \
  int bufsz; \
  HANDLE handle;

typedef struct {
  callback_data_
} LuaCallback, *PLuaCallback;

void lcb_callback(void *lcb, lua_State *L, int idx) {
  LuaCallback *data = (LuaCallback*) lcb;
  data->L = L;
  data->callback = make_ref(L,idx);
  data->buf = NULL;
  data->handle = NULL;
}

BOOL lcb_call(void *data, int idx, Str text, int persist) {
  LuaCallback *lcb = (LuaCallback*)data;
  return call_lua(lcb->L,lcb->callback,idx,text,persist);
}

void lcb_allocate_buffer(void *data, int size) {
  LuaCallback *lcb = (LuaCallback*)data;
  lcb->buf = malloc(size);
  lcb->bufsz = size;
}

void lcb_free(void *data) {
  LuaCallback *lcb = (LuaCallback*)data;
  if (! lcb) return;
  if (lcb->buf) {
    free(lcb->buf);
    lcb->buf = NULL;
  }
  if (lcb->handle) {
    CloseHandle(lcb->handle);
    lcb->handle = NULL;
  }
  release_ref(lcb->L,lcb->callback);
}

/// Thread object. This is returned by the @{File.read_async} method and the @{make_timer},
// @{make_pipe_server} and @{watch_for_file_changes} functions. Useful to kill a thread
// and free associated resources.
// @type Thread
#line 905 "winapi.l.c"

typedef struct {
  LuaCallback *lcb;
  HANDLE thread;

} Thread;


#define Thread_MT "Thread"

Thread * Thread_arg(lua_State *L,int idx) {
  Thread *this = (Thread *)luaL_checkudata(L,idx,Thread_MT);
  luaL_argcheck(L, this != NULL, idx, "Thread expected");
  return this;
}

static void Thread_ctor(lua_State *L, Thread *this, PLuaCallback lcb, HANDLE thread);

static int push_new_Thread(lua_State *L,PLuaCallback lcb, HANDLE thread) {
  Thread *this = (Thread *)lua_newuserdata(L,sizeof(Thread));
  luaL_getmetatable(L,Thread_MT);
  lua_setmetatable(L,-2);
  Thread_ctor(L,this,lcb,thread);
  return 1;
}


static void Thread_ctor(lua_State *L, Thread *this, PLuaCallback lcb, HANDLE thread) {
    #line 906 "winapi.l.c"
    this->lcb = lcb;
    this->thread = thread;
  }

  /// suspend this thread.
  // @function suspend
  static int l_Thread_suspend(lua_State *L) {
    Thread *this = Thread_arg(L,1);
    #line 913 "winapi.l.c"
    return push_bool(L, SuspendThread(this->thread) >= 0);
  }

  /// resume a suspended thread.
  // @function resume
  static int l_Thread_resume(lua_State *L) {
    Thread *this = Thread_arg(L,1);
    #line 919 "winapi.l.c"
    return push_bool(L, ResumeThread(this->thread) >= 0);
  }

  /// kill this thread. Generally considered a 'nuclear' option, but
  // this implementation will free any associated callback references, buffers
  // and handles. @{test-timer.lua} shows how a timer can be terminated.
  // @function kill
  static int l_Thread_kill(lua_State *L) {
    Thread *this = Thread_arg(L,1);
    #line 927 "winapi.l.c"
    lcb_free(this->lcb);
    return push_bool(L, TerminateThread(this->thread,1));
  }

  /// set a thread's priority
  // @param p positive integer to increase thread priority
  // @function set_priority
  static int l_Thread_set_priority(lua_State *L) {
    Thread *this = Thread_arg(L,1);
    int p = luaL_checkinteger(L,2);
    #line 935 "winapi.l.c"
    return push_bool(L, SetThreadPriority(this->thread,p));
  }

  /// get a thread's priority
  // @function get_priority
  static int l_Thread_get_priority(lua_State *L) {
    Thread *this = Thread_arg(L,1);
    #line 941 "winapi.l.c"
    int res = GetThreadPriority(this->thread);
    if (res != THREAD_PRIORITY_ERROR_RETURN) {
      lua_pushinteger(L,res);
      return 1;
    } else {
      return push_error(L);
    }
  }

  static int l_Thread___gc(lua_State *L) {
    Thread *this = Thread_arg(L,1);
    #line 951 "winapi.l.c"
    // lcb_free(this->lcb); concerned that this cd kick in prematurely!
    CloseHandle(this->thread);
    return 0;
  }
#line 955 "winapi.l.c"

static const struct luaL_reg Thread_methods [] = {
     {"suspend",l_Thread_suspend},
   {"resume",l_Thread_resume},
   {"kill",l_Thread_kill},
   {"set_priority",l_Thread_set_priority},
   {"get_priority",l_Thread_get_priority},
   {"__gc",l_Thread___gc},
  {NULL, NULL}  /* sentinel */
};

static void Thread_register (lua_State *L) {
  luaL_newmetatable(L,Thread_MT);
  luaL_register(L,NULL,Thread_methods);
  lua_pushvalue(L,-1);
  lua_setfield(L,-2,"__index");
  lua_pop(L,1);
}


#line 957 "winapi.l.c"

typedef LPTHREAD_START_ROUTINE  TCB;

int lcb_new_thread(TCB fun, void *data) {
  LuaCallback *lcb = (LuaCallback*)data;
  HANDLE thread = CreateThread(NULL,THREAD_STACK_SIZE,fun,data,0,NULL);
  return push_new_Thread(lcb->L,lcb,thread);
}

#define lcb_buf(data) ((LuaCallback *)data)->buf
#define lcb_bufsz(data) ((LuaCallback *)data)->bufsz
#define lcb_handle(data) ((LuaCallback *)data)->handle

/// this represents a raw Windows file handle.
// The write handle may be distinct from the read handle.
// @type File
#line 976 "winapi.l.c"

typedef struct {
  callback_data_
  HANDLE hWrite;

} File;


#define File_MT "File"

File * File_arg(lua_State *L,int idx) {
  File *this = (File *)luaL_checkudata(L,idx,File_MT);
  luaL_argcheck(L, this != NULL, idx, "File expected");
  return this;
}

static void File_ctor(lua_State *L, File *this, HANDLE hread, HANDLE hwrite);

static int push_new_File(lua_State *L,HANDLE hread, HANDLE hwrite) {
  File *this = (File *)lua_newuserdata(L,sizeof(File));
  luaL_getmetatable(L,File_MT);
  lua_setmetatable(L,-2);
  File_ctor(L,this,hread,hwrite);
  return 1;
}


static void File_ctor(lua_State *L, File *this, HANDLE hread, HANDLE hwrite) {
    #line 977 "winapi.l.c"
    lcb_handle(this) = hread;
    this->hWrite = hwrite;
    this->L = L;
    lcb_allocate_buffer(this,FILE_BUFF_SIZE);
  }

  /// write to a file.
  // @param s text
  // @return number of bytes written.
  // @function write
  static int l_File_write(lua_State *L) {
    File *this = File_arg(L,1);
    const char *s = luaL_checklstring(L,2,NULL);
    #line 988 "winapi.l.c"
    DWORD bytesWrote;
    WriteFile(this->hWrite, s, lua_objlen(L,2), &bytesWrote, NULL);
    lua_pushinteger(L,bytesWrote);
    return 1;
  }

  static BOOL raw_read (File *this) {
    DWORD bytesRead = 0;
    BOOL res = ReadFile(lcb_handle(this), lcb_buf(this), lcb_bufsz(this), &bytesRead, NULL);
    lcb_buf(this)[bytesRead] = '\0';
    return res && bytesRead;
  }

  /// read from a file.
  // Please note that this is not buffered, and you will have to
  // split into lines, etc yourself.
  // @return text if successful, nil plus error otherwise.
  // @function read
  static int l_File_read(lua_State *L) {
    File *this = File_arg(L,1);
    #line 1007 "winapi.l.c"
    if (raw_read(this)) {
      lua_pushstring(L,lcb_buf(this));
      return 1;
    } else {
      return push_error(L);
    }
  }

  static void file_reader (File *this) { // background reader thread
    int n;
    do {
      n = raw_read(this);
      lcb_call (this,0,lcb_buf(this),! n);
    } while (n);

  }

  /// asynchronous read.
  // @param callback function that will receive each chunk of text
  // as it comes in.
  // @return @{Thread}
  // @function read_async
  static int l_File_read_async(lua_State *L) {
    File *this = File_arg(L,1);
    int callback = 2;
    #line 1030 "winapi.l.c"
    this->callback = make_ref(L,callback);
    return lcb_new_thread((TCB)&file_reader,this);
  }

  static int l_File_close(lua_State *L) {
    File *this = File_arg(L,1);
    #line 1035 "winapi.l.c"
    if (this->hWrite != lcb_handle(this))
      CloseHandle(this->hWrite);
    lcb_free(this);
    return 0;
  }

  static int l_File___gc(lua_State *L) {
    File *this = File_arg(L,1);
    #line 1042 "winapi.l.c"
    free(this->buf);
    return 0;
  }
#line 1045 "winapi.l.c"

static const struct luaL_reg File_methods [] = {
     {"write",l_File_write},
   {"read",l_File_read},
   {"read_async",l_File_read_async},
   {"close",l_File_close},
   {"__gc",l_File___gc},
  {NULL, NULL}  /* sentinel */
};

static void File_register (lua_State *L) {
  luaL_newmetatable(L,File_MT);
  luaL_register(L,NULL,File_methods);
  lua_pushvalue(L,-1);
  lua_setfield(L,-2,"__index");
  lua_pop(L,1);
}


#line 1047 "winapi.l.c"

/// Launching processes.
// @section Launch

/// set an environment variable for any child processes.
// @{setenv.lua} shows how this also affects processes
// launched with @{os.execute}
// Note that this can't affect any system environment variables, see
// [here](http://msdn.microsoft.com/en-us/library/ms682653%28VS.85%29.aspx)
// for how to set these.
// @param name name of variable
// @param value value to set
// @function setenv
static int l_setenv(lua_State *L) {
  const char *name = luaL_checklstring(L,1,NULL);
  const char *value = luaL_checklstring(L,2,NULL);
  #line 1060 "winapi.l.c"
  WCHAR wname[256],wvalue[MAX_WPATH];
  return push_bool(L, SetEnvironmentVariableW(wconv(name),wconv(value)));
}

/// Spawn a process.
// @param program the command-line (program + parameters)
// @param dir the working directory for the process (optional)
// @return @{Process}
// @return @{File}
// @function spawn_process
static int l_spawn_process(lua_State *L) {
  const char *program = luaL_checklstring(L,1,NULL);
  const char *dir = lua_tostring(L,2);
  #line 1071 "winapi.l.c"
  WCHAR wdir [MAX_WPATH];
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), 0, 0};
  SECURITY_DESCRIPTOR sd;
  STARTUPINFOW si = {
           sizeof(STARTUPINFOW), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
       };
  HANDLE hPipeRead,hWriteSubProcess;
  HANDLE hRead2,hPipeWrite;
  BOOL running;
  PROCESS_INFORMATION pi;
  HANDLE hProcess = GetCurrentProcess();
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;
  InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
  SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = &sd;

  // Create pipe for output redirection
  CreatePipe(&hPipeRead, &hPipeWrite, &sa, 0);

  // Create pipe for input redirection. In this code, you do not
  // redirect the output of the child process, but you need a handle
  // to set the hStdInput field in the STARTUP_INFO struct. For safety,
  // you should not set the handles to an invalid handle.

  hRead2 = NULL;
  CreatePipe(&hRead2, &hWriteSubProcess, &sa, 0);

  SetHandleInformation(hPipeRead, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(hWriteSubProcess, HANDLE_FLAG_INHERIT, 0);

  si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
  si.wShowWindow = SW_HIDE;
  si.hStdInput = hRead2;
  si.hStdOutput = hPipeWrite;
  si.hStdError = hPipeWrite;

  running = CreateProcessW(
        NULL,
        (LPWSTR)wstring(program),
        NULL, NULL,
        TRUE, CREATE_NEW_PROCESS_GROUP,
        NULL,
        wconv(dir),
        &si, &pi);

  if (running) {
    CloseHandle(pi.hThread);
    CloseHandle(hRead2);
    CloseHandle(hPipeWrite);
    push_new_Process(L,pi.dwProcessId,pi.hProcess);
    push_new_File(L,hPipeRead,hWriteSubProcess);
    return 2;
  } else {
    return push_error(L);
  }
}

/// execute a system command.
// This is like `os.execute()`, except that it works without ugly
// console flashing in Windows GUI applications. It additionally
// returns all text read from stdout and stderr.
// @param cmd a shell command (may include redirection, etc)
// @param unicode if 'unicode' force built-in commands to output in unicode;
// in this case the result is always UTF-8
// @return return code
// @return program output
// @function execute

// Timer support //////////
typedef struct {
  callback_data_
  int msec;
} TimerData;

static void timer_thread(TimerData *data) { // background timer thread
  while (1) {
    Sleep(data->msec);
    if (lcb_call(data,0,0,0))
      break;
  }
}

/// Asynchronous Timers.
// @section Timers

/// Create an asynchronous timer.
// The callback can return true if it wishes to cancel the timer.
// @{test-sleep.lua} shows how you need to call @{sleep} at the end of
// a console application for these timers to work in the background.
// @param msec interval in millisec
// @param callback a function to be called at each interval.
// @return @{Thread}
// @function make_timer
static int l_make_timer(lua_State *L) {
  int msec = luaL_checkinteger(L,1);
  int callback = 2;
  #line 1167 "winapi.l.c"
  TimerData *data = (TimerData *)malloc(sizeof(TimerData));
  data->msec = msec;
  lcb_callback(data,L,callback);
  return lcb_new_thread((TCB)&timer_thread,data);
}

#define PSIZE 512

typedef struct {
  callback_data_
  const char *pipename;
} PipeServerParms;

static void pipe_server_thread(PipeServerParms *parms) {
  while (1) {
    BOOL connected;
    HANDLE hPipe = CreateNamedPipe(
          parms->pipename,             // pipe named
          PIPE_ACCESS_DUPLEX,       // read/write access
          PIPE_WAIT,                // blocking mode
          255,
          PSIZE,                  // output buffer size
          PSIZE,                  // input buffer size
          0,                        // client time-out
          NULL);                    // default security attribute

    if (hPipe == INVALID_HANDLE_VALUE) {
      push_error(parms->L); // how to signal main thread about this?
    }
    // Wait for the client to connect; if it succeeds,
    // the function returns a nonzero value. If the function
    // returns zero, GetLastError returns ERROR_PIPE_CONNECTED.

    connected = ConnectNamedPipe(hPipe, NULL) ?
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

    if (connected) {
      push_new_File(parms->L,hPipe,hPipe);
      lcb_call(parms,-1,0,0);
    } else {
      CloseHandle(hPipe);
    }
  }
}

/// Dealing with named pipes.
// @section Pipes

/// open a pipe for reading and writing.
// @param pipename the pipename (default is "\\\\.\\pipe\\luawinapi")
// @function open_pipe
static int l_open_pipe(lua_State *L) {
  const char *pipename = luaL_optlstring(L,1,"\\\\.\\pipe\\luawinapi",NULL);
  #line 1219 "winapi.l.c"
  HANDLE hPipe = CreateFile(
      pipename,
      GENERIC_READ |  // read and write access
      GENERIC_WRITE,
      0,              // no sharing
      NULL,           // default security attributes
      OPEN_EXISTING,  // opens existing pipe
      0,              // default attributes
      NULL);          // no template file
  if (hPipe == INVALID_HANDLE_VALUE) {
    return push_error(L);
  } else {
    return push_new_File(L,hPipe,hPipe);
  }
}

/// create a named pipe server.
// This goes into a background loop, and accepts client connections.
// For each new connection, the callback will be called with a File
// object for reading and writing to the client.
// @param callback a function that will be passed a File object
// @param pipename Must be of the form \\.\pipe\name, defaults to
// \\.\pipe\luawinapi.
// @return @{Thread}.
// @function make_pipe_server
static int l_make_pipe_server(lua_State *L) {
  int callback = 1;
  const char *pipename = luaL_optlstring(L,2,"\\\\.\\pipe\\luawinapi",NULL);
  #line 1245 "winapi.l.c"
  PipeServerParms *psp = (PipeServerParms*)malloc(sizeof(PipeServerParms));
  lcb_callback(psp,L,callback);
  psp->pipename = pipename;
  return lcb_new_thread((TCB)&pipe_server_thread,psp);
}

// Directory change notification ///////

typedef struct {
  callback_data_
  DWORD how;
  DWORD subdirs;
} FileChangeParms;

static void file_change_thread(FileChangeParms *fc) { // background file monitor thread
  while (1) {
    int next;
    DWORD bytes;
    // This fills in some gaps:
    // http://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw_19.html
    if (! ReadDirectoryChangesW(lcb_handle(fc),lcb_buf(fc),lcb_bufsz(fc),
        fc->subdirs, fc->how, &bytes,NULL,NULL))  {
      throw_error(fc->L,"read dir changes failed");
    }
    next = 0;
    do {
      int i,sz, outchars;
      short *pfile;
      char outbuff[MAX_PATH];
      PFILE_NOTIFY_INFORMATION pni = (PFILE_NOTIFY_INFORMATION)(lcb_buf(fc)+next);
      outchars = WideCharToMultiByte(
        get_encoding(), 0,
        pni->FileName,
        pni->FileNameLength/2, // it's bytes, not number of characters!
        outbuff,sizeof(outbuff),
        NULL,NULL);
      if (outchars == 0) {
        throw_error(fc->L,"wide char conversion borked");
      }
      outbuff[outchars] = '\0';  // not null-terminated!
      lcb_call(fc,pni->Action,outbuff,0);
      next = pni->NextEntryOffset;
    } while (next != 0);
  }
}

/// Drive information and directories.
// @section Directories

/// the short path name of a directory or file.
// This is always in ASCII, 8.3 format. This function will create the
// file first if it does not exist; the result can be used to open
// files with unicode names (see @{testshort.lua})
// @param path multibyte encoded file path
// @return ASCII 8.3 format file path
// @function short_path
static int l_short_path(lua_State *L) {
  const char *path = luaL_checklstring(L,1,NULL);
  #line 1302 "winapi.l.c"
  WCHAR wpath[MAX_WPATH];
  HANDLE hFile;
  int res;
  wconv(path);
  // if the file doesn't exist, then force its creation
  hFile = CreateFileW(wpath,
    GENERIC_WRITE,
    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
    CREATE_NEW,
    FILE_ATTRIBUTE_NORMAL,
    NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    if (GetLastError() != ERROR_FILE_EXISTS) // that error is fine!
      return push_error(L);
  } else { // if we created it successfully, then close.
    CloseHandle(hFile);
  }
  res = GetShortPathNameW(wpath,wbuff,sizeof(wbuff));
  if (res > 0) {
    return push_wstring(L,wbuff);
  } else {
    return push_error(L);
  }
}

/// get a temporary filename.
// (Don't use os.tmpname)
// @return full path within temporary files directory.
// @function temp_name

/// delete a file or directory.
// @param file may be a wildcard
// @function delete_file_or_dir

/// make a directory.
// Will make necessary subpaths if command extensions are enabled.
// @function make_dir

/// remove a directory.
// @param dir the directory
// @param tree if true, clean out the directory tree
// @function remove_dir

/// iterator over directory contents.
// @usage for f in winapi.files 'dir\\*.txt' do print(f) end
// @param mask a file mask like "*.txt"
// @param subdirs iterate over subdirectories (default no)
// @param attrib iterate over items with given attribute (as in dir /A:)
// @see files.lua
// @function files

/// iterate over subdirectories
// @param file mask like "mydirs\\t*"
// @param subdirs iterate over subdirectories (default no)
// @see files
// @function dirs

/// get all the drives on this computer.
// An example is @{drives.lua}
// @return a table of drive names
// @function get_logical_drives
static int l_get_logical_drives(lua_State *L) {
  int i, lasti = 0, k = 1;
  WCHAR dbuff[MAX_WPATH];
  LPWSTR p = dbuff;
  DWORD size = GetLogicalDriveStringsW(sizeof(dbuff),dbuff);
  lua_newtable(L);
  for (i = 0; i < size; i++) {
    if (dbuff[i] == '\0') {
      push_wstring_l(L,p, i - lasti);
      lua_rawseti(L,-2,k++);
      p = dbuff + i+1;
      lasti = i+1;
    }
  }
  return 1;
}

/// get the type of the given drive.
// @param root root of drive (e.g. 'c:\\')
// @return one of the following: unknown, none, removable, fixed, remote,
// cdrom, ramdisk.
// @function get_drive_type
static int l_get_drive_type(lua_State *L) {
  const char *root = luaL_checklstring(L,1,NULL);
  #line 1386 "winapi.l.c"
  UINT res = GetDriveType(root);
  const char *type;
  switch(res) {
    case DRIVE_UNKNOWN: type = "unknown"; break;
    case DRIVE_NO_ROOT_DIR: type = "none"; break;
    case DRIVE_REMOVABLE: type = "removable"; break;
    case DRIVE_FIXED: type = "fixed"; break;
    case DRIVE_REMOTE: type = "remote"; break;
    case DRIVE_CDROM: type = "cdrom"; break;
    case DRIVE_RAMDISK: type = "ramdisk"; break;
  }
  lua_pushstring(L,type);
  return 1;
}

/// get the free disk space.
// @param root the root of the drive (e.g. 'd:\\')
// @return free space in kB
// @return total space in kB
// @function get_disk_free_space
static int l_get_disk_free_space(lua_State *L) {
  const char *root = luaL_checklstring(L,1,NULL);
  #line 1407 "winapi.l.c"
  ULARGE_INTEGER freebytes, totalbytes;
  if (! GetDiskFreeSpaceEx(root,&freebytes,&totalbytes,NULL)) {
    return push_error(L);
  }
  lua_pushnumber(L,freebytes.QuadPart/1024);
  lua_pushnumber(L,totalbytes.QuadPart/1024);
  return 2;
}

/// get the network resource associated with this drive.
// @param root drive name in the form 'X:'
// @return UNC name
// @function get_disk_network_name
static int l_get_disk_network_name(lua_State *L) {
  const char *root = luaL_checklstring(L,1,NULL);
  #line 1421 "winapi.l.c"
  DWORD size = sizeof(wbuff);
  DWORD res = WNetGetConnectionW(wstring(root),wbuff,&size);
  if (res == NO_ERROR) {
    return push_wstring(L,wbuff);
  } else {
    return push_error(L);
  }
}

//// start watching a directory.
// @param dir the directory
// @param how what events to monitor. Can be a sum of these flags:
//
//  * `FILE_NOTIFY_CHANGE_FILE_NAME`
//  * `FILE_NOTIFY_CHANGE_DIR_NAME`
//  * `FILE_NOTIFY_CHANGE_LAST_WRITE`
//
// @param subdirs whether subdirectories should be monitored
// @param callback a function which will receive the kind of change
// plus the filename that changed. The change will be one of these:
//
// * `FILE_ACTION_ADDED`
// * `FILE_ACTION_REMOVED`
// * `FILE_ACTION_MODIFIED`
// * `FILE_ACTION_RENAMED_OLD_NAME`
// * `FILE_ACTION_RENAMED_NEW_NAME`
//
// @return a thread object.
// @see test-watcher.lua
// @function watch_for_file_changes
static int l_watch_for_file_changes(lua_State *L) {
  const char *dir = luaL_checklstring(L,1,NULL);
  int how = luaL_checkinteger(L,2);
  int subdirs = lua_toboolean(L,3);
  int callback = 4;
  #line 1452 "winapi.l.c"
  FileChangeParms *fc = (FileChangeParms*)malloc(sizeof(FileChangeParms));
  lcb_callback(fc,L,callback);
  fc->how = how;
  fc->subdirs = subdirs;
  lcb_handle(fc) = CreateFileW(wstring(dir),
    FILE_LIST_DIRECTORY,
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    NULL,
    OPEN_ALWAYS,
    FILE_FLAG_BACKUP_SEMANTICS,
    NULL
    );
  if (lcb_handle(fc) == INVALID_HANDLE_VALUE) {
    return push_error(L);
  }
  lcb_allocate_buffer(fc,2048);
  return lcb_new_thread((TCB)&file_change_thread,fc);
}

/// Class representing Windows registry keys.
// @type Regkey
#line 1476 "winapi.l.c"

typedef struct {
  HKEY key;

} Regkey;


#define Regkey_MT "Regkey"

Regkey * Regkey_arg(lua_State *L,int idx) {
  Regkey *this = (Regkey *)luaL_checkudata(L,idx,Regkey_MT);
  luaL_argcheck(L, this != NULL, idx, "Regkey expected");
  return this;
}

static void Regkey_ctor(lua_State *L, Regkey *this, HKEY k);

static int push_new_Regkey(lua_State *L,HKEY k) {
  Regkey *this = (Regkey *)lua_newuserdata(L,sizeof(Regkey));
  luaL_getmetatable(L,Regkey_MT);
  lua_setmetatable(L,-2);
  Regkey_ctor(L,this,k);
  return 1;
}


static void Regkey_ctor(lua_State *L, Regkey *this, HKEY k) {
    #line 1477 "winapi.l.c"
    this->key = k;
  }

  /// set the string value of a name.
  // @param name the name
  // @param val the value
  // @function set_value
  static int l_Regkey_set_value(lua_State *L) {
    Regkey *this = Regkey_arg(L,1);
    const char *name = luaL_checklstring(L,2,NULL);
    const char *val = luaL_checklstring(L,3,NULL);
    #line 1485 "winapi.l.c"
    return push_bool(L, RegSetValueEx(this->key,name,0,REG_SZ,val,lua_objlen(L,2)) == ERROR_SUCCESS);
  }

  /// get the value and type of a name.
  // @param name the name (can be empty for the default value)
  // @return the value (either a string or a number)
  // @return the type
  // @function get_value
  static int l_Regkey_get_value(lua_State *L) {
    Regkey *this = Regkey_arg(L,1);
    const char *name = luaL_optlstring(L,2,"",NULL);
    #line 1494 "winapi.l.c"
    DWORD type,size = sizeof(wbuff);
    if (RegQueryValueExW(this->key,wstring(name),0,&type,(void *)wbuff,&size) != ERROR_SUCCESS) {
      return push_error(L);
    }
    if (type == REG_BINARY) {
      lua_pushlstring(L,(const char *)wbuff,size);
    } else if (type == REG_EXPAND_SZ || type == REG_SZ) {
      push_wstring(L,wbuff); //,size);
    } else {
      lua_pushnumber(L,*(unsigned long *)wbuff);
    }
    lua_pushinteger(L,type);
    return 2;

  }

  /// enumerate the subkeys of a key.
  // @return a table of key names
  // @function get_keys
  static int l_Regkey_get_keys(lua_State *L) {
    Regkey *this = Regkey_arg(L,1);
    #line 1514 "winapi.l.c"
    int i = 0;
    LONG res;
    DWORD size;
    lua_newtable(L);
    while (1) {
      size = sizeof(wbuff);
      res = RegEnumKeyExW(this->key,i,wbuff,&size,NULL,NULL,NULL,NULL);
      if (res != ERROR_SUCCESS) break;
      push_wstring(L,wbuff);
      lua_rawseti(L,-2,i+1);
      ++i;
    }
    if (res != ERROR_NO_MORE_ITEMS) {
      lua_pop(L,1);
      return push_error(L);
    }
    return 1;
  }

  /// close this key.
  // Although this will happen when garbage collection happens, it
  // is good practice to call this explicitly.
  // @function close
  static int l_Regkey_close(lua_State *L) {
    Regkey *this = Regkey_arg(L,1);
    #line 1538 "winapi.l.c"
    RegCloseKey(this->key);
    this->key = NULL;
    return 0;
  }

  /// flush the key.
  // Considered an expensive function; use it only when you have
  // to guarantee modification.
  // @function flush
  static int l_Regkey_flush(lua_State *L) {
    Regkey *this = Regkey_arg(L,1);
    #line 1548 "winapi.l.c"
    return push_bool(L,RegFlushKey(this->key));
  }

  static int l_Regkey___gc(lua_State *L) {
    Regkey *this = Regkey_arg(L,1);
    #line 1552 "winapi.l.c"
    if (this->key != NULL)
      RegCloseKey(this->key);
    return 0;
  }

#line 1557 "winapi.l.c"

static const struct luaL_reg Regkey_methods [] = {
     {"set_value",l_Regkey_set_value},
   {"get_value",l_Regkey_get_value},
   {"get_keys",l_Regkey_get_keys},
   {"close",l_Regkey_close},
   {"flush",l_Regkey_flush},
   {"__gc",l_Regkey___gc},
  {NULL, NULL}  /* sentinel */
};

static void Regkey_register (lua_State *L) {
  luaL_newmetatable(L,Regkey_MT);
  luaL_register(L,NULL,Regkey_methods);
  lua_pushvalue(L,-1);
  lua_setfield(L,-2,"__index");
  lua_pop(L,1);
}


#line 1559 "winapi.l.c"

/// Registry Functions.
// @section Registry

/// Open a registry key.
// @{test-reg.lua} shows reading a registry value and enumerating subkeys.
// @param path the full registry key
// e.g `[[HKEY\_LOCAL\_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion]]`
// @param writeable true if you want to set values
// @return @{Regkey}
// @function open_reg_key
static int l_open_reg_key(lua_State *L) {
  const char *path = luaL_checklstring(L,1,NULL);
  int writeable = lua_toboolean(L,2);
  #line 1570 "winapi.l.c"
  HKEY hKey;
  DWORD access;
  char kbuff[1024];
  hKey = split_registry_key(path,kbuff);
  if (hKey == NULL) {
    return push_error_msg(L,"unrecognized registry key");
  }
  access = writeable ? KEY_ALL_ACCESS : (KEY_READ | KEY_ENUMERATE_SUB_KEYS);
  if (RegOpenKeyExW(hKey,wstring(kbuff),0,access,&hKey) == ERROR_SUCCESS) {
    return push_new_Regkey(L,hKey);
  } else {
    return push_error(L);
  }
}

/// Create a registry key.
// @param path the full registry key
// @return @{Regkey}
// @function create_reg_key
static int l_create_reg_key(lua_State *L) {
  const char *path = luaL_checklstring(L,1,NULL);
  #line 1590 "winapi.l.c"
  char kbuff[1024];
  HKEY hKey = split_registry_key(path,kbuff);
  if (hKey == NULL) {
    return push_error_msg(L,"unrecognized registry key");
  }
  if (RegCreateKeyExW(hKey,wstring(kbuff),0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL)) {
    return push_new_Regkey(L,hKey);
  } else {
    return push_error(L);
  }
}

#line 1673 "winapi.l.c"
static const char *lua_code_block = ""\
  "function winapi.execute(cmd,unicode)\n"\
  "  local comspec = os.getenv('COMSPEC')\n"\
  "  if unicode ~= 'unicode' then\n"\
  "    cmd = comspec ..' /c '..cmd\n"\
  "    local P,f = winapi.spawn_process(cmd)\n"\
  "    if not P then return nil,f end\n"\
  "    local txt = f:read()\n"\
  "    local out = {}\n"\
  "    while txt do\n"\
  "      table.insert(out,txt)\n"\
  "      txt = f:read()\n"\
  "    end\n"\
  "    return P:wait():get_exit_code(),table.concat(out,'')\n"\
  "  else\n"\
  "    local tmpfile,res,f,out = winapi.temp_name()\n"\
  "    cmd = comspec..' /u /c '..cmd..' > \"'..tmpfile..'\"'\n"\
  "    local P,err = winapi.spawn_process(cmd)\n"\
  "    if not P then return nil,err end\n"\
  "    res = P:wait():get_exit_code()\n"\
  "    f = io.open(tmpfile)\n"\
  "    out = f:read '*a'\n"\
  "    f:close()\n"\
  "    os.remove(tmpfile)\n"\
  "    out, err = winapi.encode(winapi.CP_UTF16,winapi.CP_UTF8,out)\n"\
  "    if err then return nil,err end\n"\
  "    return res,out\n"\
  "  end\n"\
  "end\n"\
  "function winapi.make_name_matcher(text)\n"\
  "  return function(w) return tostring(w):match(text) end\n"\
  "end\n"\
  "function winapi.make_class_matcher(classname)\n"\
  "  return function(w) return w:get_class_name():match(classname) end\n"\
  "end\n"\
  "function winapi.find_window_ex(match)\n"\
  "  local res\n"\
  "  winapi.enum_windows(function(w)\n"\
  "    if match(w) then res = w end\n"\
  "  end)\n"\
  "  return res\n"\
  "end\n"\
  "function winapi.find_all_windows(match)\n"\
  "  local res = {}\n"\
  "  winapi.enum_windows(function(w)\n"\
  "    if match(w) then res[#res+1] = w end\n"\
  "  end)\n"\
  "  return res\n"\
  "end\n"\
  "function winapi.find_window_match(text)\n"\
  "  return winapi.find_window_ex(winapi.make_name_matcher(text))\n"\
  "end\n"\
  "function winapi.temp_name () return os.getenv('TEMP')..os.tmpname() end\n"\
  "local function exec_cmd (cmd,arg)\n"\
  "    local res,err = winapi.execute(cmd..' \"'..arg..'\"')\n"\
  "    if res == 0 then return true\n"\
  "    else return nil,err\n"\
  "    end\n"\
  "end\n"\
  "function winapi.make_dir(dir) return exec_cmd('mkdir',dir) end\n"\
  "function winapi.remove_dir(dir,tree) return exec_cmd('rmdir '.. (tree and '/S'),dir) end\n"\
  "function winapi.delete_file_or_dir(file) return exec_cmd('del',file) end\n"\
  "function winapi.files(mask,subdirs,attrib)\n"\
  "    local flags = '/B '\n"\
  "    if subdirs then flags = flags..' /S' end\n"\
  "    if attrib then flags = flags..' /A:'..attrib end\n"\
  "    local ret, text = winapi.execute('dir '..flags..' \"'..mask..'\"','unicode')\n"\
  "    if ret ~= 0 then return nil,text end\n"\
  "    return text:gmatch('[^\\r\\n]+')\n"\
  "end\n"\
  "function winapi.dirs(mask,subdirs) return winapi.files(mask,subdirs,'D') end\n"\
;
static void load_lua_code (lua_State *L) {
  luaL_dostring(L,lua_code_block);
}



#line 1676 "winapi.l.c"


/*** Constants.
The following constants are available:

 * CP_ACP, (valid values for encoding)
 * CP_UTF8,
 * CP_UTF16,
 * SW_HIDE, (Window operations for Window.show)
 * SW_MAXIMIZE,
 * SW_MINIMIZE,
 * SW_SHOWNORMAL,
 * VK_BACK,
 * VK_TAB,
 * VK_RETURN,
 * VK_SPACE,
 * VK_PRIOR,
 * VK_NEXT,
 * VK_END,
 * VK_HOME,
 * VK_LEFT,
 * VK_UP,
 * VK_RIGHT,
 * VK_DOWN,
 * VK_INSERT,
 * VK_DELETE,
 * VK_ESCAPE,
 * VK_F1,
 * VK_F2,
 * VK_F3,
 * VK_F4,
 * VK_F5,
 * VK_F6,
 * VK_F7,
 * VK_F8,
 * VK_F9,
 * VK_F10,
 * VK_F11,
 * VK_F12,
 * FILE\_NOTIFY\_CHANGE\_FILE\_NAME  (these are input flags for watch\_for\_file\_changes)
 * FILE\_NOTIFY\_CHANGE\_DIR\_NAME
 * FILE\_NOTIFY\_CHANGE\_LAST\_WRITE
 * FILE\_ACTION\_ADDED     (these describe the change: first argument of callback)
 * FILE\_ACTION\_REMOVED
 * FILE\_ACTION\_MODIFIED
 * FILE\_ACTION\_RENAMED\_OLD\_NAME
 * FILE\_ACTION\_RENAMED\_NEW\_NAME

 @section constants
 */#line 1723 "winapi.l.c"


 #line 1725 "winapi.l.c"

 /// useful Windows API constants
 // @table constants

#define CP_UTF16 -1

#line 1773 "winapi.l.c"
static void set_winapi_constants(lua_State *L) {
 lua_pushinteger(L,CP_ACP); lua_setfield(L,-2,"CP_ACP");
 lua_pushinteger(L,CP_UTF8); lua_setfield(L,-2,"CP_UTF8");
 lua_pushinteger(L,CP_UTF16); lua_setfield(L,-2,"CP_UTF16");
 lua_pushinteger(L,SW_HIDE); lua_setfield(L,-2,"SW_HIDE");
 lua_pushinteger(L,SW_MAXIMIZE); lua_setfield(L,-2,"SW_MAXIMIZE");
 lua_pushinteger(L,SW_MINIMIZE); lua_setfield(L,-2,"SW_MINIMIZE");
 lua_pushinteger(L,SW_SHOWNORMAL); lua_setfield(L,-2,"SW_SHOWNORMAL");
 lua_pushinteger(L,VK_BACK); lua_setfield(L,-2,"VK_BACK");
 lua_pushinteger(L,VK_TAB); lua_setfield(L,-2,"VK_TAB");
 lua_pushinteger(L,VK_RETURN); lua_setfield(L,-2,"VK_RETURN");
 lua_pushinteger(L,VK_SPACE); lua_setfield(L,-2,"VK_SPACE");
 lua_pushinteger(L,VK_PRIOR); lua_setfield(L,-2,"VK_PRIOR");
 lua_pushinteger(L,VK_NEXT); lua_setfield(L,-2,"VK_NEXT");
 lua_pushinteger(L,VK_END); lua_setfield(L,-2,"VK_END");
 lua_pushinteger(L,VK_HOME); lua_setfield(L,-2,"VK_HOME");
 lua_pushinteger(L,VK_LEFT); lua_setfield(L,-2,"VK_LEFT");
 lua_pushinteger(L,VK_UP); lua_setfield(L,-2,"VK_UP");
 lua_pushinteger(L,VK_RIGHT); lua_setfield(L,-2,"VK_RIGHT");
 lua_pushinteger(L,VK_DOWN); lua_setfield(L,-2,"VK_DOWN");
 lua_pushinteger(L,VK_INSERT); lua_setfield(L,-2,"VK_INSERT");
 lua_pushinteger(L,VK_DELETE); lua_setfield(L,-2,"VK_DELETE");
 lua_pushinteger(L,VK_ESCAPE); lua_setfield(L,-2,"VK_ESCAPE");
 lua_pushinteger(L,VK_F1); lua_setfield(L,-2,"VK_F1");
 lua_pushinteger(L,VK_F2); lua_setfield(L,-2,"VK_F2");
 lua_pushinteger(L,VK_F3); lua_setfield(L,-2,"VK_F3");
 lua_pushinteger(L,VK_F4); lua_setfield(L,-2,"VK_F4");
 lua_pushinteger(L,VK_F5); lua_setfield(L,-2,"VK_F5");
 lua_pushinteger(L,VK_F6); lua_setfield(L,-2,"VK_F6");
 lua_pushinteger(L,VK_F7); lua_setfield(L,-2,"VK_F7");
 lua_pushinteger(L,VK_F8); lua_setfield(L,-2,"VK_F8");
 lua_pushinteger(L,VK_F9); lua_setfield(L,-2,"VK_F9");
 lua_pushinteger(L,VK_F10); lua_setfield(L,-2,"VK_F10");
 lua_pushinteger(L,VK_F11); lua_setfield(L,-2,"VK_F11");
 lua_pushinteger(L,VK_F12); lua_setfield(L,-2,"VK_F12");
 lua_pushinteger(L,FILE_NOTIFY_CHANGE_FILE_NAME); lua_setfield(L,-2,"FILE_NOTIFY_CHANGE_FILE_NAME");
 lua_pushinteger(L,FILE_NOTIFY_CHANGE_DIR_NAME); lua_setfield(L,-2,"FILE_NOTIFY_CHANGE_DIR_NAME");
 lua_pushinteger(L,FILE_NOTIFY_CHANGE_LAST_WRITE); lua_setfield(L,-2,"FILE_NOTIFY_CHANGE_LAST_WRITE");
 lua_pushinteger(L,FILE_ACTION_ADDED); lua_setfield(L,-2,"FILE_ACTION_ADDED");
 lua_pushinteger(L,FILE_ACTION_REMOVED); lua_setfield(L,-2,"FILE_ACTION_REMOVED");
 lua_pushinteger(L,FILE_ACTION_MODIFIED); lua_setfield(L,-2,"FILE_ACTION_MODIFIED");
 lua_pushinteger(L,FILE_ACTION_RENAMED_OLD_NAME); lua_setfield(L,-2,"FILE_ACTION_RENAMED_OLD_NAME");
 lua_pushinteger(L,FILE_ACTION_RENAMED_NEW_NAME); lua_setfield(L,-2,"FILE_ACTION_RENAMED_NEW_NAME");
}

#line 1775 "winapi.l.c"
static const luaL_reg winapi_funs[] = {
       {"set_encoding",l_set_encoding},
   {"get_encoding",l_get_encoding},
   {"encode",l_encode},
   {"utf8_expand",l_utf8_expand},
   {"find_window",l_find_window},
   {"get_foreground_window",l_get_foreground_window},
   {"get_desktop_window",l_get_desktop_window},
   {"enum_windows",l_enum_windows},
   {"use_gui",l_use_gui},
   {"send_to_window",l_send_to_window},
   {"tile_windows",l_tile_windows},
   {"sleep",l_sleep},
   {"show_message",l_show_message},
   {"beep",l_beep},
   {"copy_file",l_copy_file},
   {"output_debug_string",l_output_debug_string},
   {"move_file",l_move_file},
   {"shell_exec",l_shell_exec},
   {"set_clipboard",l_set_clipboard},
   {"get_clipboard",l_get_clipboard},
   {"process_from_id",l_process_from_id},
   {"get_current_pid",l_get_current_pid},
   {"get_current_process",l_get_current_process},
   {"get_processes",l_get_processes},
   {"wait_for_processes",l_wait_for_processes},
   {"setenv",l_setenv},
   {"spawn_process",l_spawn_process},
   {"make_timer",l_make_timer},
   {"open_pipe",l_open_pipe},
   {"make_pipe_server",l_make_pipe_server},
   {"short_path",l_short_path},
   {"get_logical_drives",l_get_logical_drives},
   {"get_drive_type",l_get_drive_type},
   {"get_disk_free_space",l_get_disk_free_space},
   {"get_disk_network_name",l_get_disk_network_name},
   {"watch_for_file_changes",l_watch_for_file_changes},
   {"open_reg_key",l_open_reg_key},
   {"create_reg_key",l_create_reg_key},
    {NULL,NULL}
};

EXPORT int luaopen_winapi (lua_State *L) {
    luaL_register (L,"winapi",winapi_funs);
    Window_register(L);
Process_register(L);
Thread_register(L);
File_register(L);
Regkey_register(L);
load_lua_code(L);
set_winapi_constants(L);
    return 1;
}


