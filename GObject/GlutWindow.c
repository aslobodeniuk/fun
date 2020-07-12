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
  gboolean called_from_display_cb;
} GlutWindow;


/* ======================= Class */
typedef struct _GlutWindowClass
{
  BaseWindowClass parent;
} GlutWindowClass;


static GlutWindow *global_self;

static void
glut_window_redraw_start (BaseWindow * base)
{
  glClearColor(0.4, 0.4, 0.4, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
//  glLoadIdentity();
}

static void
glut_window_redraw_end (BaseWindow * base)
{
  glFlush();
  glutSwapBuffers();
  if (!global_self->called_from_display_cb)
    glutPostRedisplay();
}

void glut_window_on_mouse_cb (int button, int state,
                                int x, int y)
{
  g_printf ("button = %d, state = %d, x = %d, y = %d\n",
      button, state, x, y);
}
                                

void glut_window_on_reshape_cb(int width, int height)
{
  g_printf ("width = %d, height = %d\n", width, height);
}

void glut_window_on_keyboard_cb(unsigned char key, int x, int y)
{
  g_printf ("key = %x, x = %d, y = %d\n", key, x, y);
}

void glut_window_on_special_key_cb(int key, int x, int y)
{
  g_printf ("special key = %d, x = %d, y = %d\n", key, x, y);
}


static void
glut_window_on_display_cb (void)
{
  global_self->called_from_display_cb = TRUE;
  glut_window_redraw_start ((BaseWindow *)global_self);
  
  base_window_on_display ((BaseWindow *)global_self);
  
  glut_window_redraw_end ((BaseWindow *)global_self);
  global_self->called_from_display_cb = FALSE;
}

static gpointer
glut_window_mainloop (gpointer data)
{
  glutMainLoop ();
  return NULL;
}

static void
glut_window_redraw (BaseWindow * base)
{
  glut_window_redraw_start (base);
  glut_window_redraw_end (base);
}

static void
glut_window_close (BaseWindow * base)
{
  GlutWindow *self = (GlutWindow *) base;
  
  glutLeaveMainLoop ();
  g_thread_join (self->mainloop_thr);
}


static void
glut_window_open (BaseWindow * base)
{
  int argc = 1;
  char *argv[] = { "hack", NULL };
  GlutWindow *self = (GlutWindow *) base;

  glutInit (&argc, argv);

  /* TODO: param */
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

  glutInitWindowSize (base->width, base->height);
  glutInitWindowPosition (base->x_pos, base->y_pos);

  glutCreateWindow (base->title);

  /* TODO: param */
  glEnable(GL_DEPTH_TEST);
  
  glutDisplayFunc (glut_window_on_display_cb);
  glutSpecialFunc (glut_window_on_special_key_cb);
  glutKeyboardFunc (glut_window_on_keyboard_cb);
  glutReshapeFunc (glut_window_on_reshape_cb);
  glutMouseFunc (glut_window_on_mouse_cb);

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

  g_signal_connect (self, "redraw",
      G_CALLBACK (glut_window_redraw), NULL);
}


/* =================== CLASS */

static void
glut_window_class_init (GlutWindowClass * klass)
{
  BaseWindowClass *base_class = BASE_WINDOW_GET_CLASS (klass);

  base_class->redraw_start = glut_window_redraw_start;
  base_class->redraw_end = glut_window_redraw_end;
}


G_DEFINE_TYPE (GlutWindow, glut_window, G_TYPE_BASE_WINDOW)
MY_PLUGIN_SYSTEM_PROVIDE_GTYPE (glut_window);
