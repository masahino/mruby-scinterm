#include <stdio.h>
#include <stdlib.h>
#include "mruby.h"
#include "mruby/class.h"
#include "mruby/data.h"
#include "mruby/string.h"
#include "mruby/array.h"
#include "mruby/variable.h"

#include "Scintilla.h"
#include "ScintillaTerm.h"

#define DONE mrb_gc_arena_restore(mrb, 0)


static mrb_state *scmrb;

struct mrb_scintilla_data {
  Scintilla *view;
  mrb_value view_obj;
  bool has_callback;
  struct mrb_scintilla_data *next;
};

struct mrb_scintilla_doc_data {
  sptr_t pdoc;
};

static struct mrb_scintilla_data *scintilla_list = NULL;

void scnotification(Scintilla *view, int msg, void *lParam, void *wParam) {
  struct mrb_scintilla_data *scdata = scintilla_list;
  struct SCNotification *n = (struct SCNotification *)lParam;
  mrb_value callback;

  while (scdata != NULL) {
    if (scdata->view == view) {
      break;
    }
    scdata = scdata->next;
  }
  if (scdata == NULL) {
    fprintf(stderr, "scdata = NULL\n");
    return;
  }
  if (scdata->has_callback == TRUE) {
    callback = mrb_iv_get(scmrb, scdata->view_obj, mrb_intern_cstr(scmrb, "notification"));
    mrb_yield(scmrb, callback, mrb_fixnum_value(n->nmhdr.code));
  }
  /*
    struct SCNotification *scn = (struct SCNotification *)lParam;
    printw("SCNotification received: %i", scn->nmhdr.code);
  */
}

static void scinterm_free(mrb_state *mrb, void *ptr) {
/* fprintf(stderr, "scinterm_free %p\n", ptr); */
  if (ptr != NULL) {
    scintilla_delete((Scintilla *)ptr);
  }
}

const static struct mrb_data_type mrb_scinterm_type = { "ScinTerm", scinterm_free };

const static struct mrb_data_type mrb_document_type = { "Document", mrb_free };

static mrb_value
mrb_scinterm_initialize(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = scintilla_new(scnotification);
  mrb_value callback;
  //     struct mrb_scintilla_data *scdata = mrb_malloc(mrb, sizeof(struct mrb_scintilla_data));
  struct mrb_scintilla_data *scdata = (struct mrb_scintilla_data *)malloc(sizeof(struct mrb_scintilla_data));
  struct mrb_scintilla_data *tmp;
  mrb_int argc;

  argc = mrb_get_args(mrb, "|&", &callback);
  DATA_TYPE(self) = &mrb_scinterm_type;
  DATA_PTR(self) = sci;
  scdata->view = sci;
  scdata->view_obj = self;
  if (!mrb_nil_p(callback)) {
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "notification"), callback);
    scdata->has_callback = TRUE;
  } else {
    scdata->has_callback = FALSE;
  }
  scdata->next = NULL;

  if (scintilla_list == NULL) {
    scintilla_list = scdata;
  } else {
    tmp = scintilla_list;
    while (tmp->next != NULL) {
      tmp = tmp->next;
    }
    tmp->next = scdata;
  }
  return self;
}

/* scintilla_delete (sci) */
static mrb_value
mrb_scinterm_delete(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci;
  sci = (Scintilla *)DATA_PTR(self);
  scintilla_delete(sci);
  DATA_PTR(self) = NULL;
  return mrb_nil_value();
}

/* scintilla_get_clipboard (sci, buffer) */
static mrb_value
mrb_scinterm_get_clipboard(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci;
  char *buffer = NULL;
  mrb_int size;
  mrb_value ret_ary = mrb_ary_new(mrb);

  sci = (Scintilla *)DATA_PTR(self);
  size = scintilla_get_clipboard(sci, buffer);
  mrb_ary_push(mrb, ret_ary, mrb_fixnum_value(size));
  mrb_ary_push(mrb, ret_ary, mrb_str_new_cstr(mrb, buffer));

  return ret_ary;
}

