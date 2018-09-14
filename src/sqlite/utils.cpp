/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2018-02-24
 * ================================
 */

#include "zcc.h"

namespace zcc
{

std::string &sqlite3_escape_append(std::string &sql, const void *data, size_t size)
{
    size_t i;
    const char *s = (const char *)data;
    if (size == var_size_max) {
        size = strlen(s);
    }
    int ch;
    for(i = 0; i < size; i++) {
        ch = s[i];
        if (ch == '\'' || ch == '\\') {
            sql.push_back('\\');
            sql.push_back(ch);
            continue;
        }
#if 0
        switch(ch) {
            case '/':
            case '[':
            case ']':
            case '%':
            case '&':
            case '_':
            case '(':
            case ')':
                sql.push_back('\\');
        }
#endif
        sql.push_back(ch);
    }
    return sql;
}

}

#if 0
public static String sqliteEscape(String keyWord){
    keyWord = keyWord.replace("/", "//");
    keyWord = keyWord.replace("'", "''");
    keyWord = keyWord.replace("[", "/[");
    keyWord = keyWord.replace("]", "/]");
    keyWord = keyWord.replace("%", "/%");
    keyWord = keyWord.replace("&","/&");
    keyWord = keyWord.replace("_", "/_");
    keyWord = keyWord.replace("(", "/(");
    keyWord = keyWord.replace(")", "/)");
    return keyWord;
}
#endif
