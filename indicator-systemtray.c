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

#include "config.h"
/* Indicator Stuff */
#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
//#include <libindicator/indicator-service-manager.h>

//#include "na-tray-manager.h"
//#include "na-tray-child.h"
#include "na-tray.h"
#include <glib/gi18n.h>

#define GETTEXT_PACKAGE "indicator-systemtray-unity"
#define LOCALEDIR "/usr/share/locale"

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
  GtkWidget   *settings_item;
  GtkWidget   *static_show_item;
  GtkWidget   *floating_show_item;
  gchar       *accessible_desc;
  GtkWidget   *fixed;
  gboolean    show_background_static;
  gchar       *bg_static;
  gchar       *bg_static_stroke;
  gboolean    show_background_floating;
  gchar       *bg_floating;
  gchar       *bg_floating_stroke;
  GSettings   *settings;
};

#define INDICATOR_SYSTEMTRAY_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), INDICATOR_SYSTEMTRAY_TYPE, IndicatorSystemtrayPrivate))

#define SYSTEMTRAY_SCHEMA                       "net.launchpad.indicator.systemtray"
#define SYSTEMTRAY_KEY_DISABLE_INDICATOR        "disable-indicator"
#define SYSTEMTRAY_KEY_IS_FIRST_TIME            "started-the-first-time"
#define SYSTEMTRAY_KEY_TRAY_IS_STATIC           "tray-is-static"
#define SYSTEMTRAY_KEY_STATIC_X                 "static-x"
#define SYSTEMTRAY_KEY_STATIC_Y                 "static-y"

#define SYSTEMTRAY_KEY_STATIC_SHOW_BG           "show-background-static"
#define SYSTEMTRAY_KEY_STATIC_BG                "rgba-static"
#define SYSTEMTRAY_KEY_STATIC_BG_STROKE         "rgba-static-stroke"

#define SYSTEMTRAY_KEY_FLOATING_SHOW_BG         "show-background-floating"
#define SYSTEMTRAY_KEY_FLOATING_BG              "rgba-floating"
#define SYSTEMTRAY_KEY_FLOATING_BG_STROKE       "rgba-floating-stroke"

#define SYSTEMTRAY_ICON_WIDTH                   23
#define SYSTEMTRAY_TRAY_TOP                     24
#define SYSTEMTRAY_TRAY_HEIGHT                  24

#define INDICATOR_ICON_SYSTEMTRAY               "indicator-systemtray-unity"

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
static gboolean parse_rgb_value(const gchar *str, gchar **endp, gdouble *number);
static gboolean rgba_parse(GdkRGBA *rgba, const gchar *spec);
static gchar *rgba_to_string(const GdkRGBA *rgba, gdouble format);
static gboolean color_chooser(GdkRGBA *color, const gchar *title);

/* Callbacks */
static void menu_visible_notify_cb(GtkWidget *menu, GParamSpec *pspec, gpointer user_data);
static gboolean on_window_expose (GtkWidget *window, cairo_t *cr, gpointer user_data);
static gint width_of_tray(gpointer user_data);
static gboolean filter_tray_cb(NaTray* tray, NaTrayChild* icon, gpointer user_data);
static void on_tray_icon_removed(NaTrayManager* manager, NaTrayChild* removed, gpointer user_data);
static void static_color_bg_item_activate(GtkCheckMenuItem *menu_item, gpointer user_data);
static void static_color_bg_stroke_item_activate(GtkCheckMenuItem *menu_item, gpointer user_data);
static void static_color_bg_reset_item_activate(GtkCheckMenuItem *menu_item, gpointer user_data);
static void floating_color_bg_item_activate(GtkCheckMenuItem *menu_item, gpointer user_data);
static void floating_color_bg_stroke_item_activate(GtkCheckMenuItem *menu_item, gpointer user_data);
static void floating_color_bg_reset_item_activate(GtkCheckMenuItem *menu_item, gpointer user_data);
static void static_show_item_toggled(GtkCheckMenuItem *menu_item, gpointer user_data);
static void floating_show_item_toggled(GtkCheckMenuItem *menu_item, gpointer user_data);
static void setting_changed_cb(GSettings *settings, gchar *key, gpointer user_data);
static void set_position_menu(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data);
static gboolean show_tray(gpointer user_data);
static gboolean hide_tray(gpointer user_data);
static gboolean hide_menu(gpointer user_data);

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

  bindtextdomain( GETTEXT_PACKAGE, LOCALEDIR );
  textdomain( GETTEXT_PACKAGE );

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

    self->priv->show_background_static = g_settings_get_boolean(self->priv->settings, SYSTEMTRAY_KEY_STATIC_SHOW_BG);
    self->priv->bg_static = g_settings_get_string(self->priv->settings, SYSTEMTRAY_KEY_STATIC_BG);
    self->priv->bg_static_stroke = g_settings_get_string(self->priv->settings, SYSTEMTRAY_KEY_STATIC_BG_STROKE);
    self->priv->show_background_floating = g_settings_get_boolean(self->priv->settings, SYSTEMTRAY_KEY_FLOATING_SHOW_BG);
    self->priv->bg_floating = g_settings_get_string(self->priv->settings, SYSTEMTRAY_KEY_FLOATING_BG);
    self->priv->bg_floating_stroke = g_settings_get_string(self->priv->settings, SYSTEMTRAY_KEY_FLOATING_BG_STROKE);

    self->priv->hide_tray_is_active = FALSE;
    self->priv->hide_menu_is_active = FALSE;

    self->priv->menu = GTK_MENU( gtk_menu_new() );
    gtk_widget_set_size_request( GTK_WIDGET(self->priv->menu), 0, 0 );
    self->priv->settings_item = gtk_menu_item_new_with_label( _("Settings") );
    gtk_menu_shell_append( GTK_MENU_SHELL(self->priv->menu), self->priv->settings_item );
    GtkWidget *settings_sub = gtk_menu_new();
    gtk_menu_item_set_submenu( GTK_MENU_ITEM(self->priv->settings_item), settings_sub );