static mrb_value
mrb_scinterm_get_window(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci;
  WINDOW *win;
  sci = (Scintilla *)DATA_PTR(self);
  win = scintilla_get_window(sci);
  return mrb_cptr_value(mrb, win);
}
/*
  static mrb_value
  mrb_scinterm_get_window(mrb_state *mrb, mrb_value self)
  {
  Scintilla *sci;
  struct windata *winp;
  WINDOW *win;
  mrb_value mrb_curses, mrb_stdscr, mrb_win;

  sci = DATA_PTR(self);
  winp = (struct windata *)mrb_malloc(mrb, sizeof(struct windata));
  win = scintilla_get_window(sci);
  winp->window = win;
  mrb_curses = mrb_obj_value(mrb_class_get(mrb, "Curses"));
  mrb_stdscr = mrb_funcall(mrb, mrb_curses, "stdscr", 0);
  mrb_win = mrb_obj_value(mrb_data_object_alloc(mrb, mrb_class_get_under(mrb, mrb_class_get(mrb, "Curses"), "Window"), winp, DATA_TYPE(mrb_stdscr)));
  mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "window"), mrb_win);
  fprintf(stderr, "window p = %p\n", win);
  return mrb_win;
  }
*/

static mrb_value
mrb_scinterm_noutrefresh(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);

  scintilla_noutrefresh(sci);
  return mrb_nil_value();
}

static mrb_value
mrb_scinterm_refresh(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);

  scintilla_refresh(sci);
  return mrb_nil_value();
}

/*scintilla_send_key (sci, key, shift, ctrl, alt) */
static mrb_value
mrb_scinterm_send_key(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  mrb_int key;
  mrb_bool shift, ctrl, alt;

  mrb_get_args(mrb, "ibbb", &key, &shift, &ctrl, &alt);
  scintilla_send_key(sci, key, shift, ctrl, alt);
  return mrb_nil_value();
}

static mrb_value
mrb_scinterm_send_message(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  sptr_t ret;
  mrb_int i_message, argc;
  uptr_t w_param = 0;
  sptr_t l_param = 0;
  mrb_value w_param_obj, l_param_obj;

  argc = mrb_get_args(mrb, "i|oo", &i_message, &w_param_obj, &l_param_obj);
  if (i_message < SCI_START) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid message");
    return mrb_nil_value();
  }
  if (argc > 1) {
    switch(mrb_type(w_param_obj)) {
    case MRB_TT_FIXNUM:
      w_param = (uptr_t)mrb_fixnum(w_param_obj);
      break;
    case MRB_TT_STRING:
      w_param = (uptr_t)mrb_string_value_ptr(mrb, w_param_obj);
      break;
    case MRB_TT_TRUE:
      w_param = TRUE;
      break;
    case MRB_TT_FALSE:
      w_param = FALSE;
      break;
    case MRB_TT_UNDEF:
      w_param = 0;
      break;
    default:
      mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid parameter");
      return mrb_nil_value();
    }
  }
  if (argc > 2) {
    switch(mrb_type(l_param_obj)) {
    case MRB_TT_FIXNUM:
      l_param = (sptr_t)mrb_fixnum(l_param_obj);
      break;
    case MRB_TT_STRING:
      l_param = (sptr_t)mrb_string_value_ptr(mrb, l_param_obj);
      break;
    case MRB_TT_TRUE:
      l_param = TRUE;
      break;
    case MRB_TT_FALSE:
      l_param = FALSE;
      break;
    case MRB_TT_UNDEF:
      l_param = 0;
      break;
    default:
      mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid parameter");
      return mrb_nil_value();
    }
  }

  ret = scintilla_send_message(sci, i_message, w_param, l_param);
  /*
    if (i_message == SCI_SETPROPERTY) {
    fprintf(stderr, "w_param = %s, l_param = %s\n", w_param, l_param);
    }
  */
  return mrb_fixnum_value(ret);
}

