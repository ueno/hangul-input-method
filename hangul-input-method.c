/*
 * Copyright © 2014 Daiki Ueno
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the licence or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Daiki Ueno <dueno@src.gnome.org>
 */

#include "config.h"

#include <gio/gio.h>
#include <string.h>

#include <hangul.h>
#include <xkbcommon/xkbcommon.h>

#define G_TYPE_HANGUL_INPUT_METHOD_ENGINE (g_hangul_input_method_engine_get_type())
#define G_HANGUL_INPUT_METHOD_ENGINE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HANGUL_INPUT_METHOD_ENGINE, GHangulInputMethodEngine))
#define G_HANGUL_INPUT_METHOD_ENGINE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_HANGUL_INPUT_METHOD_ENGINE, GHangulInputMethodEngineClass))
#define G_IS_HANGUL_INPUT_METHOD_ENGINE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_HANGUL_INPUT_METHOD_ENGINE))
#define G_IS_HANGUL_INPUT_METHOD_ENGINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_HANGUL_INPUT_METHOD_ENGINE))
#define G_HANGUL_INPUT_METHOD_ENGINE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HANGUL_INPUT_METHOD_ENGINE, GHangulInputMethodEngineClass))

typedef struct _GHangulInputMethodEngine GHangulInputMethodEngine;
typedef struct _GHangulInputMethodEngineClass GHangulInputMethodEngineClass;

struct _GHangulInputMethodEngine
{
  GInputMethodEngine parent_instance;

  struct xkb_context *xkb_context;
  struct xkb_keymap *xkb_keymap;
  struct xkb_state *xkb_state;

  HangulInputContext *context;
  GArray *preedit;
};

struct _GHangulInputMethodEngineClass
{
  GInputMethodEngineClass parent_class;
};

static gboolean
g_hangul_input_method_real_key_event (GInputMethodEngine *engine,
                                      guint               keycode,
                                      gboolean            pressed);

G_DEFINE_TYPE (GHangulInputMethodEngine, g_hangul_input_method_engine,
	       G_TYPE_INPUT_METHOD_ENGINE)

static void
g_hangul_input_method_engine_real_dispose (GObject *object)
{
  GHangulInputMethodEngine *hangul = G_HANGUL_INPUT_METHOD_ENGINE (object);

  g_clear_pointer (&hangul->xkb_context, xkb_context_unref);
  g_clear_pointer (&hangul->xkb_keymap, xkb_keymap_unref);
  g_clear_pointer (&hangul->xkb_state, xkb_state_unref);
  g_clear_pointer (&hangul->context, hangul_ic_delete);

  G_OBJECT_CLASS (g_hangul_input_method_engine_parent_class)->dispose (object);
}

static void
g_hangul_input_method_engine_real_finalize (GObject *object)
{
  GHangulInputMethodEngine *hangul = G_HANGUL_INPUT_METHOD_ENGINE (object);

  g_array_free (hangul->preedit, TRUE);

  G_OBJECT_CLASS (g_hangul_input_method_engine_parent_class)->finalize (object);
}

static void
g_hangul_input_method_engine_class_init (GHangulInputMethodEngineClass *klass)
{
  GInputMethodEngineClass *engine_class = G_INPUT_METHOD_ENGINE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  engine_class->key_event = g_hangul_input_method_real_key_event;
  object_class->dispose = g_hangul_input_method_engine_real_dispose;
  object_class->finalize = g_hangul_input_method_engine_real_finalize;
}

static void
g_hangul_input_method_engine_init (GHangulInputMethodEngine *hangul)
{
  const struct xkb_rule_names names =
    {
      "evdev",
      "pc105",
      "us",
      NULL,
      NULL
    };

  hangul->xkb_context = xkb_context_new (0);
  hangul->xkb_keymap = xkb_keymap_new_from_names (hangul->xkb_context,
                                                  &names,
                                                  0);
  hangul->xkb_state = xkb_state_new (hangul->xkb_keymap);

  hangul->context = hangul_ic_new ("2");
  hangul->preedit = g_array_sized_new (TRUE, TRUE, sizeof (gunichar), 6);
}

static void
g_hangul_input_method_engine_update_preedit (GHangulInputMethodEngine *hangul)
{
  const gunichar *ucs4;
  gchar *utf8;
  gsize base_preedit_len;
  GArray *preedit;
  GInputMethodStyling styling[2];
  gsize n_styling;

  ucs4 = hangul_ic_get_preedit_string (hangul->context);

  base_preedit_len = hangul->preedit->len;
  preedit = g_array_sized_new (TRUE, TRUE, sizeof (gunichar), base_preedit_len);

  g_array_append_vals (preedit, hangul->preedit, hangul->preedit->len);
  for (; ucs4 && *ucs4 != 0; ucs4++)
    g_array_append_val (preedit, *ucs4);

  utf8 = g_ucs4_to_utf8 ((const gunichar *) preedit->data, -1,
                         NULL, NULL, NULL);
  g_assert (utf8);

  n_styling = 0;

  if (preedit->len > 0)
    {
      styling[n_styling].start = 0;
      styling[n_styling].end = preedit->len;
      styling[n_styling].type = G_INPUT_METHOD_STYLING_UNDERLINE;
      n_styling++;
    }

  if (preedit->len > base_preedit_len)
    {
      styling[n_styling].start = base_preedit_len;
      styling[n_styling].end
	= styling[n_styling].start + (preedit->len - base_preedit_len);
      styling[n_styling].type = G_INPUT_METHOD_STYLING_SELECTED;
      n_styling++;
    }

  g_input_method_engine_preedit_changed (G_INPUT_METHOD_ENGINE (hangul),
					 utf8,
					 styling,
					 n_styling,
					 preedit->len > 0 ? preedit->len : -1);
  g_free (utf8);

  g_array_free (preedit, TRUE);
}

