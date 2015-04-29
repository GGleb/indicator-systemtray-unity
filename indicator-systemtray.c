/*
An indicator to display system tray in Unity.

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Indicator Stuff */
#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
//#include <libindicator/indicator-service-manager.h>

//#include <unity-misc/na-tray-manager.h>
//#include <unity-misc/na-tray-child.h>
#include <unity-misc/na-tray.h>

#define INDICATOR_SYSTEMTRAY_TYPE            (indicator_systemtray_get_type ())
#define INDICATOR_SYSTEMTRAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_SYSTEMTRAY_TYPE, IndicatorSystemtray))
#define INDICATOR_SYSTEMTRAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_SYSTEMTRAY_TYPE, IndicatorSystemtrayClass))
#define IS_INDICATOR_SYSTEMTRAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_SYSTEMTRAY_TYPE))
#define IS_INDICATOR_SYSTEMTRAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_SYSTEMTRAY_TYPE))
#define INDICATOR_SYSTEMTRAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_SYSTEMTRAY_TYPE, IndicatorSystemtrayClass))

typedef struct _IndicatorSystemtray         IndicatorSystemtray;
typedef struct _IndicatorSystemtrayClass    IndicatorSystemtrayClass;
typedef struct _IndicatorSystemtrayPrivate  IndicatorSystemtrayPrivate;

struct _IndicatorSystemtrayClass {
  IndicatorObjectClass parent_class;
};

struct _IndicatorSystemtray {
  IndicatorObject parent;
  IndicatorSystemtrayPrivate *priv;
};

struct _IndicatorSystemtrayPrivate {
  GtkWidget   *window;
  NaTray      *tray;
  gint        count_tray_icon;
  gint        x;
  gint        y;
  gint        static_x;
  gint        static_y;
  gboolean    started_the_first_time;
  gboolean    tray_is_static;
  gboolean    disable_indicator;
  gboolean    hide_indicator;
  gboolean    hide_tray_is_active;
  gboolean    hide_menu_is_active;
  GtkImage    *image;
  GtkMenu     *menu;
  gchar       *accessible_desc;

  GSettings   *settings;
};

#define INDICATOR_SYSTEMTRAY_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), INDICATOR_SYSTEMTRAY_TYPE, IndicatorSystemtrayPrivate))

#define SYSTEMTRAY_SCHEMA                "net.launchpad.indicator.systemtray"
#define SYSTEMTRAY_KEY_DISABLE_INDICATOR "disable-indicator"
#define SYSTEMTRAY_KEY_IS_FIRST_TIME     "started-the-first-time"
#define SYSTEMTRAY_KEY_TRAY_IS_STATIC    "tray-is-static"
#define SYSTEMTRAY_KEY_STATIC_X          "static-x"
#define SYSTEMTRAY_KEY_STATIC_Y          "static-y"

#define SYSTEMTRAY_ICON_WIDTH            23
#define SYSTEMTRAY_TRAY_TOP              24

#define INDICATOR_ICON_SYSTEMTRAY   "indicator-systemtray-unity"

GType indicator_systemtray_get_type(void);

/* Indicator Class Functions */
static void indicator_systemtray_class_init(IndicatorSystemtrayClass *klass);
static void indicator_systemtray_init(IndicatorSystemtray *self);
static void indicator_systemtray_dispose(GObject *object);
static void indicator_systemtray_finalize(GObject *object);

/* Indicator Standard Methods */
static GtkImage    *get_image(IndicatorObject *io);
static GtkMenu     *get_menu(IndicatorObject *io);
static const gchar *get_accessible_desc(IndicatorObject *io);
static void indicator_position_scroll (IndicatorObject * io, IndicatorObjectEntry * entry, gint delta, IndicatorScrollDirection direction);
static void indicator_systemtray_middle_click(IndicatorObject *io, IndicatorObjectEntry *entry, guint time, gpointer user_data);

/* Utility Functions */
static void update_position_tray(gpointer user_data);
static void update_indicator_visibility(IndicatorSystemtray *self);
static void mouse_get_position(gint *x, gint *y);
/* Callbacks */
static void menu_visible_notify_cb(GtkWidget *menu, GParamSpec *pspec, gpointer user_data);
static gboolean on_window_expose (GtkWidget *window, cairo_t *cr, gpointer user_data);
static gint width_of_tray(gpointer user_data);
static gboolean filter_tray_cb(NaTray* tray, NaTrayChild* icon, gpointer user_data);
static void on_tray_icon_removed(NaTrayManager* manager, NaTrayChild* removed, gpointer user_data);
static void setting_changed_cb(GSettings *settings, gchar *key, gpointer user_data);
static void set_position_menu(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data);
static gboolean show_tray(gpointer user_data);
static gboolean hide_tray(gpointer user_data);
static gboolean hide_menu(gpointer user_data);
static void setting_changed_cb(GSettings *settings, gchar *key, gpointer user_data);