static mrb_value
mrb_scinterm_send_mouse(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  mrb_int event, button, y, x;
  mrb_float time;
  mrb_bool shift, ctrl, alt, ret;

  mrb_get_args(mrb, "ifiiibbb", &event, &time, &button, &y, &x, &shift, &ctrl, &alt);
  ret = scintilla_send_mouse(sci, event, (unsigned int)time, button, y, x, shift, ctrl, alt);

  return (ret == TRUE)? mrb_true_value() : mrb_false_value();

}

static mrb_value
mrb_scinterm_get_property(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  char *key = NULL, *value = NULL;
  mrb_int len;

  mrb_get_args(mrb, "z", &key);
  len = scintilla_send_message(sci, SCI_GETPROPERTY, (uptr_t)key, (sptr_t)NULL);
  value = (char *)malloc(sizeof(char)*len);
  len = scintilla_send_message(sci, SCI_GETPROPERTY, (uptr_t)key, (sptr_t)value);
  return mrb_str_new(mrb, value, len);
}

static mrb_value
mrb_scinterm_get_text(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  char *text = NULL;
  mrb_int nlen;

  mrb_get_args(mrb, "i", &nlen);
  text = (char *)mrb_malloc(mrb, sizeof(char)*nlen);
  scintilla_send_message(sci, SCI_GETTEXT, (uptr_t)nlen, (sptr_t)text);
  return mrb_str_new_cstr(mrb, text);
}

static mrb_value
mrb_scinterm_get_line(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  char *text = NULL;
  mrb_int line, len;

  mrb_get_args(mrb, "i", &line);
  len = scintilla_send_message(sci, SCI_LINELENGTH, (uptr_t)line, (sptr_t)0);
  text = (char *)mrb_malloc(mrb, sizeof(char)*len);
  scintilla_send_message(sci, SCI_GETLINE, (uptr_t)line, (sptr_t)text);
  return mrb_str_new(mrb, text, len);
}

static mrb_value
mrb_scinterm_get_curline(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  char *text = NULL;
  mrb_int len, pos;
  mrb_value ret_a = mrb_ary_new(mrb);

  len = scintilla_send_message(sci, SCI_GETCURLINE, (uptr_t)0, (sptr_t)0) + 1;
  text = (char *)mrb_malloc(mrb, sizeof(char)*len);
  pos = scintilla_send_message(sci, SCI_GETCURLINE, (uptr_t)len, (sptr_t)text);
  mrb_ary_push(mrb, ret_a, mrb_str_new_cstr(mrb, text));
  mrb_ary_push(mrb, ret_a, mrb_fixnum_value(pos));
  return ret_a;
}

static mrb_value
mrb_scinterm_set_lexer_language(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  char *lang = NULL;
  mrb_get_args(mrb, "z", &lang);
  scintilla_send_message(sci, SCI_SETLEXERLANGUAGE, 0, (sptr_t)lang);
  return mrb_nil_value();
}

static mrb_value
mrb_scinterm_get_lexer_language(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  mrb_int len;
  char *text = NULL;

  len = scintilla_send_message(sci, SCI_GETLEXERLANGUAGE, (uptr_t)0, (sptr_t)0) + 1;
  text = (char *)mrb_malloc(mrb, sizeof(char)*len);
  scintilla_send_message(sci, SCI_GETLEXERLANGUAGE, (uptr_t)len, (sptr_t)text);
  return mrb_str_new_cstr(mrb, text);
}


static mrb_value
mrb_scinterm_resize_window(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  mrb_int lines, cols;

  mrb_get_args(mrb, "ii", &lines, &cols);
  wresize(scintilla_get_window(sci), lines, cols);
  wrefresh(scintilla_get_window(sci));

  return mrb_nil_value();
}

static mrb_value
mrb_scinterm_move_window(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  mrb_int x, y;

  mrb_get_args(mrb, "ii", &y, &x);
  mvwin(scintilla_get_window(sci), y, x);
  wrefresh(scintilla_get_window(sci));
	   
  return mrb_nil_value();
}

