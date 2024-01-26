/*
 * geany_base64_converter.c
 *
 * Geany plugin converting base64 strings to human-readable
 * text and viceversa.
 *
 * Copyright 2024 zhgzhg @ github.com
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <sys/stat.h>

#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h> /* for the key bindings */

#include <b64/cencode.h>
#include <b64/cdecode.h>

#ifdef HAVE_LOCALE_H
	#include <locale.h>
#endif

GeanyPlugin *geany_plugin;
struct GeanyKeyGroup *geany_key_group;
GeanyData *geany_data;

static gchar *plugin_config_path = NULL;
static GKeyFile *keyfile_plugin = NULL;

PLUGIN_VERSION_CHECK(235)

PLUGIN_SET_TRANSLATABLE_INFO(LOCALEDIR,
	GETTEXT_PACKAGE,
	_("Base64 Converter"),

	_("Base64 to readable string and viceversa converter.\n\
Converts the value in selection or in the clipboard to a readable \
string.\nhttps://github.com/zhgzhg/Geany-Base64-Converter"),

	"1.0.0",

	"zhgzhg @@ github.com\n\
https://github.com/zhgzhg/Geany-Base64-Converter"
);

static GtkWidget *main_menu_item = NULL;
static GtkWidget *main_menu_item2 = NULL;
static GtkWidget *log_formatting_success_messages_btn = NULL;
static GtkWidget *show_errors_in_window_btn = NULL;

static gboolean logFormattingSuccessMessages = FALSE;
static gboolean showErrorsInMsgPopupWindow = FALSE;


static void base64_convert(GeanyDocument *doc, gboolean toBase64)
{
	base64_encodestate state_encode;
	base64_decodestate state_decode;
	const gchar *noDataMsg = "No valid data to convert!";
	const gchar *memAllocFailMsg = "Couldn't prepare the coversion!";
	const gchar *successfullyEncoded = "Successfully encoded to base64";
	const gchar *successfullyDecoded = "Successfully decoded from base64";
	gboolean workWithTextSelection = FALSE;
	gchar *input_string = NULL, *result_string = NULL;
	gint input_len = 0, result_len = 0;
	gint bytes_written = 0;
	gint cursPos = 0, colPos = 0;

	/* use the text in the clipboard */

	if (doc != NULL)
	{
		/* first try to work only with a text selection (if any) */
		if (sci_has_selection(doc->editor->sci))
		{
			input_string = sci_get_selection_contents(doc->editor->sci);
			if (input_string != NULL)
			{
				workWithTextSelection = TRUE;
				input_len =
					sci_get_selected_text_length(doc->editor->sci) + 1;
			}
		}
		else
		{	/* Work with the entire file */
			input_len = sci_get_length(doc->editor->sci);
			if (input_len == 0) return;
			++input_len;
			input_string = sci_get_contents(doc->editor->sci, -1);
		}

		if (input_string == NULL) return;

		if (toBase64)
		{
			result_len = input_len * 2;
			base64_init_encodestate(&state_encode);
		}
		else
		{
			result_len = input_len + 1;
			base64_init_decodestate(&state_decode);
		}

		result_string = g_malloc(result_len);
		if (result_string == NULL)
		{
			msgwin_msg_add(COLOR_RED, -1, doc, "%s", memAllocFailMsg);
			if (showErrorsInMsgPopupWindow)
				dialogs_show_msgbox(GTK_MESSAGE_INFO, "%s",
									memAllocFailMsg);
			return;
		}

		if (toBase64)
		{
			bytes_written = base64_encode_block((char *) input_string,
				(int) input_len - 1, result_string, &state_encode);
			bytes_written += base64_encode_blockend(
				(char *)(result_string + bytes_written), &state_encode);
			*(result_string + bytes_written) = '\0';
			if (bytes_written > 0)
			{
				if (*(result_string + bytes_written - 1) == '\n')
				{ *(result_string + bytes_written - 1) = '\0'; }
				
				if (logFormattingSuccessMessages)
					msgwin_msg_add(COLOR_BLUE, -1, doc, "%s",
									successfullyEncoded);
			}
			else
			{
				msgwin_msg_add(COLOR_RED, -1, doc, "%s", noDataMsg);

				if (showErrorsInMsgPopupWindow)
					dialogs_show_msgbox(GTK_MESSAGE_INFO, "%s",
						noDataMsg);
			}
		}
		else
		{
			bytes_written = base64_decode_block((char *) input_string,
				(int) input_len, result_string, &state_decode);
			*(result_string + bytes_written) = '\0';
			if (bytes_written > 0)
			{
				if (logFormattingSuccessMessages)
					msgwin_msg_add(COLOR_BLUE, -1, doc, "%s",
									successfullyDecoded);
			}
			else
			{
				msgwin_msg_add(COLOR_RED, -1, doc, "%s", noDataMsg);
				if (showErrorsInMsgPopupWindow)
					dialogs_show_msgbox(GTK_MESSAGE_INFO, "%s",
										noDataMsg);
			}
		}

		if (!workWithTextSelection)
		{ sci_set_text(doc->editor->sci,
			(const gchar*) result_string); }
		else
		{
			sci_replace_sel(
				doc->editor->sci, (const gchar*) result_string);
		}

		// Change the cursor position to the start of the line
		// and scroll to there
		cursPos = sci_get_current_position(doc->editor->sci);
		colPos = sci_get_col_from_position(doc->editor->sci, cursPos);
		sci_set_current_position(doc->editor->sci,
			cursPos - colPos, TRUE);

		g_free(input_string);
		g_free(result_string);
	}
	else
	{
		msgwin_msg_add(COLOR_RED, -1, doc, "%s", noDataMsg);

		if (showErrorsInMsgPopupWindow)
			dialogs_show_msgbox(GTK_MESSAGE_INFO, "%s", noDataMsg);
	}
}


