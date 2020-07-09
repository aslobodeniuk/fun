/* Compile:
 * gcc my-plugin-system-inspect.c -o my-plugin-system-inspect $(pkg-config --cflags --libs gobject-2.0) $(pkg-config --libs gmodule-2.0)
 */

#include "my-plugin-system.h"
#include <glib/gstdio.h>
#include <gmodule.h>

int
main (int argc, char **argv)
{
  GModule *module = NULL;
  gpointer ptr;
  MyPluginSystemGetGtypeFunc get_type_f;
  int ret = 1;

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

      g_printf ("GType is a GObject.\n");

      obj = g_object_new (plugin_type, NULL);

      properties =
          g_object_class_list_properties (G_OBJECT_GET_CLASS (obj),
          &n_properties);

      for (p = 0; p < n_properties; p++) {
        GParamSpec *prop = properties[p];
        gchar *def_val;

        g_printf ("* Property: '%s', '%s', '%s'\n",
            g_param_spec_get_blurb (prop),
            g_param_spec_get_nick (prop), g_param_spec_get_name (prop));

        def_val =
            g_strdup_value_contents (g_param_spec_get_default_value (prop));
        g_printf ("\tDefault value: %s\n", def_val);
        g_free (def_val);
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