//---
     GtkWidget *background_item = gtk_menu_item_new_with_label( _("Background") );
     gtk_menu_shell_append( GTK_MENU_SHELL(settings_sub), background_item );
     GtkWidget *background_sub = gtk_menu_new();
     gtk_menu_item_set_submenu( GTK_MENU_ITEM(background_item), background_sub );
//----
      GtkWidget *static_item = gtk_menu_item_new_with_label( _("Static") );
      gtk_menu_shell_append( GTK_MENU_SHELL(background_sub), static_item );
      GtkWidget *static_sub = gtk_menu_new();
      gtk_menu_item_set_submenu( GTK_MENU_ITEM(static_item), static_sub );
//-----
       GtkWidget *static_color_bg_item = gtk_menu_item_new_with_label( _("Color") );
       gtk_menu_shell_append( GTK_MENU_SHELL(static_sub), static_color_bg_item );
       g_signal_connect( G_OBJECT(static_color_bg_item), "activate", G_CALLBACK(static_color_bg_item_activate), self );
       GtkWidget *static_color_bg_stroke_item = gtk_menu_item_new_with_label( _("Stroke") );
       gtk_menu_shell_append( GTK_MENU_SHELL(static_sub), static_color_bg_stroke_item );
       g_signal_connect( G_OBJECT(static_color_bg_stroke_item), "activate", G_CALLBACK(static_color_bg_stroke_item_activate), self );
       self->priv->static_show_item = gtk_check_menu_item_new_with_label( _("Show") );
       gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(self->priv->static_show_item), self->priv->show_background_static );
       gtk_menu_shell_append( GTK_MENU_SHELL(static_sub), self->priv->static_show_item );
       g_signal_connect( G_OBJECT(self->priv->static_show_item), "toggled", G_CALLBACK(static_show_item_toggled), self );
       GtkWidget *static_color_bg_reset_item = gtk_menu_item_new_with_label( _("Reset") );
       gtk_menu_shell_append( GTK_MENU_SHELL(static_sub), static_color_bg_reset_item );
       g_signal_connect( G_OBJECT(static_color_bg_reset_item), "activate", G_CALLBACK(static_color_bg_reset_item_activate), self );
//----
      GtkWidget *floating_item = gtk_menu_item_new_with_label( _("Floating") );
      gtk_menu_shell_append( GTK_MENU_SHELL(background_sub), floating_item );
      GtkWidget *floating_sub = gtk_menu_new();
      gtk_menu_item_set_submenu( GTK_MENU_ITEM(floating_item), floating_sub );
