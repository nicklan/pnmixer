#ifndef PREFS_H_
#define PREFS_H_

GKeyFile* keyFile;
int scroll_step;
GtkIconTheme* icon_theme;

GtkWidget* create_prefs_window (void);
void apply_prefs(void);
void load_prefs(void);
void get_icon_theme();
#endif // PREFS_H_
