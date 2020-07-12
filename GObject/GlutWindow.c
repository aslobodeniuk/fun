/* Compilation:
   gcc -fpic -shared -lglut -lGL $(pkg-config --cflags --libs gobject-2.0) GlutWindow.c -o GlutWindow.so
*/

/* https://www.linuxjournal.com/content/introduction-opengl-programming */

#include "base/basewindow.h"
#include "my-plugin-system.h"
#include <glib/gstdio.h>
#include "GL/freeglut.h"
#include "GL/gl.h"


/* ======================= Instance */
typedef struct _GlutWindow
{
  BaseWindow parent;

  GThread *mainloop_thr;
} GlutWindow;


/* ======================= Class */
typedef struct _GlutWindowClass
{
  BaseWindowClass parent;
} GlutWindowClass;


static GlutWindow *global_self;

static void
glut_window_on_display_cb (void)
{
  base_window_on_display ((BaseWindow *)global_self);
}

static gpointer
glut_window_mainloop (gpointer data)
{
  glutMainLoop ();
  return NULL;
}

static void
glut_window_close (BaseWindow * base)
{
  GlutWindow *self = (GlutWindow *) base;
  
  glutLeaveMainLoop ();
  g_thread_join (self->mainloop_thr);
}

//static void
//glut_window_add_drawable (GlutWindow * self, GObject * drawable)
///{
// return;
//}

static void
glut_window_open (BaseWindow * base)
{
  int argc = 1;
  char *argv[] = { "hack", NULL };
  GlutWindow *self = (GlutWindow *) base;

  glutInit (&argc, argv);

  glutInitDisplayMode (GLUT_SINGLE);

  glutInitWindowSize (base->width, base->height);
  glutInitWindowPosition (base->x_pos, base->y_pos);

  glutCreateWindow (base->title);

  glutDisplayFunc (glut_window_on_display_cb);

  self->mainloop_thr =
      g_thread_new ("glutMainLoop", glut_window_mainloop, NULL);
}


static void
glut_window_init (GlutWindow * self)
{
  global_self = self;

  g_signal_connect (self, "open",
      G_CALLBACK (glut_window_open), NULL);

  g_signal_connect (self, "close",
      G_CALLBACK (glut_window_close), NULL);

}


/*static void
glut_window_dispose (GObject *gobject)
{
  GlutWindow *self = (GlutWindow *) gobject;

  glut_window_close (self);
}
*/

/* =================== CLASS */

static void
glut_window_class_init (GlutWindowClass * klass)
{
  BaseWindowClass *base_class = BASE_WINDOW_GET_CLASS (klass);

  g_printf ("yes!!! %p\n", base_class);
//  base_class->open = glut_window_open;
//  base_class->close = glut_window_close;
}


G_DEFINE_TYPE (GlutWindow, glut_window, G_TYPE_BASE_WINDOW)
MY_PLUGIN_SYSTEM_PROVIDE_GTYPE (glut_window);
