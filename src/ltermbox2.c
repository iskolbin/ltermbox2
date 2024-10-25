#define ltermbox2lib_c
#define LUA_LIB

#define TB_IMPL
#define TB_OPT_ATTR_W 64
#define TB_OPT_EGC 1
#include "termbox2.h"

#include <lua.h>
#include <lauxlib.h>

#if (LUA_VERSION_NUM<=501)
#define lua_len(L,i) (lua_pushnumber((L),lua_objlen((L),(i))))
#define luaL_newlib(L,l) (lua_newtable(L),luaL_register(L,NULL,l))
#endif

#ifndef luaL_checkint
#define luaL_checkint luaL_checkinteger
#endif

static const char ref_event_key = 'k';
static const char ref_event_resize = 'r';
static const char ref_event_mouse = 'm';

static int tb2_init(lua_State *L) {
  if (lua_gettop(L) >= 2) {
    int rfd = luaL_checkint(L, -2);
    int wfd = luaL_checkint(L, -1);
    lua_pushinteger(L, tb_init_rwfd(rfd, wfd));
  } else if (lua_gettop(L) == 1 && lua_isnumber(L, -1)) {
    int ttyfd = luaL_checkint(L, -1);
    lua_pushinteger(L, tb_init_fd(ttyfd));
  } else if (lua_gettop(L) == 1 && lua_isstring(L, -1)) {
    const char *path = luaL_checkstring(L, -1);
    lua_pushinteger(L, tb_init_file(path));
  } else {
    lua_pushinteger(L, tb_init());
  }
  return 1;
}

static int tb2_shutdown(lua_State *L) {
  lua_pushinteger(L, tb_shutdown());
  return 1;
}

static int tb2_width(lua_State *L) {
  lua_pushinteger(L, tb_width());
  return 1;
}

static int tb2_height(lua_State *L) {
  lua_pushinteger(L, tb_height());
  return 1;
}

static int tb2_set_clear_attrs(lua_State *L) {
  uintattr_t fg = luaL_checkint(L, 1);
  uintattr_t bg = luaL_checkint(L, 2);
  lua_pushinteger(L, tb_set_clear_attrs(fg, bg));
  return 1;
}

static int tb2_clear(lua_State *L) {
  lua_pushinteger(L, tb_clear());
  return 1;
}

static int tb2_present(lua_State *L) {
  lua_pushinteger(L, tb_present());
  return 1;
}

static int tb2_set_cursor(lua_State *L) {
  int x = luaL_checkint(L, 1);
  int y = luaL_checkint(L, 2);
  lua_pushinteger(L, tb_set_cursor(x, y));
  return 1;
}

static int tb2_hide_cursor(lua_State *L) {
  lua_pushinteger(L, tb_hide_cursor());
  return 1;
}

static int tb2_set_cell(lua_State *L) {
  int x = luaL_checkint(L, 1);
  int y = luaL_checkint(L, 2);
  const char *chstr = luaL_checkstring(L, 3);
  uint32_t ch;
  tb_utf8_char_to_unicode(&ch, chstr);
  uintattr_t fg = lua_isnumber(L, 4) ? lua_tonumber(L, 4) : TB_DEFAULT;
  uintattr_t bg = lua_isnumber(L, 5) ? lua_tonumber(L, 5) : TB_DEFAULT;
  lua_pushinteger(L, tb_set_cell(x, y, ch, fg, bg));
  return 1;
}

static int tb2_print(lua_State *L) {
  int x = luaL_checkint(L, 1);
  int y = luaL_checkint(L, 2);
  const char *str = luaL_checkstring(L, 3);
  uintattr_t fg = lua_isnumber(L, 4) ? lua_tonumber(L, 4) : TB_DEFAULT;
  uintattr_t bg = lua_isnumber(L, 5) ? lua_tonumber(L, 5) : TB_DEFAULT;
  size_t out_w = 0;
  lua_pushinteger(L, tb_print(x, y, fg, bg, str));
  return 1;
}

static int tb2_extend_cell(lua_State *L) {
  const char *chstr = luaL_checkstring(L, 1);
  int x = luaL_checkint(L, 2);
  int y = luaL_checkint(L, 3);
  uint32_t ch;
  tb_utf8_char_to_unicode(&ch, chstr);
  lua_pushinteger(L, tb_extend_cell(x, y, ch));
  return 1;
}

