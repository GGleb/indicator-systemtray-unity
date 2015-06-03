/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

gnomebg.c: Object for the desktop background.

Copyright (C) 2000 Eazel, Inc.
Copyright (C) 2007-2008 Red Hat, Inc.
Copyright (C) 2011 Canonical Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this program; if not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

Derived from eel-background.c and eel-gdk-pixbuf-extensions.c by
Darin Adler <darin@eazel.com> and Ramiro Estrugo <ramiro@eazel.com>

Authors: Soren Sandmann <sandmann@redhat.com>
         Gordon Allott <gord.allott@canonical.com>

*/

/* Basically ripped apart gnome-bg so i can get access to the slideshow
 * data, so i can sync with gnome-bg's transitions without using gnome-bg's
 * transitions
 */

#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>

#include <glib/gstdio.h>
#include <gio/gio.h>

#include <gdk/gdkx.h>

#include "gnome-bg-slideshow.h"




static double
now (void)
{
	GTimeVal tv;

	g_get_current_time (&tv);

	return (double)tv.tv_sec + (tv.tv_usec / 1000000.0);
}

Slide *
get_current_slide (SlideShow *show,
		   double    *alpha)
{
	double delta = fmod (now() - show->start_time, show->total_duration);
	GList *list;
	double elapsed;
	int i;

	if (delta < 0)
		delta += show->total_duration;

	elapsed = 0;
	i = 0;
	for (list = show->slides->head; list != NULL; list = list->next) {
		Slide *slide = list->data;

		if (elapsed + slide->duration > delta) {
			if (alpha)
				*alpha = (delta - elapsed) / (double)slide->duration;
			return slide;
		}

		i++;
		elapsed += slide->duration;
	}

	/* this should never happen since we have slides and we should always
	 * find a current slide for the elapsed time since beginning -- we're
	 * looping with fmod() */
	g_assert_not_reached ();

	return NULL;
}


double
get_slide_timeout (Slide   *slide)
{
	double timeout;
	if (slide->fixed) {
		timeout = slide->duration;
	} else {
		/* Maybe the number of steps should be configurable? */

		/* In the worst case we will do a fade from 0 to 256, which mean
		 * we will never use more than 255 steps, however in most cases
		 * the first and last value are similar and users can't percieve
		 * changes in pixel values as small as 1/255th. So, lets not waste
		 * CPU cycles on transitioning to often.
		 *
		 * 64 steps is enough for each step to be just detectable in a 16bit
		 * color mode in the worst case, so we'll use this as an approximation
		 * of whats detectable.
		 */
		timeout = slide->duration / 64.0;
	}
	return timeout;
}

static gboolean stack_is (SlideShow *parser, const char *s1, ...);

/* Parser for fading background */
static void
handle_start_element (GMarkupParseContext *context,
		      const gchar         *name,
		      const gchar        **attr_names,
		      const gchar        **attr_values,
		      gpointer             user_data,
		      GError             **err)
{
	SlideShow *parser = user_data;
	gint i;

	if (strcmp (name, "static") == 0 || strcmp (name, "transition") == 0) {
		Slide *slide = g_new0 (Slide, 1);

		if (strcmp (name, "static") == 0)
			slide->fixed = TRUE;

		g_queue_push_tail (parser->slides, slide);
	}
	else if (strcmp (name, "size") == 0) {
		Slide *slide = parser->slides->tail->data;
		FileSize *size = g_new0 (FileSize, 1);
		for (i = 0; attr_names[i]; i++) {
			if (strcmp (attr_names[i], "width") == 0)
				size->width = atoi (attr_values[i]);
			else if (strcmp (attr_names[i], "height") == 0)
				size->height = atoi (attr_values[i]);
		}
		if (parser->stack->tail &&
		    (strcmp (parser->stack->tail->data, "file") == 0 ||
		     strcmp (parser->stack->tail->data, "from") == 0)) {
			slide->file1 = g_slist_prepend (slide->file1, size);
		}
		else if (parser->stack->tail &&
			 strcmp (parser->stack->tail->data, "to") == 0) {
			slide->file2 = g_slist_prepend (slide->file2, size);
		}
	}
	g_queue_push_tail (parser->stack, g_strdup (name));
}

static void
handle_end_element (GMarkupParseContext *context,
		    const gchar         *name,
		    gpointer             user_data,
		    GError             **err)
{
	SlideShow *parser = user_data;

	g_free (g_queue_pop_tail (parser->stack));
}

static gboolean
stack_is (SlideShow *parser,
	  const char *s1,
	  ...)
{
	GList *stack = NULL;
	const char *s;
	GList *l1, *l2;
	va_list args;

	stack = g_list_prepend (stack, (gpointer)s1);

	va_start (args, s1);

	s = va_arg (args, const char *);
	while (s) {
		stack = g_list_prepend (stack, (gpointer)s);
		s = va_arg (args, const char *);
	}

	l1 = stack;
	l2 = parser->stack->head;

	while (l1 && l2) {
		if (strcmp (l1->data, l2->data) != 0) {
			g_list_free (stack);
			return FALSE;
		}

		l1 = l1->next;
		l2 = l2->next;
	}

	g_list_free (stack);

	return (!l1 && !l2);
}

static int
parse_int (const char *text)
{
	return strtol (text, NULL, 0);
}