/* Indicator Module Config */
INDICATOR_SET_VERSION
INDICATOR_SET_TYPE(INDICATOR_SYSTEMTRAY_TYPE)

G_DEFINE_TYPE (IndicatorSystemtray, indicator_systemtray, INDICATOR_OBJECT_TYPE);

static void indicator_systemtray_class_init(IndicatorSystemtrayClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private(klass, sizeof(IndicatorSystemtrayPrivate));

  object_class->dispose = indicator_systemtray_dispose;
  object_class->finalize = indicator_systemtray_finalize;

  IndicatorObjectClass *io_class = INDICATOR_OBJECT_CLASS(klass);

  io_class->get_image = get_image;
  io_class->get_menu = get_menu;
  io_class->get_accessible_desc = get_accessible_desc;
  io_class->entry_scrolled = indicator_position_scroll;
  io_class->secondary_activate = indicator_systemtray_middle_click;

  return;
}

static void indicator_systemtray_init(IndicatorSystemtray *self) {
  self->priv = INDICATOR_SYSTEMTRAY_GET_PRIVATE(self);

  self->priv->menu = NULL;
  self->priv->image = NULL;
  self->priv->count_tray_icon = 0;
  self->priv->accessible_desc = "System Tray";
  self->priv->hide_indicator = TRUE;
  update_indicator_visibility( self );

  /* Connect to GSettings */
  self->priv->settings = g_settings_new( SYSTEMTRAY_SCHEMA );
  self->priv->disable_indicator = g_settings_get_boolean( self->priv->settings, SYSTEMTRAY_KEY_DISABLE_INDICATOR );
  g_signal_connect( self->priv->settings, "changed", G_CALLBACK(setting_changed_cb), self );

  if (g_strcmp0(g_getenv("DESKTOP_SESSION"),"ubuntu") != 0 || self->priv->disable_indicator) {
    self->priv->disable_indicator = TRUE;
  }
  else {
    self->priv->started_the_first_time = g_settings_get_boolean( self->priv->settings, SYSTEMTRAY_KEY_IS_FIRST_TIME );
    self->priv->tray_is_static = g_settings_get_boolean( self->priv->settings, SYSTEMTRAY_KEY_TRAY_IS_STATIC );

    self->priv->static_x = g_settings_get_int( self->priv->settings, SYSTEMTRAY_KEY_STATIC_X );
    self->priv->static_y = g_settings_get_int( self->priv->settings, SYSTEMTRAY_KEY_STATIC_Y );

    if (self->priv->started_the_first_time || (self->priv->static_x <= 0 && self->priv->static_y <= 0)) {
      GdkScreen* screen = gdk_screen_get_default();
      gint w_s = gdk_screen_get_width( screen );
      self->priv->static_x = w_s/2;
      self->priv->static_y = 0;
      g_settings_set_boolean (self->priv->settings, SYSTEMTRAY_KEY_IS_FIRST_TIME, FALSE);
      g_settings_set_int(self->priv->settings, SYSTEMTRAY_KEY_STATIC_X, self->priv->static_x);
      g_settings_set_int(self->priv->settings, SYSTEMTRAY_KEY_STATIC_Y, self->priv->static_y);
    }

    self->priv->hide_tray_is_active = FALSE;
    self->priv->hide_menu_is_active = FALSE;
    self->priv->menu = GTK_MENU( gtk_menu_new() );
    self->priv->window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_type_hint( GTK_WINDOW (self->priv->window), GDK_WINDOW_TYPE_HINT_DOCK );
    gtk_window_set_has_resize_grip( GTK_WINDOW (self->priv->window), FALSE );
    gtk_window_set_keep_above( GTK_WINDOW (self->priv->window), TRUE );
    gtk_window_set_skip_pager_hint( GTK_WINDOW (self->priv->window), TRUE );
    gtk_window_set_skip_taskbar_hint( GTK_WINDOW (self->priv->window), TRUE );
    gtk_window_set_gravity(GTK_WINDOW(self->priv->window), GDK_GRAVITY_NORTH_EAST);
    gtk_widget_set_name( self->priv->window, "UnityPanelApplet" );
    gtk_widget_set_visual( self->priv->window, gdk_screen_get_rgba_visual(gdk_screen_get_default()) );
    gtk_widget_realize( self->priv->window );
    gtk_widget_set_app_paintable( self->priv->window, TRUE );
    gtk_window_set_title( GTK_WINDOW (self->priv->window), "System Tray" );
    g_signal_connect( self->priv->window, "draw", G_CALLBACK(on_window_expose), self );
    self->priv->tray = na_tray_new_for_screen( gdk_screen_get_default(), GTK_ORIENTATION_HORIZONTAL,
							 (NaTrayFilterCallback)filter_tray_cb, self );
    g_signal_connect( na_tray_get_manager(self->priv->tray), "tray_icon_removed", G_CALLBACK(on_tray_icon_removed), self );
    gtk_container_add( GTK_CONTAINER(self->priv->window), GTK_WIDGET(self->priv->tray) );
    gtk_widget_show( GTK_WIDGET(self->priv->tray) );
    gtk_widget_show_all( self->priv->window );
    gtk_widget_hide( self->priv->window );
    g_signal_connect( self->priv->menu, "notify::visible", G_CALLBACK(menu_visible_notify_cb), self );
    gtk_window_resize( GTK_WINDOW(self->priv->window), 1, 24 );
  }
}