static void *get_event_ref(int event_type) {
  switch (event_type) {
    case TB_EVENT_KEY: return (void *)&ref_event_key;
    case TB_EVENT_RESIZE: return (void *)&ref_event_resize;
    case TB_EVENT_MOUSE: return (void *)&ref_event_mouse;
  }
  return NULL;
}

static int tb2_wait_event(lua_State *L, int timeout) {
  struct tb_event event;
  int err = timeout > 0 ? tb_peek_event(&event, timeout) : tb_poll_event(&event);
  lua_pushlightuserdata(L, get_event_ref(event.type));
  lua_gettable(L, LUA_REGISTRYINDEX);
  if (!lua_isfunction(L, -1)) {
    return 0;
  }
  switch (event.type) {
    case TB_EVENT_KEY:
      if (event.ch) {
        char out[8] = {0};
        tb_utf8_unicode_to_char(out, event.ch);
        lua_pushstring(L, out);
        lua_pushinteger(L, event.ch);
      } else {
        lua_pushstring(L, "");
        lua_pushinteger(L, event.key);
      }
      lua_pushinteger(L, event.mod);
      lua_call(L, 3, 0);
      break;
    case TB_EVENT_RESIZE:
      lua_pushinteger(L, event.w);
      lua_pushinteger(L, event.h);
      lua_call(L, 2, 0);
      break;
    case TB_EVENT_MOUSE:
      lua_pushinteger(L, event.x);
      lua_pushinteger(L, event.y);
      lua_pushinteger(L, event.key);
      lua_call(L, 3, 0);
      break;
    default:
      lua_pop(L, 1);
  }
  return 0;
}

static int tb2_peek_event(lua_State *L) {
  int timeout = luaL_checkint(L, -1);
  return tb2_wait_event(L, timeout);
}

static int tb2_poll_event(lua_State *L) {
  return tb2_wait_event(L, -1);
}

static int tb2_set_callback(lua_State *L) {
  int event_type = luaL_checkint(L, 1);
  void *event_ref = get_event_ref(event_type);
  if (event_ref == NULL) {
    return luaL_error(L, "bad argument #1 to 'setcallback' (expected constant TB_EVENT_*)");
  }
  if (!lua_isfunction(L, 2)) {
    return luaL_error(L, "bad argument #2 to 'setcallback' (function expected)");
  }
  lua_pushlightuserdata(L, event_ref);  
  lua_pushvalue(L, 2);
  lua_settable(L, LUA_REGISTRYINDEX);
  return 0;
}

static int tb2_set_input_mode(lua_State *L) {
  int mode = luaL_checkint(L, 1);
  lua_pushinteger(L, tb_set_input_mode(mode));
  return 1;
}

static int tb2_set_output_mode(lua_State *L) {
  int mode = luaL_checkint(L, 1);
  lua_pushinteger(L, tb_set_output_mode(mode));
  return 1;
}

