#include "weechat-plugin.h"
#include "simplenotify.h"
#include <string.h>
#include <stdlib.h>

WEECHAT_PLUGIN_NAME("simplenotify");
WEECHAT_PLUGIN_DESCRIPTION("Generate a desktop notification on query or highlight");
WEECHAT_PLUGIN_AUTHOR("maciozo <maciozo@maciozo.com>");
WEECHAT_PLUGIN_VERSION("0.1");
WEECHAT_PLUGIN_LICENSE("GPL3");

struct t_weechat_plugin *weechat_plugin = NULL;
char *command = NULL;

// Config pointers
struct t_config_file *config_file = NULL;
struct t_config_section *config_section = NULL;
int newConfig = 0;

// Callback for print
int
print_cb (
	const void *tg,
	void *data,
	struct t_gui_buffer *buffer,
  time_t date,
	int tags_count,
	const char **tags,
  int displayed,
	int highlight,
  const char *prefix,
	const char *message
) {
	char *formatted_command;
	unsigned int message_type = 0;
	t_tags_command *tags_command = (t_tags_command*) tg;

	for (int i = 0; i < tags_command->tag_count; i++) {
		for (int j = 0; j < tags_count; j++) {
			if (!strcmp(tags_command->tags[i], tags[j])) {
				if (!strcmp(tags[j], "notify_private")) {
					message_type |= MSG_QUERY;
				} else {
					if (!strcmp(tags[j], "notify_highlight")) {
						message_type |= MSG_HIGHLIGHT;
					} else {
						if (!strcmp(tags[j], "irc_notice")) {
							message_type |= MSG_NOTICE;
						}
					}
				}
			}
		}
	}

	if (message_type == 0) {
		message_type = MSG_OTHER;
	}

	if (message_type & MSG_QUERY) {
		formatted_command = str_replace(tags_command->command, "{type}", "Query");
	} else {
		if (message_type & MSG_NOTICE) {
			formatted_command = str_replace(tags_command->command, "{type}", "Notice");
		} else {
			if (message_type & MSG_HIGHLIGHT) {
				formatted_command = str_replace(tags_command->command, "{type}", "Highlight");
			} else {
				if (message_type & MSG_OTHER) {
					formatted_command = str_replace(tags_command->command, "{type}", "Message");
				}
			}
		}
	}

	// formatted_command = str_replace(tags_command->command, "{origin}", "Query");

	formatted_command = str_replace(formatted_command, "{message}", (char*) message);

	weechat_printf (NULL, "[simplenotify] %s", formatted_command);

	// Replace macros in command (https://stackoverflow.com/questions/779875/what-is-the-function-to-replace-string-in-c)
	// Run command

	return WEECHAT_RC_OK;
}

// https://stackoverflow.com/a/779960
// You must free the result if result is non-NULL.
char *str_replace(char *orig, char *rep, char *with) {
  char *result; // the return string
  char *ins;    // the next insert point
  char *tmp;    // varies
  int len_rep;  // length of rep (the string to remove)
  int len_with; // length of with (the string to replace rep with)
  int len_front; // distance between rep and end of last rep
  int count;    // number of replacements

  // sanity checks and initialization
  if (!orig || !rep)
      return NULL;
  len_rep = strlen(rep);
  if (len_rep == 0)
      return NULL; // empty rep causes infinite loop during count
  if (!with)
      with = "";
  len_with = strlen(with);

  // count the number of replacements needed
  ins = orig;
  for (count = 0; (tmp = strstr(ins, rep)); ++count) {
      ins = tmp + len_rep;
  }

  tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

  if (!result)
      return NULL;

  // first time through the loop, all the variable are set correctly
  // from here on,
  //    tmp points to the end of the result string
  //    ins points to the next occurrence of rep in orig
  //    orig points to the remainder of orig after "end of rep"
  while (count--) {
      ins = strstr(orig, rep);
      len_front = ins - orig;
      tmp = strncpy(tmp, orig, len_front) + len_front;
      tmp = strcpy(tmp, with) + len_with;
      orig += len_front + len_rep; // move to next "end of rep"
  }
  strcpy(tmp, orig);
  return result;
}

