// #include "dr_api.h"

// static char *dr_strtok(char *str, const char *delim);

// static char *strtok_saved_ptr = NULL;

// static char *dr_strtok(char *str, const char *delim) {
//     if (str == NULL) {
//         str = strtok_saved_ptr;
//         if (str == NULL) {
//             return NULL;
//         }
//     } else {
//         strtok_saved_ptr = str;
//     }

//     // Skip leading delimiters
//     str += dr_strspn(str, delim);
//     if (*str == '\0') {
//         strtok_saved_ptr = NULL;
//         return NULL;
//     }

//     // Find the end of the token
//     char *end = str + dr_strcspn(str, delim);
//     if (*end == '\0') {
//         strtok_saved_ptr = NULL;
//     } else {
//         *end = '\0';
//         strtok_saved_ptr = end + 1;
//     }

//     return str;
// }

// static size_t dr_strspn(const char *str, const char *delim) {
//     const char *s = str;
//     while (*s && dr_strchr(delim, *s)) {
//         s++;
//     }
//     return s - str;
// }

// static size_t dr_strcspn(const char *str, const char *delim) {
//     const char *s = str;
//     while (*s && !dr_strchr(delim, *s)) {
//         s++;
//     }
//     return s - str;
// }

// static char *dr_strchr(const char *str, int c) {
//     while (*str) {
//         if (*str == c) {
//             return (char *)str;
//         }
//         str++;
//     }
//     return NULL;
// }