//-----
       GtkWidget *floating_color_bg_item = gtk_menu_item_new_with_label( _("Color") );
       gtk_menu_shell_append( GTK_MENU_SHELL(floating_sub), floating_color_bg_item );
       g_signal_connect( G_OBJECT(floating_color_bg_item), "activate", G_CALLBACK(floating_color_bg_item_activate), self );
       GtkWidget *floating_color_bg_stroke_item = gtk_menu_item_new_with_label( _("Stroke") );
       gtk_menu_shell_append( GTK_MENU_SHELL(floating_sub), floating_color_bg_stroke_item );
       g_signal_connect( G_OBJECT(floating_color_bg_stroke_item), "activate", G_CALLBACK(floating_color_bg_stroke_item_activate), self );
       self->priv->floating_show_item = gtk_check_menu_item_new_with_label( _("Show") );
       gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(self->priv->floating_show_item), self->priv->show_background_floating );
       gtk_menu_shell_append( GTK_MENU_SHELL(floating_sub), self->priv->floating_show_item );
       g_signal_connect( G_OBJECT(self->priv->floating_show_item), "toggled", G_CALLBACK(floating_show_item_toggled), self );
       GtkWidget *floating_color_bg_reset_item = gtk_menu_item_new_with_label( _("Reset") );
       gtk_menu_shell_append( GTK_MENU_SHELL(floating_sub), floating_color_bg_reset_item );
       g_signal_connect( G_OBJECT(floating_color_bg_reset_item), "activate", G_CALLBACK(floating_color_bg_reset_item_activate), self );

    self->priv->window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_type_hint( GTK_WINDOW (self->priv->window), GDK_WINDOW_TYPE_HINT_DOCK );
    gtk_window_set_has_resize_grip( GTK_WINDOW (self->priv->window), FALSE );
    gtk_window_set_keep_above( GTK_WINDOW (self->priv->window), TRUE );
    gtk_window_set_skip_pager_hint( GTK_WINDOW (self->priv->window), TRUE );
    gtk_window_set_skip_taskbar_hint( GTK_WINDOW (self->priv->window), TRUE );
    gtk_window_set_gravity( GTK_WINDOW(self->priv->window), GDK_GRAVITY_NORTH_EAST );
    gtk_widget_set_name( self->priv->window, "UnityPanelApplet" );
    gtk_widget_set_visual( self->priv->window, gdk_screen_get_rgba_visual(gdk_screen_get_default()) );
    gtk_widget_realize( self->priv->window );
    gtk_widget_set_app_paintable( self->priv->window, TRUE );
    gtk_window_set_title( GTK_WINDOW (self->priv->window), "System Tray" );
    g_signal_connect( self->priv->window, "draw", G_CALLBACK(on_window_expose), self );
    self->priv->tray = na_tray_new_for_screen( gdk_screen_get_default(), GTK_ORIENTATION_HORIZONTAL,
							 (NaTrayFilterCallback)filter_tray_cb, self );
    g_signal_connect( na_tray_get_manager(self->priv->tray), "tray_icon_removed", G_CALLBACK(on_tray_icon_removed), self );
    GtkWidget *fixed;
    fixed = gtk_fixed_new ();
    self->priv->fixed = fixed;
    gtk_container_add (GTK_CONTAINER (self->priv->window), self->priv->fixed);
    gtk_widget_show (self->priv->fixed);
    gtk_fixed_put (GTK_FIXED (self->priv->fixed), GTK_WIDGET(self->priv->tray), LEFT_BOARD, 0);

    gtk_widget_show( GTK_WIDGET(self->priv->tray) );
    gtk_widget_show_all( self->priv->window );
    gtk_widget_show_all( GTK_WIDGET(self->priv->menu) );
    if (self->priv->tray_is_static) {
      gtk_widget_show( self->priv->settings_item );
    }
    else {
      gtk_widget_hide( self->priv->settings_item );
    }
    gtk_widget_hide( self->priv->window );
    g_signal_connect( self->priv->menu, "notify::visible", G_CALLBACK(menu_visible_notify_cb), self );
    gtk_widget_set_size_request( GTK_WIDGET(self->priv->tray), 1, SYSTEMTRAY_TRAY_HEIGHT );
    gtk_window_resize( GTK_WINDOW(self->priv->window), 1, SYSTEMTRAY_TRAY_HEIGHT );
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
  return self->priv->menu;
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

static gboolean color_chooser(GdkRGBA *color, const gchar *title) {
  GtkWidget *colorseldlg;
  gint response_id;
  gboolean response_check = FALSE;
  colorseldlg = gtk_color_chooser_dialog_new( title, NULL );
  gtk_color_chooser_set_rgba( GTK_COLOR_CHOOSER (colorseldlg), color );

  response_id = gtk_dialog_run( GTK_DIALOG(colorseldlg) );
  if(response_id == GTK_RESPONSE_OK) {
     gtk_color_chooser_get_rgba( GTK_COLOR_CHOOSER(colorseldlg), color );
     response_check = TRUE;
  }

  gtk_widget_destroy( colorseldlg );
  return response_check;
}

