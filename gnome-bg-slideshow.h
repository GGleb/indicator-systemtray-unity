/* gnome-bg.h -

   Copyright 2007, Red Hat, Inc.

   This file is part of the Gnome Library.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Soren Sandmann <sandmann@redhat.com>
*/

#ifndef __GNOME_BG_SLIDESHOW__
#define __GNOME_BG_SLIDESHOW__

#include <glib.h>

G_BEGIN_DECLS
typedef struct _SlideShow SlideShow;
typedef struct _Slide Slide;
typedef struct _FileSize FileSize;

struct _FileSize
{
	gint width;
	gint height;

	char *file;
};
struct _Slide
{
	double   duration;		/* in seconds */
	gboolean fixed;

	GSList  *file1;
	GSList  *file2;		/* NULL if fixed is TRUE */
};

struct _SlideShow
{
	gint ref_count;
	double start_time;
	double total_duration;

	GQueue *slides;

	gboolean has_multiple_sizes;

	/* used during parsing */
	struct tm start_tm;
	GQueue *stack;
};

Slide     *get_current_slide    (SlideShow 	      *show,
																 double    	      *alpha);
SlideShow *read_slideshow_file  (const char *filename,
																 GError     **err);
SlideShow *slideshow_ref        (SlideShow  *show);
void       slideshow_unref      (SlideShow  *show);


G_END_DECLS

#endif