static void tb2_init_consts(lua_State *L) {
  lua_pushinteger(L, TB_PATH_MAX); lua_setfield(L, -2, "PATH_MAX");
  lua_pushstring(L, TB_VERSION_STR); lua_setfield(L, -2, "VERSION_STR");
  lua_pushinteger(L, TB_OPT_ATTR_W); lua_setfield(L, -2, "OPT_ATTR_W");
#ifdef TB_OPT_EGC
  lua_pushinteger(L, TB_OPT_EGC); lua_setfield(L, -2, "OPT_EGC");
#endif
  lua_pushinteger(L, TB_KEY_CTRL_TILDE); lua_setfield(L, -2, "KEY_CTRL_TILDE");
  lua_pushinteger(L, TB_KEY_CTRL_2); lua_setfield(L, -2, "KEY_CTRL_2");
  lua_pushinteger(L, TB_KEY_CTRL_A); lua_setfield(L, -2, "KEY_CTRL_A");
  lua_pushinteger(L, TB_KEY_CTRL_B); lua_setfield(L, -2, "KEY_CTRL_B");
  lua_pushinteger(L, TB_KEY_CTRL_C); lua_setfield(L, -2, "KEY_CTRL_C");
  lua_pushinteger(L, TB_KEY_CTRL_D); lua_setfield(L, -2, "KEY_CTRL_D");
  lua_pushinteger(L, TB_KEY_CTRL_E); lua_setfield(L, -2, "KEY_CTRL_E");
  lua_pushinteger(L, TB_KEY_CTRL_F); lua_setfield(L, -2, "KEY_CTRL_F");
  lua_pushinteger(L, TB_KEY_CTRL_G); lua_setfield(L, -2, "KEY_CTRL_G");
  lua_pushinteger(L, TB_KEY_BACKSPACE); lua_setfield(L, -2, "KEY_BACKSPACE");
  lua_pushinteger(L, TB_KEY_CTRL_H); lua_setfield(L, -2, "KEY_CTRL_H");
  lua_pushinteger(L, TB_KEY_TAB); lua_setfield(L, -2, "KEY_TAB");
  lua_pushinteger(L, TB_KEY_CTRL_I); lua_setfield(L, -2, "KEY_CTRL_I");
  lua_pushinteger(L, TB_KEY_CTRL_J); lua_setfield(L, -2, "KEY_CTRL_J");
  lua_pushinteger(L, TB_KEY_CTRL_K); lua_setfield(L, -2, "KEY_CTRL_K");
  lua_pushinteger(L, TB_KEY_CTRL_L); lua_setfield(L, -2, "KEY_CTRL_L");
  lua_pushinteger(L, TB_KEY_ENTER); lua_setfield(L, -2, "KEY_ENTER");
  lua_pushinteger(L, TB_KEY_CTRL_M); lua_setfield(L, -2, "KEY_CTRL_M");
  lua_pushinteger(L, TB_KEY_CTRL_N); lua_setfield(L, -2, "KEY_CTRL_N");
  lua_pushinteger(L, TB_KEY_CTRL_O); lua_setfield(L, -2, "KEY_CTRL_O");
  lua_pushinteger(L, TB_KEY_CTRL_P); lua_setfield(L, -2, "KEY_CTRL_P");
  lua_pushinteger(L, TB_KEY_CTRL_Q); lua_setfield(L, -2, "KEY_CTRL_Q");
  lua_pushinteger(L, TB_KEY_CTRL_R); lua_setfield(L, -2, "KEY_CTRL_R");
  lua_pushinteger(L, TB_KEY_CTRL_S); lua_setfield(L, -2, "KEY_CTRL_S");
  lua_pushinteger(L, TB_KEY_CTRL_T); lua_setfield(L, -2, "KEY_CTRL_T");
  lua_pushinteger(L, TB_KEY_CTRL_U); lua_setfield(L, -2, "KEY_CTRL_U");
  lua_pushinteger(L, TB_KEY_CTRL_V); lua_setfield(L, -2, "KEY_CTRL_V");
  lua_pushinteger(L, TB_KEY_CTRL_W); lua_setfield(L, -2, "KEY_CTRL_W");
  lua_pushinteger(L, TB_KEY_CTRL_X); lua_setfield(L, -2, "KEY_CTRL_X");
  lua_pushinteger(L, TB_KEY_CTRL_Y); lua_setfield(L, -2, "KEY_CTRL_Y");
  lua_pushinteger(L, TB_KEY_CTRL_Z); lua_setfield(L, -2, "KEY_CTRL_Z");
  lua_pushinteger(L, TB_KEY_ESC); lua_setfield(L, -2, "KEY_ESC");
  lua_pushinteger(L, TB_KEY_CTRL_LSQ_BRACKET); lua_setfield(L, -2, "KEY_CTRL_LSQ_BRACKET");
  lua_pushinteger(L, TB_KEY_CTRL_3); lua_setfield(L, -2, "KEY_CTRL_3");
  lua_pushinteger(L, TB_KEY_CTRL_4); lua_setfield(L, -2, "KEY_CTRL_4");
  lua_pushinteger(L, TB_KEY_CTRL_BACKSLASH); lua_setfield(L, -2, "KEY_CTRL_BACKSLASH");
  lua_pushinteger(L, TB_KEY_CTRL_5); lua_setfield(L, -2, "KEY_CTRL_5");
  lua_pushinteger(L, TB_KEY_CTRL_RSQ_BRACKET); lua_setfield(L, -2, "KEY_CTRL_RSQ_BRACKET");
  lua_pushinteger(L, TB_KEY_CTRL_6); lua_setfield(L, -2, "KEY_CTRL_6");
  lua_pushinteger(L, TB_KEY_CTRL_7); lua_setfield(L, -2, "KEY_CTRL_7");
  lua_pushinteger(L, TB_KEY_CTRL_SLASH); lua_setfield(L, -2, "KEY_CTRL_SLASH");
  lua_pushinteger(L, TB_KEY_CTRL_UNDERSCORE); lua_setfield(L, -2, "KEY_CTRL_UNDERSCORE");
  lua_pushinteger(L, TB_KEY_SPACE); lua_setfield(L, -2, "KEY_SPACE");
  lua_pushinteger(L, TB_KEY_BACKSPACE2); lua_setfield(L, -2, "KEY_BACKSPACE2");
  lua_pushinteger(L, TB_KEY_CTRL_8); lua_setfield(L, -2, "KEY_CTRL_8");
  lua_pushinteger(L, TB_KEY_F1); lua_setfield(L, -2, "KEY_F1");
  lua_pushinteger(L, TB_KEY_F2); lua_setfield(L, -2, "KEY_F2");
  lua_pushinteger(L, TB_KEY_F3); lua_setfield(L, -2, "KEY_F3");
  lua_pushinteger(L, TB_KEY_F4); lua_setfield(L, -2, "KEY_F4");
  lua_pushinteger(L, TB_KEY_F5); lua_setfield(L, -2, "KEY_F5");
  lua_pushinteger(L, TB_KEY_F6); lua_setfield(L, -2, "KEY_F6");
  lua_pushinteger(L, TB_KEY_F7); lua_setfield(L, -2, "KEY_F7");
  lua_pushinteger(L, TB_KEY_F8); lua_setfield(L, -2, "KEY_F8");
  lua_pushinteger(L, TB_KEY_F9); lua_setfield(L, -2, "KEY_F9");
  lua_pushinteger(L, TB_KEY_F10); lua_setfield(L, -2, "KEY_F10");
  lua_pushinteger(L, TB_KEY_F11); lua_setfield(L, -2, "KEY_F11");
  lua_pushinteger(L, TB_KEY_F12); lua_setfield(L, -2, "KEY_F12");
  lua_pushinteger(L, TB_KEY_INSERT); lua_setfield(L, -2, "KEY_INSERT");
  lua_pushinteger(L, TB_KEY_DELETE); lua_setfield(L, -2, "KEY_DELETE");
  lua_pushinteger(L, TB_KEY_HOME); lua_setfield(L, -2, "KEY_HOME");
  lua_pushinteger(L, TB_KEY_END); lua_setfield(L, -2, "KEY_END");
  lua_pushinteger(L, TB_KEY_PGUP); lua_setfield(L, -2, "KEY_PGUP");
  lua_pushinteger(L, TB_KEY_PGDN); lua_setfield(L, -2, "KEY_PGDN");
  lua_pushinteger(L, TB_KEY_ARROW_UP); lua_setfield(L, -2, "KEY_ARROW_UP");
  lua_pushinteger(L, TB_KEY_ARROW_DOWN); lua_setfield(L, -2, "KEY_ARROW_DOWN");
  lua_pushinteger(L, TB_KEY_ARROW_LEFT); lua_setfield(L, -2, "KEY_ARROW_LEFT");
  lua_pushinteger(L, TB_KEY_ARROW_RIGHT); lua_setfield(L, -2, "KEY_ARROW_RIGHT");
  lua_pushinteger(L, TB_KEY_BACK_TAB); lua_setfield(L, -2, "KEY_BACK_TAB");
  lua_pushinteger(L, TB_KEY_MOUSE_LEFT); lua_setfield(L, -2, "KEY_MOUSE_LEFT");
  lua_pushinteger(L, TB_KEY_MOUSE_RIGHT); lua_setfield(L, -2, "KEY_MOUSE_RIGHT");
  lua_pushinteger(L, TB_KEY_MOUSE_MIDDLE); lua_setfield(L, -2, "KEY_MOUSE_MIDDLE");
  lua_pushinteger(L, TB_KEY_MOUSE_RELEASE); lua_setfield(L, -2, "KEY_MOUSE_RELEASE");
  lua_pushinteger(L, TB_KEY_MOUSE_WHEEL_UP); lua_setfield(L, -2, "KEY_MOUSE_WHEEL_UP");
  lua_pushinteger(L, TB_KEY_MOUSE_WHEEL_DOWN); lua_setfield(L, -2, "KEY_MOUSE_WHEEL_DOWN");
  lua_pushinteger(L, TB_CAP_F1); lua_setfield(L, -2, "CAP_F1");
  lua_pushinteger(L, TB_CAP_F2); lua_setfield(L, -2, "CAP_F2");
  lua_pushinteger(L, TB_CAP_F3); lua_setfield(L, -2, "CAP_F3");
  lua_pushinteger(L, TB_CAP_F4); lua_setfield(L, -2, "CAP_F4");
  lua_pushinteger(L, TB_CAP_F5); lua_setfield(L, -2, "CAP_F5");
  lua_pushinteger(L, TB_CAP_F6); lua_setfield(L, -2, "CAP_F6");
  lua_pushinteger(L, TB_CAP_F7); lua_setfield(L, -2, "CAP_F7");
  lua_pushinteger(L, TB_CAP_F8); lua_setfield(L, -2, "CAP_F8");
  lua_pushinteger(L, TB_CAP_F9); lua_setfield(L, -2, "CAP_F9");
  lua_pushinteger(L, TB_CAP_F10); lua_setfield(L, -2, "CAP_F10");
  lua_pushinteger(L, TB_CAP_F11); lua_setfield(L, -2, "CAP_F11");
  lua_pushinteger(L, TB_CAP_F12); lua_setfield(L, -2, "CAP_F12");
  lua_pushinteger(L, TB_CAP_INSERT); lua_setfield(L, -2, "CAP_INSERT");
  lua_pushinteger(L, TB_CAP_DELETE); lua_setfield(L, -2, "CAP_DELETE");
  lua_pushinteger(L, TB_CAP_HOME); lua_setfield(L, -2, "CAP_HOME");
  lua_pushinteger(L, TB_CAP_END); lua_setfield(L, -2, "CAP_END");
  lua_pushinteger(L, TB_CAP_PGUP); lua_setfield(L, -2, "CAP_PGUP");
  lua_pushinteger(L, TB_CAP_PGDN); lua_setfield(L, -2, "CAP_PGDN");
  lua_pushinteger(L, TB_CAP_ARROW_UP); lua_setfield(L, -2, "CAP_ARROW_UP");
  lua_pushinteger(L, TB_CAP_ARROW_DOWN); lua_setfield(L, -2, "CAP_ARROW_DOWN");
  lua_pushinteger(L, TB_CAP_ARROW_LEFT); lua_setfield(L, -2, "CAP_ARROW_LEFT");
  lua_pushinteger(L, TB_CAP_ARROW_RIGHT); lua_setfield(L, -2, "CAP_ARROW_RIGHT");
  lua_pushinteger(L, TB_CAP_BACK_TAB); lua_setfield(L, -2, "CAP_BACK_TAB");
  lua_pushinteger(L, TB_CAP__COUNT_KEYS); lua_setfield(L, -2, "CAP__COUNT_KEYS");
  lua_pushinteger(L, TB_CAP_ENTER_CA); lua_setfield(L, -2, "CAP_ENTER_CA");
  lua_pushinteger(L, TB_CAP_EXIT_CA); lua_setfield(L, -2, "CAP_EXIT_CA");
  lua_pushinteger(L, TB_CAP_SHOW_CURSOR); lua_setfield(L, -2, "CAP_SHOW_CURSOR");
  lua_pushinteger(L, TB_CAP_HIDE_CURSOR); lua_setfield(L, -2, "CAP_HIDE_CURSOR");
  lua_pushinteger(L, TB_CAP_CLEAR_SCREEN); lua_setfield(L, -2, "CAP_CLEAR_SCREEN");
  lua_pushinteger(L, TB_CAP_SGR0); lua_setfield(L, -2, "CAP_SGR0");
  lua_pushinteger(L, TB_CAP_UNDERLINE); lua_setfield(L, -2, "CAP_UNDERLINE");
  lua_pushinteger(L, TB_CAP_BOLD); lua_setfield(L, -2, "CAP_BOLD");
  lua_pushinteger(L, TB_CAP_BLINK); lua_setfield(L, -2, "CAP_BLINK");
  lua_pushinteger(L, TB_CAP_ITALIC); lua_setfield(L, -2, "CAP_ITALIC");
  lua_pushinteger(L, TB_CAP_REVERSE); lua_setfield(L, -2, "CAP_REVERSE");
  lua_pushinteger(L, TB_CAP_ENTER_KEYPAD); lua_setfield(L, -2, "CAP_ENTER_KEYPAD");
  lua_pushinteger(L, TB_CAP_EXIT_KEYPAD); lua_setfield(L, -2, "CAP_EXIT_KEYPAD");
  lua_pushinteger(L, TB_CAP_DIM); lua_setfield(L, -2, "CAP_DIM");
  lua_pushinteger(L, TB_CAP_INVISIBLE); lua_setfield(L, -2, "CAP_INVISIBLE");
  lua_pushinteger(L, TB_CAP__COUNT); lua_setfield(L, -2, "CAP__COUNT");
  lua_pushstring(L, TB_HARDCAP_ENTER_MOUSE); lua_setfield(L, -2, "HARDCAP_ENTER_MOUSE");
  lua_pushstring(L, TB_HARDCAP_EXIT_MOUSE); lua_setfield(L, -2, "HARDCAP_EXIT_MOUSE");
  lua_pushstring(L, TB_HARDCAP_STRIKEOUT); lua_setfield(L, -2, "HARDCAP_STRIKEOUT");
  lua_pushstring(L, TB_HARDCAP_UNDERLINE_2); lua_setfield(L, -2, "HARDCAP_UNDERLINE_2");
  lua_pushstring(L, TB_HARDCAP_OVERLINE); lua_setfield(L, -2, "HARDCAP_OVERLINE");
  lua_pushinteger(L, TB_DEFAULT); lua_setfield(L, -2, "DEFAULT");
  lua_pushinteger(L, TB_BLACK); lua_setfield(L, -2, "BLACK");
  lua_pushinteger(L, TB_RED); lua_setfield(L, -2, "RED");
  lua_pushinteger(L, TB_GREEN); lua_setfield(L, -2, "GREEN");
  lua_pushinteger(L, TB_YELLOW); lua_setfield(L, -2, "YELLOW");
  lua_pushinteger(L, TB_BLUE); lua_setfield(L, -2, "BLUE");
  lua_pushinteger(L, TB_MAGENTA); lua_setfield(L, -2, "MAGENTA");
  lua_pushinteger(L, TB_CYAN); lua_setfield(L, -2, "CYAN");
  lua_pushinteger(L, TB_WHITE); lua_setfield(L, -2, "WHITE");
  lua_pushinteger(L, TB_BOLD); lua_setfield(L, -2, "BOLD");
  lua_pushinteger(L, TB_UNDERLINE); lua_setfield(L, -2, "UNDERLINE");
  lua_pushinteger(L, TB_REVERSE); lua_setfield(L, -2, "REVERSE");
  lua_pushinteger(L, TB_ITALIC); lua_setfield(L, -2, "ITALIC");
  lua_pushinteger(L, TB_BLINK); lua_setfield(L, -2, "BLINK");
  lua_pushinteger(L, TB_HI_BLACK); lua_setfield(L, -2, "HI_BLACK");
  lua_pushinteger(L, TB_BRIGHT); lua_setfield(L, -2, "BRIGHT");
  lua_pushinteger(L, TB_DIM); lua_setfield(L, -2, "DIM");
  lua_pushinteger(L, TB_BOLD); lua_setfield(L, -2, "BOLD");
  lua_pushinteger(L, TB_UNDERLINE); lua_setfield(L, -2, "UNDERLINE");
  lua_pushinteger(L, TB_REVERSE); lua_setfield(L, -2, "REVERSE");
  lua_pushinteger(L, TB_ITALIC); lua_setfield(L, -2, "ITALIC");
  lua_pushinteger(L, TB_BLINK); lua_setfield(L, -2, "BLINK");
  lua_pushinteger(L, TB_HI_BLACK); lua_setfield(L, -2, "HI_BLACK");
  lua_pushinteger(L, TB_BRIGHT); lua_setfield(L, -2, "BRIGHT");
  lua_pushinteger(L, TB_DIM); lua_setfield(L, -2, "DIM");
#ifdef TB_STRIKEOUT
  lua_pushinteger(L, TB_STRIKEOUT); lua_setfield(L, -2, "STRIKEOUT");
#endif
#ifdef TB_UNDERLINE_2
  lua_pushinteger(L, TB_UNDERLINE_2); lua_setfield(L, -2, "UNDERLINE_2");
#endif
#ifdef TB_OVERLINE
  lua_pushinteger(L, TB_OVERLINE); lua_setfield(L, -2, "OVERLINE");
#endif
#ifdef TB_INVISIBLE
  lua_pushinteger(L, TB_INVISIBLE); lua_setfield(L, -2, "INVISIBLE");
#endif
  lua_pushinteger(L, TB_EVENT_KEY); lua_setfield(L, -2, "EVENT_KEY");
  lua_pushinteger(L, TB_EVENT_RESIZE); lua_setfield(L, -2, "EVENT_RESIZE");
  lua_pushinteger(L, TB_EVENT_MOUSE); lua_setfield(L, -2, "EVENT_MOUSE");
  lua_pushinteger(L, TB_MOD_ALT); lua_setfield(L, -2, "MOD_ALT");
  lua_pushinteger(L, TB_MOD_CTRL); lua_setfield(L, -2, "MOD_CTRL");
  lua_pushinteger(L, TB_MOD_SHIFT); lua_setfield(L, -2, "MOD_SHIFT");
  lua_pushinteger(L, TB_MOD_MOTION); lua_setfield(L, -2, "MOD_MOTION");
  lua_pushinteger(L, TB_INPUT_CURRENT); lua_setfield(L, -2, "INPUT_CURRENT");
  lua_pushinteger(L, TB_INPUT_ESC); lua_setfield(L, -2, "INPUT_ESC");
  lua_pushinteger(L, TB_INPUT_ALT); lua_setfield(L, -2, "INPUT_ALT");
  lua_pushinteger(L, TB_INPUT_MOUSE); lua_setfield(L, -2, "INPUT_MOUSE");
  lua_pushinteger(L, TB_OUTPUT_CURRENT); lua_setfield(L, -2, "OUTPUT_CURRENT");
  lua_pushinteger(L, TB_OUTPUT_NORMAL); lua_setfield(L, -2, "OUTPUT_NORMAL");
  lua_pushinteger(L, TB_OUTPUT_256); lua_setfield(L, -2, "OUTPUT_256");
  lua_pushinteger(L, TB_OUTPUT_216); lua_setfield(L, -2, "OUTPUT_216");
  lua_pushinteger(L, TB_OUTPUT_GRAYSCALE); lua_setfield(L, -2, "OUTPUT_GRAYSCALE");
#ifdef TB_OUTPUT_TRUECOLOR
  lua_pushinteger(L, TB_OUTPUT_TRUECOLOR); lua_setfield(L, -2, "OUTPUT_TRUECOLOR");
#endif
  lua_pushinteger(L, TB_OK); lua_setfield(L, -2, "OK");
  lua_pushinteger(L, TB_ERR); lua_setfield(L, -2, "ERR");
  lua_pushinteger(L, TB_ERR_NEED_MORE); lua_setfield(L, -2, "ERR_NEED_MORE");
  lua_pushinteger(L, TB_ERR_INIT_ALREADY); lua_setfield(L, -2, "ERR_INIT_ALREADY");
  lua_pushinteger(L, TB_ERR_INIT_OPEN); lua_setfield(L, -2, "ERR_INIT_OPEN");
  lua_pushinteger(L, TB_ERR_MEM); lua_setfield(L, -2, "ERR_MEM");
  lua_pushinteger(L, TB_ERR_NO_EVENT); lua_setfield(L, -2, "ERR_NO_EVENT");
  lua_pushinteger(L, TB_ERR_NO_TERM); lua_setfield(L, -2, "ERR_NO_TERM");
  lua_pushinteger(L, TB_ERR_NOT_INIT); lua_setfield(L, -2, "ERR_NOT_INIT");
  lua_pushinteger(L, TB_ERR_OUT_OF_BOUNDS); lua_setfield(L, -2, "ERR_OUT_OF_BOUNDS");
  lua_pushinteger(L, TB_ERR_READ); lua_setfield(L, -2, "ERR_READ");
  lua_pushinteger(L, TB_ERR_RESIZE_IOCTL); lua_setfield(L, -2, "ERR_RESIZE_IOCTL");
  lua_pushinteger(L, TB_ERR_RESIZE_PIPE); lua_setfield(L, -2, "ERR_RESIZE_PIPE");
  lua_pushinteger(L, TB_ERR_RESIZE_SIGACTION); lua_setfield(L, -2, "ERR_RESIZE_SIGACTION");
  lua_pushinteger(L, TB_ERR_POLL); lua_setfield(L, -2, "ERR_POLL");
  lua_pushinteger(L, TB_ERR_TCGETATTR); lua_setfield(L, -2, "ERR_TCGETATTR");
  lua_pushinteger(L, TB_ERR_TCSETATTR); lua_setfield(L, -2, "ERR_TCSETATTR");
  lua_pushinteger(L, TB_ERR_UNSUPPORTED_TERM); lua_setfield(L, -2, "ERR_UNSUPPORTED_TERM");
  lua_pushinteger(L, TB_ERR_RESIZE_WRITE); lua_setfield(L, -2, "ERR_RESIZE_WRITE");
  lua_pushinteger(L, TB_ERR_RESIZE_POLL); lua_setfield(L, -2, "ERR_RESIZE_POLL");
  lua_pushinteger(L, TB_ERR_RESIZE_READ); lua_setfield(L, -2, "ERR_RESIZE_READ");
  lua_pushinteger(L, TB_ERR_RESIZE_SSCANF); lua_setfield(L, -2, "ERR_RESIZE_SSCANF");
  lua_pushinteger(L, TB_ERR_CAP_COLLISION); lua_setfield(L, -2, "ERR_CAP_COLLISION");
  lua_pushinteger(L, TB_ERR_SELECT); lua_setfield(L, -2, "ERR_SELECT");
  lua_pushinteger(L, TB_ERR_RESIZE_SELECT); lua_setfield(L, -2, "ERR_RESIZE_SELECT");
  lua_pushinteger(L, TB_OPT_PRINTF_BUF); lua_setfield(L, -2, "OPT_PRINTF_BUF");
  lua_pushinteger(L, TB_OPT_READ_BUF); lua_setfield(L, -2, "OPT_READ_BUF");
  lua_pushinteger(L, TB_RESIZE_FALLBACK_MS); lua_setfield(L, -2, "RESIZE_FALLBACK_MS");
}

static const luaL_Reg tb2lib[] = {
  {"init", tb2_init},
  {"shutdown", tb2_shutdown},
  {"width", tb2_width},
  {"height", tb2_height},
  {"clear", tb2_clear},
  {"setclearattr", tb2_set_clear_attrs},
  {"setinputmode", tb2_set_input_mode},
  {"setoutputmode", tb2_set_output_mode},
  {"present", tb2_present},
  {"setcursor", tb2_set_cursor},
  {"hidecursor", tb2_hide_cursor},
  {"peekevent", tb2_peek_event},
  {"pollevent", tb2_poll_event},
  {"setcell", tb2_set_cell},
  {"print", tb2_print},
  {"extendcell", tb2_extend_cell},
  {"setcallback", tb2_set_callback},
  {NULL, NULL}
};

LUAMOD_API int luaopen_termbox2(lua_State *L) {
  luaL_newlib(L, tb2lib);
  tb2_init_consts(L);
  return 1;
}