static void indicator_systemtray_dispose(GObject *object) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY(object);

  if(self->priv->image != NULL) {
    g_object_unref( G_OBJECT(self->priv->image) );
    self->priv->image = NULL;
  }

  if(self->priv->menu != NULL) {
    g_object_unref( G_OBJECT(self->priv->menu) );
    self->priv->menu = NULL;
  }

  if(self->priv->settings != NULL) {
    g_object_unref(G_OBJECT(self->priv->settings));
    self->priv->settings = NULL;
  }

  if(self->priv->tray != NULL) {
    g_object_unref(G_OBJECT(self->priv->tray));
    self->priv->tray = NULL;
  }

  if(self->priv->window != NULL) {
    g_object_unref(G_OBJECT(self->priv->window));
    self->priv->window = NULL;
  }

  G_OBJECT_CLASS( indicator_systemtray_parent_class )->dispose( object );
  return;
}

static void indicator_systemtray_finalize(GObject *object) {
  G_OBJECT_CLASS( indicator_systemtray_parent_class )->finalize( object );
  return;
}

static GtkImage *get_image(IndicatorObject *io) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( io );

  if(self->priv->image == NULL) {
    self->priv->image = GTK_IMAGE( gtk_image_new() );
    gtk_image_set_from_icon_name( self->priv->image, INDICATOR_ICON_SYSTEMTRAY, GTK_ICON_SIZE_MENU );
  }

  return self->priv->image;
}

static GtkMenu *get_menu(IndicatorObject *io) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( io );
  return GTK_MENU( self->priv->menu );
}

static const gchar *get_accessible_desc(IndicatorObject *io) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( io );
  return self->priv->accessible_desc;
}

static void indicator_position_scroll (IndicatorObject * io, IndicatorObjectEntry * entry, gint delta, IndicatorScrollDirection direction) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( io );
  if (!self->priv->tray_is_static) return;

  if (direction == INDICATOR_OBJECT_SCROLL_UP) {
    self->priv->static_x += 1;
  } else {
    self->priv->static_x -= 1;
  }
  update_position_tray( self );
}

static void indicator_systemtray_middle_click(IndicatorObject *io, IndicatorObjectEntry *entry, guint time, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( io );
  self->priv->tray_is_static = !self->priv->tray_is_static;
  g_settings_set_boolean( self->priv->settings, SYSTEMTRAY_KEY_TRAY_IS_STATIC, self->priv->tray_is_static );
}

/**
 * setting_changed_cb:
 * @settings: the GSettings object
 * @key: the GSettings key
 * @user_data: the indicator object
 *
 * Called when a GSettings key is changed.
 **/
