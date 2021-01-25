/*****************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (c) 2000 - 2020 David Odin (aka DindinX) for MandrakeSoft
 */
/* <DESC>
 * use the libcarl in a gtk-threaded application
 * </DESC>
 */

#include <stdio.h>
#include <gtk/gtk.h>

#include <carl/carl.h>

GtkWidget *Bar;

static size_t my_write_func(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return fwrite(ptr, size, nmemb, stream);
}

static size_t my_read_func(char *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return fread(ptr, size, nmemb, stream);
}

static int my_progress_func(GtkWidget *bar,
                            double t, /* dltotal */
                            double d, /* dlnow */
                            double ultotal,
                            double ulnow)
{
/*  printf("%d / %d (%g %%)\n", d, t, d*100.0/t);*/
  gdk_threads_enter();
  gtk_progress_set_value(GTK_PROGRESS(bar), d*100.0/t);
  gdk_threads_leave();
  return 0;
}

static void *my_thread(void *ptr)
{
  CARL *carl;

  carl = carl_easy_init();
  if(carl) {
    gchar *url = ptr;
    const char *filename = "test.carl";
    FILE *outfile = fopen(filename, "wb");

    carl_easy_setopt(carl, CARLOPT_URL, url);
    carl_easy_setopt(carl, CARLOPT_WRITEDATA, outfile);
    carl_easy_setopt(carl, CARLOPT_WRITEFUNCTION, my_write_func);
    carl_easy_setopt(carl, CARLOPT_READFUNCTION, my_read_func);
    carl_easy_setopt(carl, CARLOPT_NOPROGRESS, 0L);
    carl_easy_setopt(carl, CARLOPT_PROGRESSFUNCTION, my_progress_func);
    carl_easy_setopt(carl, CARLOPT_PROGRESSDATA, Bar);

    carl_easy_perform(carl);

    fclose(outfile);
    /* always cleanup */
    carl_easy_cleanup(carl);
  }

  return NULL;
}

int main(int argc, char **argv)
{
  GtkWidget *Window, *Frame, *Frame2;
  GtkAdjustment *adj;

  /* Must initialize libcarl before any threads are started */
  carl_global_init(CARL_GLOBAL_ALL);

  /* Init thread */
  g_thread_init(NULL);

  gtk_init(&argc, &argv);
  Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  Frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(Frame), GTK_SHADOW_OUT);
  gtk_container_add(GTK_CONTAINER(Window), Frame);
  Frame2 = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(Frame2), GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(Frame), Frame2);
  gtk_container_set_border_width(GTK_CONTAINER(Frame2), 5);
  adj = (GtkAdjustment*)gtk_adjustment_new(0, 0, 100, 0, 0, 0);
  Bar = gtk_progress_bar_new_with_adjustment(adj);
  gtk_container_add(GTK_CONTAINER(Frame2), Bar);
  gtk_widget_show_all(Window);

  if(!g_thread_create(&my_thread, argv[1], FALSE, NULL) != 0)
    g_warning("can't create the thread");

  gdk_threads_enter();
  gtk_main();
  gdk_threads_leave();
  return 0;
}