static void static_color_bg_item_activate(GtkCheckMenuItem *menu_item, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  GdkRGBA color;
  rgba_parse( &color, self->priv->bg_static );
  if(color_chooser(&color, _("Color"))) {
     g_settings_set_string( self->priv->settings, SYSTEMTRAY_KEY_STATIC_BG, rgba_to_string(&color, 255) );
  }
}

static void static_color_bg_stroke_item_activate(GtkCheckMenuItem *menu_item, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  GdkRGBA color;
  rgba_parse( &color, self->priv->bg_static_stroke );
  if(color_chooser(&color, _("Stroke"))) {
     g_settings_set_string( self->priv->settings, SYSTEMTRAY_KEY_STATIC_BG_STROKE, rgba_to_string(&color, 255) );
  }
}

static void static_color_bg_reset_item_activate(GtkCheckMenuItem *menu_item, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  g_settings_reset( self->priv->settings, SYSTEMTRAY_KEY_STATIC_BG );
  g_settings_reset( self->priv->settings, SYSTEMTRAY_KEY_STATIC_BG_STROKE );
  g_settings_reset( self->priv->settings, SYSTEMTRAY_KEY_STATIC_SHOW_BG );
}

static void floating_color_bg_item_activate(GtkCheckMenuItem *menu_item, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  GdkRGBA color;
  rgba_parse( &color, self->priv->bg_floating );
  if(color_chooser(&color, _("Color"))) {
     g_settings_set_string( self->priv->settings, SYSTEMTRAY_KEY_FLOATING_BG, rgba_to_string(&color, 255) );
  }
}

static void floating_color_bg_stroke_item_activate(GtkCheckMenuItem *menu_item, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  GdkRGBA color;
  rgba_parse( &color, self->priv->bg_floating_stroke );
  if(color_chooser(&color, _("Stroke"))) {
     g_settings_set_string( self->priv->settings, SYSTEMTRAY_KEY_FLOATING_BG_STROKE, rgba_to_string(&color, 255) );
  }
}

static void floating_color_bg_reset_item_activate(GtkCheckMenuItem *menu_item, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  g_settings_reset( self->priv->settings, SYSTEMTRAY_KEY_FLOATING_BG );
  g_settings_reset( self->priv->settings, SYSTEMTRAY_KEY_FLOATING_BG_STROKE );
  g_settings_reset( self->priv->settings, SYSTEMTRAY_KEY_FLOATING_SHOW_BG );
}

static void static_show_item_toggled(GtkCheckMenuItem *menu_item, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  g_settings_set_boolean( self->priv->settings, SYSTEMTRAY_KEY_STATIC_SHOW_BG, gtk_check_menu_item_get_active( menu_item ) );
}

static void floating_show_item_toggled(GtkCheckMenuItem *menu_item, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  g_settings_set_boolean( self->priv->settings, SYSTEMTRAY_KEY_FLOATING_SHOW_BG, gtk_check_menu_item_get_active( menu_item ) );
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
      gtk_widget_show( self->priv->settings_item );
    }
    else {
      gtk_widget_hide( self->priv->window );
      gtk_widget_hide( self->priv->settings_item );
    }
    gtk_widget_queue_draw( self->priv->window );
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
  else if (g_strcmp0(key, SYSTEMTRAY_KEY_STATIC_SHOW_BG) == 0) {
    self->priv->show_background_static = g_settings_get_boolean(self->priv->settings, SYSTEMTRAY_KEY_STATIC_SHOW_BG);
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self->priv->static_show_item)) != self->priv->show_background_static)
      gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(self->priv->static_show_item), self->priv->show_background_static );
    gtk_widget_queue_draw( self->priv->window );
  }
  else if (g_strcmp0(key, SYSTEMTRAY_KEY_STATIC_BG) == 0) {
    self->priv->bg_static = g_settings_get_string(self->priv->settings, SYSTEMTRAY_KEY_STATIC_BG);
    gtk_widget_queue_draw( self->priv->window );
  }
  else if (g_strcmp0(key, SYSTEMTRAY_KEY_STATIC_BG_STROKE) == 0) {
    self->priv->bg_static_stroke = g_settings_get_string(self->priv->settings, SYSTEMTRAY_KEY_STATIC_BG_STROKE);
    gtk_widget_queue_draw( self->priv->window );
  }
  else if (g_strcmp0(key, SYSTEMTRAY_KEY_FLOATING_SHOW_BG) == 0) {
    self->priv->show_background_floating = g_settings_get_boolean(self->priv->settings, SYSTEMTRAY_KEY_FLOATING_SHOW_BG);
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self->priv->floating_show_item)) != self->priv->show_background_floating)
      gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(self->priv->floating_show_item), self->priv->show_background_floating );
    gtk_widget_queue_draw( self->priv->window );
  }
  else if (g_strcmp0(key, SYSTEMTRAY_KEY_FLOATING_BG) == 0) {
    self->priv->bg_floating = g_settings_get_string(self->priv->settings, SYSTEMTRAY_KEY_FLOATING_BG);
    gtk_widget_queue_draw( self->priv->window );
  }
  else if (g_strcmp0(key, SYSTEMTRAY_KEY_FLOATING_BG_STROKE) == 0) {
    self->priv->bg_floating_stroke = g_settings_get_string(self->priv->settings, SYSTEMTRAY_KEY_FLOATING_BG_STROKE);
  if (gtk_widget_is_visible(self->priv->window))
    gtk_widget_queue_draw( self->priv->window );
  }
}