static void setting_changed_cb(GSettings *settings, gchar *key, gpointer user_data) {
  g_return_if_fail(IS_INDICATOR_SYSTEMTRAY( user_data ));
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );

  if (g_strcmp0(key, SYSTEMTRAY_KEY_DISABLE_INDICATOR) == 0) {
    self->priv->disable_indicator = g_settings_get_boolean( settings, SYSTEMTRAY_KEY_DISABLE_INDICATOR );
    //TODO: update_indicator_activity( self );
  }
  else if (g_strcmp0(key, SYSTEMTRAY_KEY_IS_FIRST_TIME) == 0) {
    self->priv->started_the_first_time = g_settings_get_boolean(self->priv->settings, SYSTEMTRAY_KEY_IS_FIRST_TIME);
    //TODO: update_indicator_pisition( self );
  }
  else if (g_strcmp0(key, SYSTEMTRAY_KEY_TRAY_IS_STATIC) == 0) {
    self->priv->tray_is_static = g_settings_get_boolean(self->priv->settings, SYSTEMTRAY_KEY_TRAY_IS_STATIC);
    if (self->priv->tray_is_static) {
      gtk_widget_show( self->priv->window );
      gtk_window_move( GTK_WINDOW(self->priv->window), self->priv->static_x, self->priv->static_y );
    }
    else gtk_widget_hide( self->priv->window );
  }
  else if (g_strcmp0(key, SYSTEMTRAY_KEY_STATIC_X) == 0) {
    self->priv->static_x = g_settings_get_int(self->priv->settings, SYSTEMTRAY_KEY_STATIC_X);
    if (self->priv->tray_is_static)
      gtk_window_move( GTK_WINDOW(self->priv->window), self->priv->static_x, self->priv->static_y );
  }
  else if (g_strcmp0(key, SYSTEMTRAY_KEY_STATIC_Y) == 0) {
    self->priv->static_y = g_settings_get_int(self->priv->settings, SYSTEMTRAY_KEY_STATIC_Y);
    if (self->priv->tray_is_static)
      gtk_window_move( GTK_WINDOW(self->priv->window), self->priv->static_x, self->priv->static_y );
  }
}

static gboolean on_window_expose (GtkWidget *window, cairo_t *cr, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  GtkAllocation alloc;

  gtk_widget_get_allocation( window, &alloc );

  cairo_set_operator( cr, CAIRO_OPERATOR_CLEAR );
  cairo_paint( cr );

  cairo_set_operator( cr, CAIRO_OPERATOR_OVER );
  cairo_set_source_rgba( cr, 0.0f, 0.0f, 0.0f, 0.0f );
  cairo_rectangle( cr, 0, 0, alloc.width, alloc.height );
  cairo_fill( cr );

  gtk_container_propagate_draw( GTK_CONTAINER(window), gtk_bin_get_child(GTK_BIN(window)), cr );

  update_position_tray( self );

  if (self->priv->tray_is_static && self->priv->count_tray_icon == 0 && gtk_widget_is_visible(self->priv->window)) {
    gtk_widget_hide( self->priv->window );
  }

  return FALSE;
}

static gint width_of_tray(gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );

  return self->priv->count_tray_icon * SYSTEMTRAY_ICON_WIDTH;
}

static gboolean filter_tray_cb(NaTray* tray, NaTrayChild* icon, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  self->priv->count_tray_icon++;
  gtk_window_resize( GTK_WINDOW(self->priv->window), width_of_tray(self), 24 );

  if (self->priv->count_tray_icon > 0 && self->priv->hide_indicator) {
    self->priv->hide_indicator = FALSE;
    update_indicator_visibility( self );
  }
  if (self->priv->tray_is_static && !gtk_widget_is_visible(self->priv->window) && self->priv->count_tray_icon > 0) {
    gtk_widget_show( self->priv->window );
  }
  return TRUE;
}

static void on_tray_icon_removed(NaTrayManager* manager, NaTrayChild* removed, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  self->priv->count_tray_icon--;
  gtk_window_resize( GTK_WINDOW(self->priv->window), width_of_tray(self), 24 );
  if (self->priv->count_tray_icon == 0) {
    self->priv->hide_indicator = TRUE;
    update_indicator_visibility( self );
  }
}

static void update_position_tray(gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  GdkScreen* screen = gdk_screen_get_default();
  gint w_s = gdk_screen_get_width( screen );
  if (self->priv->tray_is_static) {
    if (self->priv->static_x > w_s)
      self->priv->static_x = w_s;
    if (self->priv->static_x <= 0)
      self->priv->static_x = 1;
    g_settings_set_int(self->priv->settings, SYSTEMTRAY_KEY_STATIC_X, self->priv->static_x);
    g_settings_set_int(self->priv->settings, SYSTEMTRAY_KEY_STATIC_Y, self->priv->static_y);
  }
  else {
    self->priv->x = self->priv->x + (width_of_tray(self)/2) + 18;
    self->priv->y = SYSTEMTRAY_TRAY_TOP;
    if (self->priv->x > w_s)
      self->priv->x = w_s;  
  }
}