static mrb_value
mrb_scinterm_setpos(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  mrb_int x, y;

  mrb_get_args(mrb, "ii", &y, &x);
  wmove(scintilla_get_window(sci), y, x);
  wrefresh(scintilla_get_window(sci));

  return mrb_nil_value();
}

static mrb_value
mrb_scinterm_get_docpointer(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  sptr_t pdoc;
  struct mrb_scintilla_doc_data *doc = (struct mrb_scintilla_doc_data *)mrb_malloc(mrb, sizeof(struct mrb_scintilla_doc_data));

  pdoc = scintilla_send_message(sci, SCI_GETDOCPOINTER, 0, 0);
  doc->pdoc = pdoc;
  return mrb_obj_value(mrb_data_object_alloc(mrb, mrb_class_get_under(mrb, mrb_module_get(mrb, "Scintilla"), "Document"), doc, &mrb_document_type));
}

static mrb_value
mrb_scinterm_set_docpointer(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  struct mrb_scintilla_doc_data *doc;
  mrb_value doc_obj;

  mrb_get_args(mrb, "o", &doc_obj);
  if (mrb_nil_p(doc_obj)) {
    scintilla_send_message(sci, SCI_SETDOCPOINTER, 0, (sptr_t)0);
  } else {
    doc = (struct mrb_scintilla_doc_data *)DATA_PTR(doc_obj);
    scintilla_send_message(sci, SCI_SETDOCPOINTER, 0, doc->pdoc);
  }
  return mrb_nil_value();
}

static mrb_value
mrb_scinterm_create_document(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  sptr_t pdoc;
  struct mrb_scintilla_doc_data *doc = (struct mrb_scintilla_doc_data *)mrb_malloc(mrb, sizeof(struct mrb_scintilla_doc_data));

  pdoc = scintilla_send_message(sci, SCI_CREATEDOCUMENT, 0, 0);
  doc->pdoc = pdoc;
  return mrb_obj_value(mrb_data_object_alloc(mrb, mrb_class_get_under(mrb, mrb_module_get(mrb, "Scintilla"), "Document"), doc, &mrb_document_type));

}

static mrb_value
mrb_scinterm_add_refdocument(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  struct mrb_scintilla_doc_data *doc;
  mrb_value doc_obj;

  mrb_get_args(mrb, "o", &doc_obj);
  doc = (struct mrb_scintilla_doc_data *)DATA_PTR(doc_obj);
  scintilla_send_message(sci, SCI_ADDREFDOCUMENT, 0, doc->pdoc);
  return mrb_nil_value();
}

static mrb_value
mrb_scinterm_release_document(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  struct mrb_scintilla_doc_data *doc;
  mrb_value doc_obj;

  mrb_get_args(mrb, "o", &doc_obj);
  doc = (struct mrb_scintilla_doc_data *)DATA_PTR(doc_obj);
  scintilla_send_message(sci, SCI_RELEASEDOCUMENT, 0, doc->pdoc);
  return mrb_nil_value();
}

static mrb_value
mrb_scinterm_autoc_get_current_text(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  char *text = NULL;
  mrb_int len;

  len = scintilla_send_message(sci, SCI_AUTOCGETCURRENTTEXT, (uptr_t)0, (sptr_t)0) + 1;
  if (len == 1) {
    return mrb_nil_value();
  }
  text = (char *)mrb_malloc(mrb, sizeof(char)*len);
  len = scintilla_send_message(sci, SCI_AUTOCGETCURRENTTEXT, (uptr_t)len, (sptr_t)text);
  return mrb_str_new_cstr(mrb, text);
}

static mrb_value
mrb_scinterm_margin_get_text(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  char *text = NULL;
  mrb_int line, len;

  mrb_get_args(mrb, "i", &line);
  len = scintilla_send_message(sci, SCI_MARGINGETTEXT, (uptr_t)line, (sptr_t)0) + 1;
  text = (char *)mrb_malloc(mrb, sizeof(char)*len);
  len = scintilla_send_message(sci, SCI_MARGINGETTEXT, (uptr_t)line, (sptr_t)text);
  return mrb_str_new_cstr(mrb, text);
}

