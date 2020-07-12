/* Compile:
 * gcc my-plugin-system-inspect.c -o my-plugin-system-inspect $(pkg-config --cflags --libs gobject-2.0) $(pkg-config --libs gmodule-2.0)
 */

#include "my-plugin-system.h"
#include <glib/gstdio.h>
#include <gmodule.h>
#include <string.h>
#include <stdlib.h>

void _strtoint (const GValue *src_value,
                    GValue *dest_value)
{
  guint ret = 0;
  const gchar * s = g_value_get_string (src_value);

  if (s) {
    ret = atoi (s);
  } else
    g_warning ("couldn't convert string %s to uint", s);

  g_value_set_uint (dest_value, ret);
}

int
main (int argc, char **argv)
{
  GModule *module = NULL;
  gpointer ptr;
  MyPluginSystemGetGtypeFunc get_type_f;
  int ret = 1;

  g_value_register_transform_func (G_TYPE_STRING,
      G_TYPE_UINT,
      _strtoint);
  
  if (argc != 2) {
    g_printf ("my-plugin-system-inspect <plugin>");
    return 1;
  }

  module = g_module_open (argv[1], G_MODULE_BIND_LOCAL);
  if (!module) {
    g_warning ("Failed to load plugin '%s': %s", argv[1], g_module_error ());
    goto beach;
  }

  if (!g_module_symbol (module, "my_plugin_system_get_gtype", &ptr)) {
    g_warning ("File '%s' is not a plugin from my system", argv[1]);
    goto beach;
  }

  get_type_f = (MyPluginSystemGetGtypeFunc) ptr;

  {
    GTypeQuery query;
    GType plugin_type = get_type_f ();

    g_printf ("=============  '%s':\n", argv[1]);
    g_type_query (plugin_type, &query);
    g_printf ("GType name = '%s'\n", query.type_name);

    if (G_TYPE_IS_OBJECT (plugin_type)) {
      GObject *obj;
      GParamSpec **properties;
      guint n_properties, p;
      guint *signals, s, n_signals;

      g_printf ("GType is a GObject.\n");

      obj = g_object_new (plugin_type, NULL);

      properties =
          g_object_class_list_properties (G_OBJECT_GET_CLASS (obj),
          &n_properties);

      for (p = 0; p < n_properties; p++) {
        GParamSpec *prop = properties[p];
        gchar *def_val;

        def_val =
            g_strdup_value_contents (g_param_spec_get_default_value (prop));

        g_printf ("- Property: (%s) %s = %s\n",
            G_PARAM_SPEC_TYPE_NAME (prop),
            g_param_spec_get_name (prop), def_val);

        g_printf ("\tnick = '%s', %s\n\n",
            g_param_spec_get_nick (prop), g_param_spec_get_blurb (prop));
        g_free (def_val);
      }


      /* Iterate signals */
      {
        GType t;
        const gchar *_tab = "  ";
        gchar *tab = g_strdup (_tab);
        g_printf ("--- Signals:\n");
        for (t = plugin_type; t && t != G_TYPE_OBJECT; t = g_type_parent (t)) {
          gchar *tmptab;
          
          signals = g_signal_list_ids (t, &n_signals);

          for (s = 0; s < n_signals; s++) {
            GSignalQuery query;
            GTypeQuery t_query;

            g_signal_query (signals[s], &query);
            g_type_query (t, &t_query);

            g_printf ("%s%s:: (* %s) ", tab, t_query.type_name, query.signal_name);

            g_printf ("(");
            for (p = 0; p < query.n_params; p++) {
              GTypeQuery pquery;
              g_type_query (query.param_types[p], &pquery);
              g_printf ("%s%s", p ? ", " : "", pquery.type_name);
            }
            g_printf (");\n");
          }

          tmptab = tab;
          tab = g_strdup_printf ("%s%s", _tab, tab);
          g_free (tmptab);
          g_free (signals);
        }
      }

      g_printf ("--- Entering command shell...\n");

      
      for (;;) {
        char a[512];
        char ** tokens;
        g_printf ("(q to quit), (call <signal>), (set <parameter> <val>)\n\t# ");
        fgets(a, sizeof(a), stdin);
        a [strlen (a) - 1] = 0; 

        g_printf ("a = %s\n", a);
        
        tokens = g_strsplit (a, " ", 0);

         if (tokens && tokens [0] && !g_strcmp0 (tokens [0], "q"))
          break;
        
        if (tokens && tokens [0] && tokens [1]) {
          /* Call */
          if (!g_strcmp0 (tokens [0], "call")) {
            g_printf ("calling %s ()\n", tokens [1]);
            g_signal_emit_by_name (obj, tokens [1], NULL);
          }
          
          if (tokens [2] && !g_strcmp0 (tokens [0], "set")) {
            GValue inp = G_VALUE_INIT;
            GValue outp = G_VALUE_INIT;
            uint set;
            g_value_init (&inp, G_TYPE_STRING);
            g_value_init (&outp, G_TYPE_UINT);

            g_printf ("setting %s to %s\n", tokens [1], tokens [2]);
            
            g_value_set_string (&inp, tokens [2]);
            g_value_transform (&inp, &outp);

            set = g_value_get_uint (&outp);
            
            g_printf ("setting %s to %d\n", tokens [1], set);
            g_object_set (obj, tokens [1], g_value_get_uint (&outp), NULL);
          }
          
          g_strfreev (tokens);
        }
      }

      g_object_unref (obj);
      g_free (properties);
    }
  }

  ret = 0;
beach:
  if (module)
    g_module_close (module);
  return ret;
}
