#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libconfig.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <security/pam_appl.h>
#include <security/pam_modules.h>

#define CONFIG_FILE "/etc/touchid-remote.conf"

PAM_EXTERN int pam_sm_setcred( pam_handle_t *pamh, int flags, int argc, const char **argv ) {
	return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	printf("Acct mgmt\n");
	return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_authenticate( pam_handle_t *pamh, int flags,int argc, const char **argv ) {
  FILE *fp;
  char buf[1024];
  int rc;
  struct passwd *p;
  struct sockaddr_in server_address;
  int c, s, r, i, valid_user;
  char *pos;
  struct stat st;
  config_t cfg;
  config_setting_t *setting;
  const char *key1;
  const char *key2;
  const char *user;
  int server_port;
  char *e;

  fp = popen("echo $USE_TOUCHID", "r");
  fread((char *)&buf, 1, 1, fp);
  pclose(fp);

  if (buf[0] != '1') {
    return PAM_AUTH_ERR;
  }

  config_init(&cfg);

  if (!config_read_file(&cfg, CONFIG_FILE)) {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
    config_destroy(&cfg);
    return PAM_AUTH_ERR;
  }

  if (!config_lookup_int(&cfg, "server_port", &server_port)) {
    fprintf(stderr, "No 'server_port' setting in configuration file.\n");
    return PAM_AUTH_ERR;
  }

  if (!config_lookup_string(&cfg, "key1", &key1)) {
    fprintf(stderr, "No 'key1' setting in configuration file.\n");
    return PAM_AUTH_ERR;
  }

  if (strlen(key1) >128) {
    fprintf(stderr, "Max key length is 128 bytes\n");
    return PAM_AUTH_ERR;
  }

  if (!config_lookup_string(&cfg, "key2", &key2)) {
    fprintf(stderr, "No 'key2' setting in configuration file.\n");
    return PAM_AUTH_ERR;
  }

  if (strlen(key2) >128) {
    fprintf(stderr, "Max key length is 128 bytes\n");
    return PAM_AUTH_ERR;
  }

  setting = config_lookup(&cfg, "users");

  if (setting == NULL) {
    fprintf(stderr, "No 'users' setting in configuration file.\n");
    return PAM_AUTH_ERR;
  }

  if (config_setting_length(setting) == 0) {
    fprintf(stderr, "No users defined in configuration file.\n");
    return PAM_AUTH_ERR;
  }

  p = getpwuid(getuid());

  for(i = 0; i < config_setting_length(setting); i++) {
    config_setting_t *user_setting = config_setting_get_elem(setting, i);

    user = config_setting_get_string(user_setting);

    if (strcmp(user, p->pw_name) == 0) {
      valid_user = 1;
      break;
    }
  }

  if (valid_user == 0) {
    fprintf(stderr, "invalid user\n");
    return PAM_AUTH_ERR;
  }

  s = socket(AF_INET, SOCK_STREAM, 0);

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(server_port);
  server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (connect(s, (struct sockaddr *)&server_address, sizeof(server_address)) != 0) {
    fprintf(stderr, "error: connect() failed\n");
    return PAM_AUTH_ERR;
  }

  snprintf((char *)&buf, sizeof(buf)-1, "GET /auth HTTP/1.1\rHost: localhost:%d\rUser-Agent: touchid-remote\rAccept: */*\rAuth: %s\r\r", server_port, key1);

  send(s, &buf, strlen(buf), 1);

  memset(&buf, 0, sizeof(buf));

  recv(s, &buf, sizeof(buf)-1, 0);

  close(s);

  for (i=0; i<strlen(buf); i++) {
    if (buf[i] == '\n' && i+2 < strlen(buf) && buf[i+1] == '\n') {
      if (strcmp(&buf[i+2], key2) == 0) {
        return PAM_SUCCESS;
      }
      break;
    }
  }

  return PAM_AUTH_ERR;
}