static mrb_value
mrb_scinterm_get_textrange(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);

  mrb_int cp_min, cp_max, len;
  struct Sci_TextRange *tr = (struct Sci_TextRange *)mrb_malloc(mrb, sizeof(struct Sci_TextRange));

  mrb_get_args(mrb, "ii", &cp_min, &cp_max);
  tr->chrg.cpMin = cp_min;
  tr->chrg.cpMax = cp_max;
  tr->lpstrText = (char *)mrb_malloc(mrb, sizeof(char)*(cp_max-cp_min+2));

  len = scintilla_send_message(sci, SCI_GETTEXTRANGE, 0, (sptr_t)tr);
  return mrb_str_new_cstr(mrb, tr->lpstrText);
}

static mrb_value
mrb_scinterm_get_wordchars(mrb_state *mrb, mrb_value self)
{
  Scintilla *sci = (Scintilla *)DATA_PTR(self);
  char *text = NULL;
  mrb_int len;
  len = scintilla_send_message(sci, SCI_GETWORDCHARS, 0, (sptr_t)0) + 1;
  text = (char *)mrb_malloc(mrb, sizeof(char)*len);
  len = scintilla_send_message(sci, SCI_GETWORDCHARS, 0, (sptr_t)text);
  return mrb_str_new_cstr(mrb, text);
}

static mrb_value
mrb_scinterm_color_pair(mrb_state *mrb, mrb_value self)
{
  mrb_int f, b;
  
  mrb_get_args(mrb, "ii", &f, &b);
  return mrb_fixnum_value(SCI_COLOR_PAIR(f, b));
}