/* Geany plugin EP code */

static void item_activate_cb(GtkMenuItem *menuitem, gpointer gdata)
{
	base64_convert(document_get_current(), TRUE);
}

static void item_activate_cb2(GtkMenuItem *menuitem, gpointer gdata)
{
	base64_convert(document_get_current(), FALSE);
}

static void kb_text_to_base64_converter(G_GNUC_UNUSED guint key_id)
{
	base64_convert(document_get_current(), TRUE);
}

static void kb_base64_to_text_converter(G_GNUC_UNUSED guint key_id)
{
	base64_convert(document_get_current(), FALSE);
}


static void config_save_setting(GKeyFile *keyfile, const gchar *filePath)
{
	if (keyfile && filePath)
		g_key_file_save_to_file(keyfile, filePath, NULL);
}


static gboolean config_get_setting(GKeyFile *keyfile, const gchar *name)
{
	if (keyfile)
		return g_key_file_get_boolean(keyfile, "settings", name, NULL);

	return FALSE;
}


static void config_set_setting(GKeyFile *keyfile, const gchar *name,
								gboolean value)
{
	if (keyfile)
		g_key_file_set_boolean(keyfile, "settings", name, value);
}


static void on_configure_response(GtkDialog* dialog, gint response,
									gpointer user_data)
{
	gboolean value = FALSE;

	if (keyfile_plugin &&
		(response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY))
	{
		value = gtk_toggle_button_get_active(
					GTK_TOGGLE_BUTTON(
						log_formatting_success_messages_btn));
		logFormattingSuccessMessages = value;
		config_set_setting(keyfile_plugin,
							"log_formatting_success_messages", value);

		value = gtk_toggle_button_get_active(
						GTK_TOGGLE_BUTTON(show_errors_in_window_btn));
		showErrorsInMsgPopupWindow = value;
		config_set_setting(keyfile_plugin,
							"show_failure_messages_in_window", value);

		config_save_setting(keyfile_plugin, plugin_config_path);
	}
}

static void config_set_defaults(GKeyFile *keyfile)
{
	if (!keyfile) return;
	config_set_setting(keyfile, "log_formatting_success_messages", TRUE);
	logFormattingSuccessMessages = TRUE;
	config_set_setting(keyfile, "show_failure_messages_in_window", FALSE);
	showErrorsInMsgPopupWindow = FALSE;
}