#define SKIP_WHITESPACES(s) while (*(s) == ' ') (s)++;

static gboolean parse_rgb_value (const gchar  *str,  gchar **endp, gdouble *number)
{
  const char *p;

  *number = g_ascii_strtod (str, endp);

  if (*endp == str)
    return FALSE;

  p = *endp;

  SKIP_WHITESPACES (p);

  if (*p == '%')
    {
      *endp = (char *)(p + 1);
      *number = CLAMP(*number / 100., 0., 1.);
    }
  else
    {
      *number = CLAMP(*number / 255., 0., 1.);
    }

  return TRUE;
}

static gboolean rgba_parse (GdkRGBA *rgba, const gchar *spec)
{
  gboolean has_alpha;
  gdouble r, g, b, a;
  gchar *str = (gchar *) spec;

  g_return_val_if_fail (spec != NULL, FALSE);


  if(g_ascii_strncasecmp (str, "rgba", 4) == 0)
    {
      has_alpha = TRUE;
      str += 4;
    }
  else if (g_ascii_strncasecmp (str, "rgb", 3) == 0)
    {
      has_alpha = FALSE;
      a = 1;
      str += 3;
    }
  else
    {
      PangoColor pango_color;

      /* Resort on PangoColor for rgb.txt color
       * map and '#' prefixed colors
       */
      if (pango_color_parse (&pango_color, str))
        {
          if (rgba)
            {
              rgba->red = pango_color.red / 65535.;
              rgba->green = pango_color.green / 65535.;
              rgba->blue = pango_color.blue / 65535.;
              rgba->alpha = 1;
            }

          return TRUE;
        }
      else
        return FALSE;
    }

  SKIP_WHITESPACES (str);

  if (*str != '(')
    return FALSE;

  str++;

  /* Parse red */
  SKIP_WHITESPACES (str);
  if (!parse_rgb_value (str, &str, &r))
    return FALSE;
  SKIP_WHITESPACES (str);

  if (*str != ',')
    return FALSE;

  str++;

  /* Parse green */
  SKIP_WHITESPACES (str);
  if (!parse_rgb_value (str, &str, &g))
    return FALSE;
  SKIP_WHITESPACES (str);

  if (*str != ',')
    return FALSE;

  str++;

  /* Parse blue */
  SKIP_WHITESPACES (str);
  if (!parse_rgb_value (str, &str, &b))
    return FALSE;
  SKIP_WHITESPACES (str);

  if (has_alpha)
    {
      if (*str != ',')
        return FALSE;

      str++;

      SKIP_WHITESPACES (str);
  if (!parse_rgb_value (str, &str, &a))
    return FALSE;
      SKIP_WHITESPACES (str);
    }

  if (*str != ')')
    return FALSE;

  str++;

  SKIP_WHITESPACES (str);

  if (*str != '\0')
    return FALSE;

  if (rgba)
    {
      rgba->red = CLAMP (r, 0, 1);
      rgba->green = CLAMP (g, 0, 1);
      rgba->blue = CLAMP (b, 0, 1);
      rgba->alpha = CLAMP (a, 0, 1);
    }

  return TRUE;
}

#undef SKIP_WHITESPACES