static void update_indicator_visibility(IndicatorSystemtray *self) {
  g_return_if_fail( IS_INDICATOR_SYSTEMTRAY(self) );
  
  if (self->priv->image != NULL) {
    if (self->priv->hide_indicator)
      gtk_widget_hide( GTK_WIDGET(self->priv->image) );
    else
      gtk_widget_show( GTK_WIDGET(self->priv->image) );
  }
}

static void mouse_get_position(gint *x, gint *y) {
  GdkDisplay *display = gdk_display_get_default();
  GdkDeviceManager *device_manager = gdk_display_get_device_manager( display );
  GdkDevice *device = gdk_device_manager_get_client_pointer( device_manager );
  gdk_device_get_position( device, NULL, x, y ); 
}

static gboolean show_tray(gpointer user_data) {
    IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY(user_data);
    gint x = 0;
    gint y = 0;
    mouse_get_position( &x, &y );
    gboolean visible;
    g_object_get( G_OBJECT(self->priv->menu), "visible", &visible, NULL );
    if (visible) {
      gtk_menu_reposition( GTK_MENU(self->priv->menu) );
      GtkWidget *top_widget = gtk_widget_get_toplevel( GTK_WIDGET(self->priv->menu) );
      GtkWindow *top_win = GTK_WINDOW( top_widget );
      gtk_window_get_position( top_win, &self->priv->x, &self->priv->y );
      update_position_tray( self );
      gtk_menu_popup( GTK_MENU(self->priv->menu), NULL, NULL, (GtkMenuPositionFunc)set_position_menu, self, 0, gtk_get_current_event_time() );
      gtk_widget_show( self->priv->window );
      gtk_window_move( GTK_WINDOW(self->priv->window), self->priv->x, self->priv->y/*+1*/ );
      if (y > 24) return FALSE;
      if (!self->priv->hide_menu_is_active) {
        self->priv->hide_menu_is_active = TRUE;
        g_timeout_add( 100, hide_menu, self );
      }
      if (!self->priv->hide_tray_is_active) {
        self->priv->hide_tray_is_active = TRUE;
        g_timeout_add( 300, hide_tray, self );
      }
    } 
    else {
      if (y <= 24 || y > 70)
        gtk_widget_hide( self->priv->window );
    }

  return FALSE;
}

static gboolean hide_menu(gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY(user_data);
  if (!gtk_widget_is_visible( GTK_WIDGET(self->priv->menu) ) || self->priv->tray_is_static) {
    self->priv->hide_menu_is_active = FALSE;
    return FALSE;
  }
  gint x = 0;
  gint y = 0;
  mouse_get_position( &x, &y );
  if (y > 24) {
    gtk_widget_hide( GTK_WIDGET(self->priv->menu) );
    self->priv->hide_menu_is_active = FALSE;
    return FALSE;
  }
   else return TRUE;
}

static gboolean hide_tray(gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  if (!gtk_widget_is_visible( self->priv->window ) || self->priv->tray_is_static) {
    self->priv->hide_tray_is_active = FALSE;
    return FALSE;
  }
  gint x = 0;
  gint y = 0;
  mouse_get_position( &x, &y );
  if(y > 70) {
    gtk_widget_hide( self->priv->window );
    self->priv->hide_tray_is_active = FALSE;
    return FALSE;
  }
   else return TRUE;
}

static void set_position_menu(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data) {
  //g_return_if_fail(IS_INDICATOR_SYSTEMTRAY(user_data));
  //IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY(user_data);
  GdkScreen* screen = gdk_screen_get_default();
  *x = gdk_screen_get_width( screen );//self->priv->x;
  *y = gdk_screen_get_height( screen );//self->priv->y;
  *push_in = FALSE;
}

static void menu_visible_notify_cb(GtkWidget *menu, G_GNUC_UNUSED GParamSpec *pspec, gpointer user_data) {
  g_return_if_fail( GTK_IS_MENU(menu) );
  g_return_if_fail( IS_INDICATOR_SYSTEMTRAY(user_data) );
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  
  if (!self->priv->tray_is_static) {
    g_timeout_add( 10, show_tray, self );
  }
}