static void
g_hangul_input_method_engine_flush (GHangulInputMethodEngine *hangul)
{
  const gunichar *ucs4;

  ucs4 = hangul_ic_flush (hangul->context);

  for (; ucs4 && *ucs4 != 0; ucs4++)
    g_array_append_val (hangul->preedit, *ucs4);

  if (hangul->preedit->len > 0)
    {
      gchar *utf8;

      utf8 = g_ucs4_to_utf8 ((const gunichar *) hangul->preedit->data,
                             -1,
                             NULL,
                             NULL,
                             NULL);
      g_assert (utf8);
      g_array_remove_range (hangul->preedit, 0, hangul->preedit->len);

      g_input_method_engine_commit (G_INPUT_METHOD_ENGINE (hangul), utf8);
      g_input_method_engine_preedit_changed (G_INPUT_METHOD_ENGINE (hangul),
					     "", NULL, 0, 0);

      g_free (utf8);
    }
}

static gboolean
g_hangul_input_method_real_key_event (GInputMethodEngine *engine,
                                      guint               keycode,
                                      gboolean            pressed)
{
  GHangulInputMethodEngine *hangul = G_HANGUL_INPUT_METHOD_ENGINE (engine);
  xkb_keysym_t keysym;
  gboolean retval;

  xkb_state_update_key (hangul->xkb_state,
			keycode,
			pressed ? XKB_KEY_DOWN : XKB_KEY_UP);

  retval = FALSE;

  /* Ignore release.  */
  if (!pressed)
    goto out;

  keysym = xkb_state_key_get_one_sym (hangul->xkb_state, keycode);

  if (keysym == XKB_KEY_Shift_L || keysym == XKB_KEY_Shift_R)
    goto out;

  if (xkb_state_mod_names_are_active (hangul->xkb_state,
				      XKB_STATE_MODS_EFFECTIVE,
				      XKB_STATE_MATCH_ANY,
				      "Control",
				      "Mod1",
				      "Mod3",
				      "Mod4",
				      "Mod5",
				      NULL))
    goto out;

  if (keysym == XKB_KEY_BackSpace)
    {
      retval = hangul_ic_backspace (hangul->context);
      if (!retval && hangul->preedit->len > 0)
        {
          hangul->preedit = g_array_remove_range (hangul->preedit,
                                                  hangul->preedit->len,
                                                  1);
          retval = TRUE;
          goto out;
        }
    }
  else
    {
      const gunichar *ucs4;

      retval = hangul_ic_process (hangul->context, keysym);

      ucs4 = hangul_ic_get_commit_string (hangul->context);
      if (ucs4 && ucs4[0] != 0)
        {
          gchar *utf8;

          utf8 = g_ucs4_to_utf8 (ucs4, -1, NULL, NULL, NULL);
          g_assert (utf8);

          g_input_method_engine_commit (engine, utf8);
          g_free (utf8);
        }

      if (!retval)
        g_hangul_input_method_engine_flush (hangul);
    }

 out:
  g_hangul_input_method_engine_update_preedit (hangul);
  return retval;
}

static GInputMethodEngine *
create_engine (GInputMethod *inputmethod,
               const gchar  *client_id,
               gpointer      user_data)
{
  return g_object_new (G_TYPE_HANGUL_INPUT_METHOD_ENGINE,
		       "client-id", client_id, NULL);
}

int
main (int argc, char **argv)
{
  GInputMethod *inputmethod;
  int status;

  inputmethod = g_input_method_new ("org.gtk.HangulInputMethod",
                                    G_APPLICATION_FLAGS_NONE,
				    NULL);
  g_signal_connect (inputmethod, "create-engine", G_CALLBACK (create_engine),
                    NULL);
#ifdef STANDALONE
  g_application_set_inactivity_timeout (G_APPLICATION (inputmethod), 10000);
#else
  g_application_set_inactivity_timeout (G_APPLICATION (inputmethod), 1000);
#endif

  status = g_application_run (G_APPLICATION (inputmethod), argc - 1, argv + 1);

  g_object_unref (inputmethod);

  g_print ("exit status: %d\n", status);

  return 0;
}
