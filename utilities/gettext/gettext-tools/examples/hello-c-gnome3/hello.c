/* Example for use of GNU gettext.
   This file is in the public domain.

   Source code of the C program.  */


/* Get GTK declarations.  */
#include <gtk/gtk.h>
#include <glib/gi18n.h>

/* Get exit() declaration.  */
#include <stdlib.h>

/* Get getpid() declaration.  */
#if defined _WIN32 && !defined __CYGWIN__
/* native Windows API */
# include <process.h>
# define getpid _getpid
#else
/* POSIX API */
# include <unistd.h>
#endif

#define UI_PATH "/org/gnu/gettext/examples/hello/hello.ui"
#define APPLICATION_ID "org.gnu.gettext.examples.hello"

/* An ad-hoc struct for managing the main window.
   (Not connected to the GObject type system.)  */
struct HelloWindow
{
  GtkWindow *window;
  GtkLabel *label;
  GtkButton *button;
  gsize label_id;
  gchar *labels[3];
};

static void
update_content (struct HelloWindow *hello_window)
{
  gtk_label_set_label (hello_window->label,
                       hello_window->labels[hello_window->label_id]);
  hello_window->label_id =
    (hello_window->label_id + 1) % G_N_ELEMENTS (hello_window->labels);
}

static void
clicked_callback (GtkWidget *widget, struct HelloWindow *hello_window)
{
  update_content (hello_window);
}

static void
activate (GApplication *application, void *data)
{
  GtkBuilder *builder;
  GError *error = NULL;

  /* Instantiate the UI.  */
  builder = gtk_builder_new ();
  if (gtk_builder_add_from_resource (builder, UI_PATH, &error) == 0)
    {
      g_printerr ("Error instantiating UI: %s\n", error->message);
      g_clear_error (&error);
      exit (1);
    }

  struct HelloWindow *hello_window = g_malloc (sizeof (struct HelloWindow));
  hello_window->window = GTK_WINDOW (gtk_builder_get_object (builder, "main_window"));
  hello_window->label = GTK_LABEL (gtk_builder_get_object (builder, "label"));
  hello_window->button = GTK_BUTTON (gtk_builder_get_object (builder, "button"));

  /* Allow Pango markup in the label.  */
  gtk_label_set_use_markup (hello_window->label, TRUE);

  /* Prepare various presentations of the label.  */
  hello_window->label_id = 0;
  gchar *line1 = g_strdup_printf ("<big>%s</big>", _("Hello world!"));
  gchar *line2 =
    g_strdup_printf (_("This program is running as process number %s."),
                     g_strdup_printf ("<b>%d</b>", getpid ()));
  hello_window->labels[0] = g_strdup_printf ("%s\n%s", line1, line2);
  hello_window->labels[1] =
    g_strdup_printf ("<big><u>%s</u></big>", _("This is another text"));
  hello_window->labels[2] =
    g_strdup_printf ("<big><i>%s</i></big>", _("This is yet another text"));

  update_content (hello_window);

  /* Make sure that the application runs for as long as this window is
     still open.  */
  gtk_application_add_window (GTK_APPLICATION (application),
                              GTK_WINDOW (hello_window->window));

  g_signal_connect (hello_window->button, "clicked",
                    G_CALLBACK (clicked_callback), hello_window);
  gtk_window_present (GTK_WINDOW (hello_window->window));
}

int
main (int argc, char *argv[])
{
  GApplication *application;
  int status;

  /* Initializations.  */
  textdomain ("hello-c-gnome3");
  bindtextdomain ("hello-c-gnome3", LOCALEDIR);

  /* Create application.  */
  application =
    G_APPLICATION (gtk_application_new (APPLICATION_ID,
                                        G_APPLICATION_DEFAULT_FLAGS));
  g_signal_connect (application, "activate", G_CALLBACK (activate), NULL);

  /* Start the application.  */
  status = g_application_run (application, argc, argv);
  g_object_unref (application);

  return status;
}
