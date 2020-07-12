#include "base/basewindow.h"

enum
{
  SIGNAL_ON_DISPLAY,
  SIGNAL_OPEN,
  SIGNAL_CLOSE,
  SIGNAL_REDRAW,
  LAST_SIGNAL
};

static guint base_window_signals[LAST_SIGNAL] = { 0 };

/* ================= PROPERTIES */
typedef enum
{
  PROP_WIDTH = 1,
  PROP_HEIGHT,
  PROP_X_POS,
  PROP_Y_POS,
  PROP_TITLE,
  N_PROPERTIES
} BaseWindowProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

void base_window_on_display (BaseWindow * self) {
  /* Chain up to user */
  g_signal_emit (self, base_window_signals[SIGNAL_ON_DISPLAY], 0);
}


void base_window_redraw (BaseWindow * self) {
  BaseWindowClass *klass = BASE_WINDOW_GET_CLASS (self);

  klass->redraw_start (self);
  g_signal_emit (self, base_window_signals[SIGNAL_REDRAW], 0);
  klass->redraw_end (self);
}


static void
base_window_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  BaseWindow *self = (BaseWindow *) object;

  switch ((BaseWindowProperty) property_id) {
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
base_window_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  BaseWindow *self = (BaseWindow *) object;

  switch ((BaseWindowProperty) property_id) {
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    case PROP_WIDTH:
      g_value_set_uint (value, self->width);
      break;

    case PROP_HEIGHT:
      g_value_set_uint (value, self->height);
      break;

    case PROP_X_POS:
      g_value_set_uint (value, self->x_pos);
      break;

    case PROP_Y_POS:
      g_value_set_uint (value, self->y_pos);
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
base_window_init (BaseWindow * self)
{
}


static void
base_window_dispose (GObject *gobject)
{
  BaseWindow *self = (BaseWindow *) gobject;
  BaseWindowClass *klass = BASE_WINDOW_GET_CLASS (self);

  if (self->opened)
    klass->close (self);
}

/* =================== CLASS */

static void
base_window_class_init (BaseWindowClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = base_window_dispose;
  object_class->set_property = base_window_set_property;
  object_class->get_property = base_window_get_property;

  obj_properties[PROP_TITLE] =
      g_param_spec_string ("title",
      "Window title",
      "Window title",
      "Base Window", G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

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


  base_window_signals[SIGNAL_ON_DISPLAY] =
      g_signal_new ("on-display", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (BaseWindowClass, on_display), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE);

  base_window_signals[SIGNAL_OPEN] =
      g_signal_new ("open", G_TYPE_FROM_CLASS (klass),
          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
          G_STRUCT_OFFSET (BaseWindowClass, open), NULL, NULL,
          g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE);

  base_window_signals[SIGNAL_CLOSE] =
      g_signal_new ("close", G_TYPE_FROM_CLASS (klass),
          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
          G_STRUCT_OFFSET (BaseWindowClass, close), NULL, NULL,
          g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE);

  base_window_signals[SIGNAL_REDRAW] =
      g_signal_new ("redraw", G_TYPE_FROM_CLASS (klass),
          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
          G_STRUCT_OFFSET (BaseWindowClass, redraw), NULL, NULL,
          g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE);
  
}


G_DEFINE_TYPE (BaseWindow, base_window, G_TYPE_OBJECT)
