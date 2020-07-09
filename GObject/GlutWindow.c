/* Compilation:
   gcc -fpic -c GlutWindow.c -lglut -lGL $(pkg-config --cflags --libs gobject-2.0) -o GlutWindow.o 
   gcc -shared GlutWindow.o -fpic -o GlutWindow.so -lglut -lGL $(pkg-config --cflags --libs gobject-2.0)
*/

/* https://www.linuxjournal.com/content/introduction-opengl-programming */

#include <glib-object.h>
#include "GL/freeglut.h"
#include "GL/gl.h"
#include "my-plugin-system.h"
#include <glib/gstdio.h>

/* ======================= Instance */
typedef struct _GlutWindow
{
  GObject parent;

  /* instance members */
  int width;
  int height;
  int x_pos;
  int y_pos;

  gchar *title;

  GThread *mainloop_thr;
} GlutWindow;


/* ======================= Class */
typedef struct _GlutWindowClass
{
  GObjectClass parent;

  void (*on_display) (GlutWindow *);
} GlutWindowClass;

enum
{
  SIGNAL_ON_DISPLAY,
  LAST_SIGNAL
};

static guint glut_window_signals[LAST_SIGNAL] = { 0 };

static GlutWindow *global_self;

/*  */
static void
glut_window_on_display_cb (void)
{
  /* Chain up to user */
  g_signal_emit (global_self, glut_window_signals[SIGNAL_ON_DISPLAY], 0);
}

static gpointer
glut_window_mainloop (gpointer data)
{
  glutMainLoop ();
  return NULL;
}

static void
glut_window_close (GlutWindow * self)
{
  glutLeaveMainLoop ();
  g_thread_join (self->mainloop_thr);
}

static void
glut_window_open (GlutWindow * self)
{
  int argc = 1;
  char *argv[] = { "hack", NULL };

  glutInit (&argc, argv);

  glutInitDisplayMode (GLUT_SINGLE);

  glutInitWindowSize (self->width, self->height);
  glutInitWindowPosition (self->x_pos, self->y_pos);

  glutCreateWindow (self->title);

  glutDisplayFunc (glut_window_on_display_cb);

  self->mainloop_thr =
      g_thread_new ("glutMainLoop", glut_window_mainloop, NULL);
}

/* ================= PROPERTIES */
typedef enum
{
  PROP_WIDTH = 1,
  PROP_HEIGHT,
  PROP_X_POS,
  PROP_Y_POS,
  PROP_TITLE,
  N_PROPERTIES
} GlutWindowProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
glut_window_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  GlutWindow *self = (GlutWindow *) object;

  switch ((GlutWindowProperty) property_id) {
    case PROP_TITLE:
      g_free (self->title);
      self->title = g_value_dup_string (value);
      break;

    case PROP_WIDTH:
      self->width = g_value_get_uint (value);
      break;

    case PROP_HEIGHT:
      self->height = g_value_get_uint (value);
      break;

    case PROP_X_POS:
      self->x_pos = g_value_get_uint (value);
      break;

    case PROP_Y_POS:
      self->y_pos = g_value_get_uint (value);
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
glut_window_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  GlutWindow *self = (GlutWindow *) object;

  switch ((GlutWindowProperty) property_id) {
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    case PROP_WIDTH:
      g_value_set_uint (value, self->width);
      break;

    case PROP_HEIGHT:
      g_value_set_uint (value, self->height);
      break;


    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


/* =================== CLASS */

static void
glut_window_class_init (GlutWindowClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = glut_window_set_property;
  object_class->get_property = glut_window_get_property;

  obj_properties[PROP_TITLE] =
      g_param_spec_string ("title",
      "Window title",
      "Window title",
      "Glut Window", G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  obj_properties[PROP_WIDTH] =
      g_param_spec_uint ("width",
      "Window width", "Window width", 0 /* minimum value */ ,
      G_MAXUINT /* maximum value */ ,
      500 /* default value */ ,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  obj_properties[PROP_HEIGHT] =
      g_param_spec_uint ("height",
      "Window height", "Window height", 0 /* minimum value */ ,
      G_MAXUINT /* maximum value */ ,
      500 /* default value */ ,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  obj_properties[PROP_X_POS] =
      g_param_spec_uint ("x-pos",
      "Window position X", "Window position X", 0 /* minimum value */ ,
      G_MAXUINT /* maximum value */ ,
      100 /* default value */ ,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  obj_properties[PROP_Y_POS] =
      g_param_spec_uint ("y-pos",
      "Window position Y", "Window position Y", 0 /* minimum value */ ,
      G_MAXUINT /* maximum value */ ,
      100 /* default value */ ,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
      N_PROPERTIES, obj_properties);


  glut_window_signals[SIGNAL_ON_DISPLAY] =
      g_signal_new ("on-display", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GlutWindowClass, on_display), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE);
}

static void
glut_window_init (GlutWindow * self)
{
  global_self = self;
}


static void
glut_window_dispose (GlutWindow * self)
{
  glut_window_close (self);
}


G_DEFINE_TYPE (GlutWindow, glut_window, G_TYPE_OBJECT)
MY_PLUGIN_SYSTEM_PROVIDE_GTYPE (glut_window);
