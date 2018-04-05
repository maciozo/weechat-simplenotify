#pragma once

#define MSG_OTHER 1
#define MSG_HIGHLIGHT 2
#define MSG_QUERY 4
#define MSG_NOTICE 8

typedef struct s_tags_command {
  char** tags;
  char* command;
  int tag_count;
} t_tags_command;

// Callback for print
int print_cb (
	const void *tags_command,
	void *data,
	struct t_gui_buffer *buffer,
  time_t date,
	int tags_count,
	const char **tags,
  int displayed,
	int highlight,
  const char *prefix,
	const char *message
);

char *str_replace(char *orig, char *rep, char *with);

int config_write_default (
	const void *pointer,
	void *data,
	struct t_config_file *config_file,
  const char *section_name
);

int config_init (void);

int weechat_plugin_init (
	struct t_weechat_plugin *plugin,
	int argc,
	char *argv[]
);

int weechat_plugin_end (
	struct t_weechat_plugin *plugin
);
