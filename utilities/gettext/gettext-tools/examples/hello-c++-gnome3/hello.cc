/* Example for use of GNU gettext.
   This file is in the public domain.

   Source code of the C++ program.  */


/* Get GTKmm declarations.  */
#include <gtkmm.h>
#include <glib/gi18n.h>

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
  Gtk::Window *window;
  Gtk::Label *label;
  Gtk::Button *button;
  gsize label_id;
  gchar *labels[3];
};

static void
update_content (HelloWindow *hello_window)
{
  hello_window->label->set_label (hello_window->labels[hello_window->label_id]);
  hello_window->label_id =
    (hello_window->label_id + 1) % G_N_ELEMENTS (hello_window->labels);
}

static void
clicked_callback (HelloWindow *hello_window)
{
  update_content (hello_window);
}

static void
activate (Glib::RefPtr<Gtk::Application> application)
{
  /* Instantiate the UI.  */
  Glib::RefPtr<Gtk::Builder> builder =
    Gtk::Builder::create_from_resource (UI_PATH);

  HelloWindow *hello_window = new HelloWindow ();
  hello_window->window = nullptr;
  builder->get_widget<Gtk::Window> ("main_window", hello_window->window);
  hello_window->label = nullptr;
  builder->get_widget<Gtk::Label> ("label", hello_window->label);
  hello_window->button = nullptr;
  builder->get_widget<Gtk::Button> ("button", hello_window->button);

  /* Allow Pango markup in the label.  */
  hello_window->label->set_use_markup (true);

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
  application->add_window (*(hello_window->window));

  hello_window->button->signal_clicked ()
    .connect (sigc::bind (sigc::ptr_fun (clicked_callback), hello_window));
  hello_window->window->present ();
}

int
main (int argc, char *argv[])
{
  /* Initializations.  */
  textdomain ("hello-c++-gnome3");
  bindtextdomain ("hello-c++-gnome3", LOCALEDIR);

  /* Create application.  */
  Glib::RefPtr<Gtk::Application> application =
    Gtk::Application::create (APPLICATION_ID, Gio::APPLICATION_FLAGS_NONE);
  application->signal_activate ()
    .connect (sigc::bind (sigc::ptr_fun (activate), application));

  /* Start the application.  */
  return application->run (argc, argv);
}