void
mrb_mruby_scinterm_gem_init(mrb_state* mrb)
{
  struct RClass *sci, *scim, *doc;

  scim = mrb_module_get(mrb, "Scintilla");

#if !_WIN32
  mrb_define_const(mrb, scim, "PLATFORM", mrb_symbol_value(mrb_intern_cstr(mrb, "CURSES")));
#else
  mrb_define_const(mrb, scim, "PLATFORM", mrb_symbol_value(mrb_intern_cstr(mrb, "CURSES_WIN32")));
#endif

  sci = mrb_define_class_under(mrb, scim, "ScinTerm", mrb_class_get_under(mrb, scim, "ScintillaBase"));
  MRB_SET_INSTANCE_TT(sci, MRB_TT_DATA);

  doc = mrb_define_class_under(mrb, scim, "Document", mrb->object_class);
  MRB_SET_INSTANCE_TT(doc, MRB_TT_DATA);

  mrb_define_method(mrb, sci, "initialize", mrb_scinterm_initialize, MRB_ARGS_OPT(1));
  mrb_define_method(mrb, sci, "delete", mrb_scinterm_delete, MRB_ARGS_NONE());
  mrb_define_method(mrb, sci, "get_clipboard", mrb_scinterm_get_clipboard, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sci, "get_window", mrb_scinterm_get_window, MRB_ARGS_NONE());
  mrb_define_method(mrb, sci, "noutrefresh", mrb_scinterm_noutrefresh, MRB_ARGS_NONE());
  mrb_define_method(mrb, sci, "refresh", mrb_scinterm_refresh, MRB_ARGS_NONE());
  mrb_define_method(mrb, sci, "send_key", mrb_scinterm_send_key, MRB_ARGS_REQ(5));

  mrb_define_method(mrb, sci, "send_message", mrb_scinterm_send_message, MRB_ARGS_ARG(1, 2));

  mrb_define_method(mrb, sci, "send_mouse", mrb_scinterm_send_mouse, MRB_ARGS_REQ(8));

  mrb_define_method(mrb, sci, "sci_get_property", mrb_scinterm_get_property, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sci, "sci_get_text", mrb_scinterm_get_text, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sci, "sci_get_line", mrb_scinterm_get_line, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sci, "sci_get_curline", mrb_scinterm_get_curline, MRB_ARGS_NONE());

  mrb_define_method(mrb, sci, "sci_set_lexer_language", mrb_scinterm_set_lexer_language, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sci, "sci_get_lexer_language", mrb_scinterm_get_lexer_language, MRB_ARGS_NONE());

  mrb_define_method(mrb, sci, "resize_window", mrb_scinterm_resize_window, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, sci, "move_window", mrb_scinterm_move_window, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, sci, "setpos", mrb_scinterm_setpos, MRB_ARGS_REQ(2));

  mrb_define_method(mrb, sci, "sci_get_docpointer", mrb_scinterm_get_docpointer, MRB_ARGS_NONE());
  mrb_define_method(mrb, sci, "sci_set_docpointer", mrb_scinterm_set_docpointer, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sci, "sci_create_document", mrb_scinterm_create_document, MRB_ARGS_NONE());
  mrb_define_method(mrb, sci, "sci_add_refdocument", mrb_scinterm_add_refdocument, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sci, "sci_release_document", mrb_scinterm_release_document, MRB_ARGS_REQ(1));
  
  mrb_define_method(mrb, sci, "sci_autoc_get_current_text", mrb_scinterm_autoc_get_current_text, MRB_ARGS_NONE());
  mrb_define_method(mrb, sci, "sci_margin_get_text", mrb_scinterm_margin_get_text, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sci, "sci_get_textrange", mrb_scinterm_get_textrange, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, sci, "sci_get_wordchars", mrb_scinterm_get_wordchars, MRB_ARGS_NONE());

  mrb_define_class_method(mrb, sci, "color_pair", mrb_scinterm_color_pair, MRB_ARGS_REQ(2));
  
  mrb_define_const(mrb, scim, "COLOR_BLACK", mrb_fixnum_value(0x000000));
  mrb_define_const(mrb, scim, "COLOR_RED",  mrb_fixnum_value(0x000080));
  mrb_define_const(mrb, scim, "COLOR_GREEN", mrb_fixnum_value(0x008000));
  mrb_define_const(mrb, scim, "COLOR_YELLOW",  mrb_fixnum_value(0x008080));
  mrb_define_const(mrb, scim, "COLOR_BLUE", mrb_fixnum_value(0x800000));
  mrb_define_const(mrb, scim, "COLOR_MAGENTA", mrb_fixnum_value(0x800080));
  mrb_define_const(mrb, scim, "COLOR_CYAN", mrb_fixnum_value(0x808000));
  mrb_define_const(mrb, scim, "COLOR_WHITE", mrb_fixnum_value(0xC0C0C0));
  mrb_define_const(mrb, scim, "COLOR_LIGHTBLACK", mrb_fixnum_value(0x404040));
  mrb_define_const(mrb, scim, "COLOR_LIGHTRED", mrb_fixnum_value(0x0000FF));
  mrb_define_const(mrb, scim, "COLOR_LIGHTGREEN", mrb_fixnum_value(0x00FF00));
  mrb_define_const(mrb, scim, "COLOR_LIGHTYELLOW", mrb_fixnum_value(0x00FFFF));
  mrb_define_const(mrb, scim, "COLOR_LIGHTBLUE", mrb_fixnum_value(0xFF0000));
  mrb_define_const(mrb, scim, "COLOR_LIGHTMAGENTA", mrb_fixnum_value(0xFF00FF));
  mrb_define_const(mrb, scim, "COLOR_LIGHTCYAN", mrb_fixnum_value(0xFFFF00));
  mrb_define_const(mrb, scim, "COLOR_LIGHTWHITE", mrb_fixnum_value(0xFFFFFF));

  scmrb = mrb;

  DONE;
}

void
mrb_mruby_scinterm_gem_final(mrb_state* mrb)
{
}