int
config_write_default (
	const void *pointer,
	void *data,
	struct t_config_file *config_file,
  const char *section_name
) {
	// Write defaults
	int status;

	status = weechat_config_write_line (config_file, "var", NULL);
	if (!status) {
		return WEECHAT_CONFIG_WRITE_ERROR;
	}

	status = weechat_config_write_line(
		config_file,
		"tags",
		"notify_private,notify_highlight,irc_notice"
	);
	if (!status) {
		return WEECHAT_CONFIG_WRITE_ERROR;
	}

	status = weechat_config_write_line(
		config_file,
		"command",
		"notify-send -t 3000 -a 'WeeChat' -c im.received '{type} from {origin}' '{message}'"
	);
	if (!status) {
		return WEECHAT_CONFIG_WRITE_ERROR;
	}

	return WEECHAT_CONFIG_WRITE_OK;
}

int
config_init (void) {
	config_file = weechat_config_new (
		"simplenotify", // Config file name
	  NULL, // No callback for /reload
	  NULL, // No pointer to callback
		NULL // No pointer to callback data
	);

	if (config_file == NULL) {
		weechat_printf (NULL, "%s[simplenotify] Error accessing config file", weechat_prefix("error"));
		return WEECHAT_CONFIG_READ_MEMORY_ERROR;
	}

	config_section = weechat_config_search_section (
		config_file,
		"var"
	);

	if (config_section == NULL) {
		// var section not found
		weechat_config_free (config_file);

		config_section = weechat_config_new_section (
			config_file,
			"var",
			0, // User can't add new options
			0, // User can't delete options
			NULL, // No callback for read
			NULL, // No pointer for read callback
			NULL, // No pointer for read callback data
			NULL, // No callback for write
			NULL,
			NULL,
			&config_write_default, // Callback for writing defaults
			NULL,
			NULL,
			NULL, // No callback for creating options
			NULL,
			NULL,
			NULL, // No callback for deleting options
			NULL,
			NULL
		);

		if (config_section == NULL) {
			return WEECHAT_CONFIG_WRITE_ERROR;
		}
		weechat_config_write(config_file);
	}

	return 0;
}

// Plugin initialistion
int
weechat_plugin_init (
	struct t_weechat_plugin *plugin,
	int argc,
	char *argv[]
) {
	weechat_plugin = plugin;

	t_tags_command tags_command;

	if (config_init() != 0) {
		weechat_printf (NULL, "%s[simplenotify] Error accessing configuration file", weechat_prefix("error"));
		return WEECHAT_RC_ERROR;
	}

	struct t_config_option *option_tags = weechat_config_search_option (
		config_file,
		config_section,
		"tags"
	);

	struct t_config_option *option_command = weechat_config_search_option (
		config_file,
		config_section,
		"command"
	);

	if (option_tags == NULL) {
		weechat_printf (NULL, "%s[simplenotify] Error reading tags option", weechat_prefix("error"));
		return WEECHAT_RC_ERROR;
	}

	if (option_command == NULL) {
		weechat_printf (NULL, "%s[simplenotify] Error reading command option", weechat_prefix("error"));
		return WEECHAT_RC_ERROR;
	}

	tags_command.tags = weechat_string_split(
		weechat_config_string(option_tags), // String to split
		",", // Separator
		0, // Each string will comtain 1 word
		0, // No maximum number of words
		&(tags_command.tag_count) // Pointer to int for item count
	);

	tags_command.command = (char*) weechat_config_string(option_command);

	if (weechat_hook_print (
		NULL, // Catch print from all buffers
		weechat_config_string(option_tags), // Tags we're interested in
		NULL, // Catch all messages
		1, // Strip colours from message
		&print_cb, // Callback function
		&tags_command, // Send the command to the callback function
		NULL // No callback data pointer
	) == NULL) {
		weechat_printf (NULL, "%s[simplenotify] Error initialising print hook", weechat_prefix("error"));
		return WEECHAT_RC_ERROR;
	}

	return WEECHAT_RC_OK;
}

// Plugin deinitialisation
int
weechat_plugin_end (
	struct t_weechat_plugin *plugin
) {
	return WEECHAT_RC_OK;
}