void plugin_init(GeanyData *data)
{
	/* read & prepare configuration */
	gchar *config_dir = g_build_path(G_DIR_SEPARATOR_S,
		geany_data->app->configdir, "plugins", "base64converter", NULL);
	plugin_config_path = g_build_path(G_DIR_SEPARATOR_S, config_dir,
										"base64converter.conf", NULL);

	g_mkdir_with_parents(config_dir, S_IRUSR | S_IWUSR | S_IXUSR);
	g_free(config_dir);

	keyfile_plugin = g_key_file_new();

	if (!g_key_file_load_from_file(keyfile_plugin, plugin_config_path,
									G_KEY_FILE_NONE, NULL))
	{
		config_set_defaults(keyfile_plugin);
		config_save_setting(keyfile_plugin, plugin_config_path);
	}
	else
	{
		logFormattingSuccessMessages = config_get_setting(
			keyfile_plugin, "log_formatting_success_messages");

		showErrorsInMsgPopupWindow = config_get_setting(
			keyfile_plugin, "show_failure_messages_in_window");
	}

	/* ---------------------------- */

	main_menu_item = gtk_menu_item_new_with_mnemonic("Text to Base64");
	gtk_widget_show(main_menu_item);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu),
						main_menu_item);
	g_signal_connect(main_menu_item, "activate",
						G_CALLBACK(item_activate_cb), NULL);

	main_menu_item2 = gtk_menu_item_new_with_mnemonic("Base64 to Text");
	gtk_widget_show(main_menu_item2);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu),
						main_menu_item2);
	g_signal_connect(main_menu_item2, "activate",
						G_CALLBACK(item_activate_cb2), NULL);

	/* do not activate if there are do documents opened */
	ui_add_document_sensitive(main_menu_item);
	ui_add_document_sensitive(main_menu_item2);

	/* Register shortcut key group */
	geany_key_group = plugin_set_key_group(
						geany_plugin, _("base64_converter"), 2, NULL);

	/* Ctrl + Alt + 6 to convert to base64 */
	keybindings_set_item(geany_key_group, 0,
                         kb_text_to_base64_converter,
                         GDK_6, GDK_CONTROL_MASK | GDK_MOD1_MASK,
                         "text_to_base64",
                         _("Text to Base64"),
                         main_menu_item);

    /* Ctrl + Alt + 0 to convert from base64 */
	keybindings_set_item(geany_key_group, 1,
                         kb_base64_to_text_converter,
                         GDK_0, GDK_CONTROL_MASK | GDK_MOD1_MASK,
                         "base64_to_text",
                         _("Base64 to Text"),
                         main_menu_item2);
}


GtkWidget *plugin_configure(GtkDialog *dialog)
{
#if !(GTK_CHECK_VERSION(3, 2, 0))

	GtkWidget *vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget *_hbox1 = gtk_hbox_new(FALSE, 2);
	GtkWidget *_hbox2 = gtk_hbox_new(FALSE, 2);

#else

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	GtkWidget *_hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	GtkWidget *_hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

#endif

	log_formatting_success_messages_btn =
		gtk_check_button_new_with_label(
			_("Log successful conversion messages."));
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(log_formatting_success_messages_btn),
		config_get_setting(keyfile_plugin,
							"log_formatting_success_messages"));

	show_errors_in_window_btn = gtk_check_button_new_with_label(
		_("Show conversion failures in window."));
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(show_errors_in_window_btn),
		config_get_setting(keyfile_plugin,
							"show_failure_messages_in_window"));


	gtk_box_pack_start(GTK_BOX(_hbox1),
						log_formatting_success_messages_btn, TRUE,
						TRUE, 0);
	gtk_box_pack_start(GTK_BOX(_hbox2), show_errors_in_window_btn, TRUE,
						TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), _hbox1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), _hbox2, FALSE, FALSE, 0);

	gtk_widget_show_all(vbox);

	g_signal_connect(dialog, "response",
						G_CALLBACK(on_configure_response), NULL);

	return vbox;
}


void plugin_cleanup(void)
{
	g_free(plugin_config_path);
	g_key_file_free(keyfile_plugin);
	gtk_widget_destroy(main_menu_item2);
	gtk_widget_destroy(main_menu_item);
}