static void
handle_text (GMarkupParseContext *context,
	     const gchar         *text,
	     gsize                text_len,
	     gpointer             user_data,
	     GError             **err)
{
	SlideShow *parser = user_data;
	Slide *slide = parser->slides->tail? parser->slides->tail->data : NULL;
	FileSize *fs;
	gint i;

	if (stack_is (parser, "year", "starttime", "background", NULL)) {
		parser->start_tm.tm_year = parse_int (text) - 1900;
	}
	else if (stack_is (parser, "month", "starttime", "background", NULL)) {
		parser->start_tm.tm_mon = parse_int (text) - 1;
	}
	else if (stack_is (parser, "day", "starttime", "background", NULL)) {
		parser->start_tm.tm_mday = parse_int (text);
	}
	else if (stack_is (parser, "hour", "starttime", "background", NULL)) {
		parser->start_tm.tm_hour = parse_int (text) - 1;
	}
	else if (stack_is (parser, "minute", "starttime", "background", NULL)) {
		parser->start_tm.tm_min = parse_int (text);
	}
	else if (stack_is (parser, "second", "starttime", "background", NULL)) {
		parser->start_tm.tm_sec = parse_int (text);
	}
	else if (stack_is (parser, "duration", "static", "background", NULL) ||
		 stack_is (parser, "duration", "transition", "background", NULL)) {
		slide->duration = g_strtod (text, NULL);
		parser->total_duration += slide->duration;
	}
	else if (stack_is (parser, "file", "static", "background", NULL) ||
		 stack_is (parser, "from", "transition", "background", NULL)) {
		for (i = 0; text[i]; i++) {
			if (!g_ascii_isspace (text[i]))
				break;
		}
		if (text[i] == 0)
			return;
		fs = g_new (FileSize, 1);
		fs->width = -1;
		fs->height = -1;
		fs->file = g_strdup (text);
		slide->file1 = g_slist_prepend (slide->file1, fs);
		if (slide->file1->next != NULL)
			parser->has_multiple_sizes = TRUE;
	}
	else if (stack_is (parser, "size", "file", "static", "background", NULL) ||
		 stack_is (parser, "size", "from", "transition", "background", NULL)) {
		fs = slide->file1->data;
		fs->file = g_strdup (text);
		if (slide->file1->next != NULL)
			parser->has_multiple_sizes = TRUE;
	}
	else if (stack_is (parser, "to", "transition", "background", NULL)) {
		for (i = 0; text[i]; i++) {
			if (!g_ascii_isspace (text[i]))
				break;
		}
		if (text[i] == 0)
			return;
		fs = g_new (FileSize, 1);
		fs->width = -1;
		fs->height = -1;
		fs->file = g_strdup (text);
		slide->file2 = g_slist_prepend (slide->file2, fs);
		if (slide->file2->next != NULL)
			parser->has_multiple_sizes = TRUE;
	}
	else if (stack_is (parser, "size", "to", "transition", "background", NULL)) {
		fs = slide->file2->data;
		fs->file = g_strdup (text);
		if (slide->file2->next != NULL)
			parser->has_multiple_sizes = TRUE;
	}
}

SlideShow *
slideshow_ref (SlideShow *show)
{
	show->ref_count++;
	return show;
}

void
slideshow_unref (SlideShow *show)
{
	GList *list;
	GSList *slist;
	FileSize *size;

	show->ref_count--;
	if (show->ref_count > 0)
		return;

	for (list = show->slides->head; list != NULL; list = list->next) {
		Slide *slide = list->data;

		for (slist = slide->file1; slist != NULL; slist = slist->next) {
			size = slist->data;
			g_free (size->file);
			g_free (size);
		}
		g_slist_free (slide->file1);

		for (slist = slide->file2; slist != NULL; slist = slist->next) {
			size = slist->data;
			g_free (size->file);
			g_free (size);
		}
		g_slist_free (slide->file2);

		g_free (slide);
	}

	g_queue_free (show->slides);

	g_list_foreach (show->stack->head, (GFunc) g_free, NULL);
	g_queue_free (show->stack);

	g_free (show);
}

static void
threadsafe_localtime (time_t threadsafe_time, struct tm *tm)
{
	struct tm *res;

	G_LOCK_DEFINE_STATIC (localtime_mutex);

	G_LOCK (localtime_mutex);

	res = localtime (&threadsafe_time);
	if (tm) {
		*tm = *res;
	}

	G_UNLOCK (localtime_mutex);
}

SlideShow *
read_slideshow_file (const char *filename,
		     GError     **err)
{
	GMarkupParser parser = {
		handle_start_element,
		handle_end_element,
		handle_text,
		NULL, /* passthrough */
		NULL, /* error */
	};

	GFile *file;
	char *contents = NULL;
	gsize len;
	SlideShow *show = NULL;
	GMarkupParseContext *context = NULL;
	time_t t;

	if (!filename)
		return NULL;

	file = g_file_new_for_path (filename);
	if (!g_file_load_contents (file, NULL, &contents, &len, NULL, NULL)) {
		g_object_unref (file);
		return NULL;
	}
	g_object_unref (file);

	show = g_new0 (SlideShow, 1);
	show->ref_count = 1;
	threadsafe_localtime ((time_t)0, &show->start_tm);
	show->stack = g_queue_new ();
	show->slides = g_queue_new ();

	context = g_markup_parse_context_new (&parser, 0, show, NULL);

	if (!g_markup_parse_context_parse (context, contents, len, err)) {
		slideshow_unref (show);
		show = NULL;
	}


	if (show) {
		if (!g_markup_parse_context_end_parse (context, err)) {
			slideshow_unref (show);
			show = NULL;
		}
	}

	g_markup_parse_context_free (context);

	if (show) {
		int show_len;

		t = mktime (&show->start_tm);

		show->start_time = (double)t;

		show_len = g_queue_get_length (show->slides);

		/* no slides, that's not a slideshow */
		if (show_len == 0) {
			slideshow_unref (show);
			show = NULL;
		/* one slide, there's no transition */
		} else if (show_len == 1) {
			Slide *slide = show->slides->head->data;
			slide->duration = G_MAXUINT;
		}
	}

	g_free (contents);

	return show;
}