gchar * rgba_to_string (const GdkRGBA *rgba, gdouble format) {
  return g_strdup_printf ("rgba(%d,%d,%d,%d)",
                           (int)(rgba->red * format),
                           (int)(rgba->green * format),
                           (int)(rgba->blue * format),
                           (int)(rgba->alpha * format));
}

static gboolean on_window_expose (GtkWidget *window, cairo_t *cr, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );
  GdkRGBA bg;
  GdkRGBA bg_stroke;
  gboolean check_bground = FALSE;
  gboolean check_bground_stroke = FALSE;
  GtkAllocation alloc;
  
  gtk_widget_get_allocation( window, &alloc );
  
  cairo_set_operator( cr, CAIRO_OPERATOR_CLEAR );
  cairo_paint( cr );

 cairo_set_operator( cr, CAIRO_OPERATOR_OVER );
  if(self->priv->tray_is_static) {
    if(self->priv->show_background_static) {
       check_bground = rgba_parse( &bg, self->priv->bg_static );
       check_bground_stroke = rgba_parse( &bg_stroke, self->priv->bg_static_stroke );
    }
    else {
       check_bground = rgba_parse( &bg, "rgba(0,0,0,0)" );
       check_bground_stroke = rgba_parse( &bg_stroke, "rgba(0,0,0,0)" );
    }
  }
  else {
    if(self->priv->show_background_floating) {
       check_bground = rgba_parse( &bg, self->priv->bg_floating );
       check_bground_stroke = rgba_parse( &bg_stroke, self->priv->bg_floating_stroke );
    }
    else {
       check_bground = rgba_parse( &bg, "rgba(0,0,0,0)" );
       check_bground_stroke = rgba_parse( &bg_stroke, "rgba(0,0,0,0)" );
    }
  }

  if(check_bground) {
     cairo_set_source_rgba( cr, bg.red, bg.green, bg.blue, bg.alpha );
     cairo_rectangle( cr, 0, 0, alloc.width, alloc.height );
     cairo_fill( cr );
  }
  if(check_bground_stroke) {
     cairo_set_line_width( cr, 0.3 );
     cairo_set_source_rgba( cr, bg_stroke.red, bg_stroke.green, bg_stroke.blue, bg_stroke.alpha );
     cairo_rectangle( cr, 0.25, 0.25, alloc.width-0.65, alloc.height-0.45 );
     cairo_stroke( cr );
  }
  gtk_container_propagate_draw( GTK_CONTAINER(window), gtk_bin_get_child(GTK_BIN(window)), cr );
  
  update_position_tray( self );

  if (self->priv->tray_is_static && self->priv->count_tray_icon == 0 && gtk_widget_is_visible(self->priv->window)) {
    gtk_widget_hide( self->priv->window );
  }

  return FALSE;
}

static gint width_of_tray(gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );

  return self->priv->count_tray_icon * SYSTEMTRAY_ICON_WIDTH + 4;
}

static gboolean filter_tray_cb(NaTray* tray, NaTrayChild* icon, gpointer user_data) {
  IndicatorSystemtray *self = INDICATOR_SYSTEMTRAY( user_data );

  if (na_tray_child_has_alpha(icon))
    na_tray_child_set_composited(icon, TRUE);

  self->priv->count_tray_icon++;
  gtk_widget_set_size_request( GTK_WIDGET(self->priv->tray), width_of_tray(self), SYSTEMTRAY_TRAY_HEIGHT );
  gtk_window_resize( GTK_WINDOW(self->priv->window), width_of_tray(self), SYSTEMTRAY_TRAY_HEIGHT );
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
  gtk_widget_set_size_request( GTK_WIDGET(self->priv->tray), width_of_tray(self), SYSTEMTRAY_TRAY_HEIGHT );
  gtk_window_resize( GTK_WINDOW(self->priv->window), width_of_tray(self), SYSTEMTRAY_TRAY_HEIGHT );
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
    self->priv->x = self->priv->x + (width_of_tray(self)/2) + 19;
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
      gtk_menu_reposition( self->priv->menu );
      GtkWidget *top_widget = gtk_widget_get_toplevel( GTK_WIDGET(self->priv->menu) );
      GtkWindow *top_win = GTK_WINDOW( top_widget );
      gtk_window_get_position( top_win, &self->priv->x, &self->priv->y );
      update_position_tray( self );
      gtk_menu_popup( self->priv->menu, NULL, NULL, (GtkMenuPositionFunc)set_position_menu, self, 0, gtk_get_current_event_time() );
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
