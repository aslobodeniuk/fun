/* Example code of the GObject Class Mutation
 *
 * Copyright (C) 2023 Alexander Slobodeniuk <aslobodeniuk@fluendo.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


/* Compiling:
 * Needs GStreamer development package
 * $ cc gobject-mutation.c $(pkg-config --cflags --libs gstreamer-1.0) -o gobject-mutation
 */

#include <gst/gst.h>

GstElementClass *parent_class;
typedef struct
{
  int abc;
} MutatorClass;

typedef struct
{
  guint num;
} Mutator;

static guint gmo_dna_class_offset;
static guint gmo_dna_instance_offset;
static MutatorClass *
gmo_get_class (gpointer class)
{
  return (MutatorClass *) (class + gmo_dna_class_offset);
}

static Mutator *
gmo_get_mutator (gpointer self)
{
  return (Mutator *) (self + gmo_dna_instance_offset);
}

static GstStateChangeReturn
gmo_override_change_state (GstElement * element, GstStateChange transition)
{
  Mutator *mut = gmo_get_mutator (element);

  g_message ("Changing state to %s. Will count it as number %d",
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)),
      ++mut->num);
  return parent_class->change_state (element, transition);
}

static void
gmo_class_init (gpointer dna, gpointer class_data)
{
  MutatorClass *klass;
  GstElementClass *element_class = (GstElementClass *) dna;

  parent_class = g_type_class_peek_parent (dna);

  klass = gmo_get_class (dna);
  klass->abc = 123;

  element_class->change_state = gmo_override_change_state;
}

static void
gmo_init (GTypeInstance * dna, gpointer klass)
{
  Mutator *mut;

  mut = gmo_get_mutator (dna);
  mut->num = 0;

  g_message ("Hello from %s, the child of %s", G_OBJECT_CLASS_NAME (klass),
      G_OBJECT_CLASS_NAME (parent_class));
}

static GType
gmo_get_mutant_type (GType dna)
{
  static GType ret;
  GTypeQuery dna_info;

  if (G_LIKELY (ret))
    return ret;

  g_type_query (dna, &dna_info);
  /* set offsets */
  gmo_dna_class_offset = dna_info.class_size;
  gmo_dna_instance_offset = dna_info.instance_size;

  return (ret = g_type_register_static_simple
      (dna, "MyMutantType",
          dna_info.class_size + sizeof (MutatorClass),
          gmo_class_init, dna_info.instance_size + sizeof (Mutator), gmo_init,
          /* Definitelly not an abstract type. Maybe next time.. */
          0));
}

int
main (int argc, char *argv[])
{
  GType dna_type;
  GType mutated_dna_type;
  GstElement *pipe;

  /* Register GStreamer's gtypes */
  gst_init (&argc, &argv);

  gst_object_unref (gst_element_factory_make ("videotestsrc", NULL));

  /* Fetch type of fakesrc element */
  dna_type = g_type_from_name ("GstVideoTestSrc");
  if (0 == dna_type) {
    g_error ("Couldn't find DNA type");
  }

  mutated_dna_type = gmo_get_mutant_type (dna_type);
  if (0 == mutated_dna_type) {
    g_error ("Couldn't register mutated type");
  }

  /* Register our mutant as GstElement */
  if (!gst_element_register (NULL, "mutated_videotestsrc", 999,
          mutated_dna_type)) {
    g_error ("Couldn't register plugin");
  }

  pipe = gst_parse_launch ("mutated_videotestsrc ! autovideosink", NULL);
  g_assert (NULL != pipe);
  gst_element_set_state (pipe, GST_STATE_PLAYING);

  /* Play for 1 second */
  g_usleep (G_TIME_SPAN_SECOND);

  gst_element_set_state (pipe, GST_STATE_NULL);
  gst_object_unref (pipe);
  return 0;
}