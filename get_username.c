#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
  #include <windows.h>
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <pwd.h>
#endif

char *get_username(void) {
    char *result = NULL;

#if defined(_WIN32)
    char *env = getenv("USERNAME");
    if (env && env[0]) {
        int wlen = MultiByteToWideChar(CP_ACP, 0, env, -1, NULL, 0);
        if (wlen > 0) {
            wchar_t *wenv = (wchar_t*)malloc(wlen * sizeof(wchar_t));
            if (wenv && MultiByteToWideChar(CP_ACP, 0, env, -1, wenv, wlen)) {
                int utf8len = WideCharToMultiByte(CP_UTF8, 0, wenv, -1, NULL, 0, NULL, NULL);
                if (utf8len > 0) {
                    result = (char*)malloc(utf8len);
                    if (result) {
                        WideCharToMultiByte(CP_UTF8, 0, wenv, -1, result, utf8len, NULL, NULL);
                    }
                }
            }
            free(wenv);
            if (result) return result;
        }
    }

    wchar_t wname[256];
    DWORD len = 256;
    if (GetUserNameW(wname, &len)) {
        int utf8len = WideCharToMultiByte(CP_UTF8, 0, wname, -1, NULL, 0, NULL, NULL);
        if (utf8len > 0) {
            result = (char*)malloc(utf8len);
            if (result) {
                WideCharToMultiByte(CP_UTF8, 0, wname, -1, result, utf8len, NULL, NULL);
            }
        }
    }
#else
    char *env = getenv("USER");
    if (env && env[0]) {
        return strdup(env);
    }

    struct passwd *pw = getpwuid(geteuid());
    if (pw && pw->pw_name) {
        return strdup(pw->pw_name);
    }
#endif
    if (!result) {
        result = strdup("Usuário");
    }
    return result;
}