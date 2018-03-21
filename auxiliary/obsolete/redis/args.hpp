/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

namespace zcc
{

class redis_cmd_args
{
public:
    redis_cmd_args(const char *cmd);
    ~redis_cmd_args();
    redis_cmd_args &append(double f);
    redis_cmd_args &append(long l);
    inline redis_cmd_args &append(int l) { return append((long)l); }
    redis_cmd_args &append(const char *s, size_t slen = 0);
    inline redis_cmd_args &append(const std::string &S) { return append(S.c_str(), S.size()); }
    redis_cmd_args &append(const std::list<std::string> &a_list);
    redis_cmd_args &vappend(const char *fmt, ...);
    redis_cmd_args &vappend(const char *fmt, va_list ap);
    redis_cmd_args &vappend(int inner_type, va_list ap);
    int a_count;
    std::string a_query; 
    long *a_nval;
    std::string *a_sval;
    std::list<std::string> *a_lval;
    json *a_jval;
    std::string a_cmd;
    std::string a_key;
    std::string a_fixed_destination;
    int a_slot;
    std::string a_slot_destination;
    /* */
    int n_slot;
    int n_fd;
    int n_addr;
    int n_port;
    short int n_key_slot;
};

redis_cmd_args::redis_cmd_args(const char *cmd)
{
    a_cmd = cmd;
    append(cmd);
    a_count = 1;
    a_slot = -1;
}

redis_cmd_args::~redis_cmd_args()
{
}

redis_cmd_args &redis_cmd_args::append(double f)
{
    char numberbuf[1000];
    int nlen = sprintf(numberbuf, "%lf", f);
    char lenbuf[64];
    sprintf(lenbuf, "%d", nlen);
    a_query.append("$").append(lenbuf).append("\r\n").append(numberbuf).append("\r\n");
    a_count ++;
    return *this;
}

redis_cmd_args &redis_cmd_args::append(long l)
{
    char numberbuf[1000];
    int nlen = sprintf(numberbuf, "%ld", l);
    char lenbuf[64];
    sprintf(lenbuf, "%d", nlen);
    a_query.append("$").append(lenbuf).append("\r\n").append(numberbuf).append("\r\n");
    a_count ++;
    return *this;
}

redis_cmd_args &redis_cmd_args::append(const char *s, size_t slen)
{
    char lenbuf[64];
    size_t len;
    if (slen > 0) {
        len = slen;
    } else {
        if (s == 0) {
            len = 0;
            s = "";
        } else {
            len = ::strlen(s);
        }
    }
    sprintf(lenbuf, "%zd", len);
    a_query.append("$").append(lenbuf).append("\r\n");
    if (len > 0) {
        a_query.append(s, len);
    }
    a_query.append("\r\n");
    a_count ++;
    return *this;
}

redis_cmd_args &redis_cmd_args::append(const std::list<std::string> &a_list)
{
    for (std::list<std::string>::const_iterator it = a_list.begin(); it != a_list.end(); it++) {
        append(it->c_str(), it->size());
    }
    return *this;
}

redis_cmd_args &redis_cmd_args::vappend(const char *redis_fmt, ...)
{
    va_list ap;
    va_start(ap, redis_fmt);
    vappend(redis_fmt, ap);
    va_end(ap);
    return *this;
}

redis_cmd_args &redis_cmd_args::vappend(const char *redis_fmt, va_list ap)
{
    int ch;
    long dv;
    double fv;
    const char *sv;
    const std::string *Sv;
    if (redis_fmt == 0) {
        return *this;
    }
    const char *fmt = redis_fmt;
    while((ch = *fmt++)) {
        if (ch == 's') {
            sv = va_arg(ap, const char *);
            if (sv != ((const char *)-9)) {
                if (sv) {
                    append(sv);
                } else {
                    append("");
                }
            } else {
            }
        } else if (ch == 'S') {
            Sv = va_arg(ap, const std::string *);
            if (Sv && (!is_std_string_ignore(*Sv))) {
                append(Sv->c_str(), Sv->size());
            }
        } else if (ch == 'd') {
            dv = va_arg(ap, long);
            append(dv);
        } else if (ch == 'f') {
            fv = va_arg(ap, double);
            append(fv);
        } else if (ch == 'L') {
            const std::list<std::string> *lv = va_arg(ap, const std::list<std::string> *);
            if (lv) {
                append(*lv);
            }
        }
    }

    return *this;
}

redis_cmd_args &redis_cmd_args::vappend(int inner_type, va_list ap)
{
    const char *sv;
    const std::string *Sv;
    if (inner_type == 1) {
        while((sv = va_arg(ap, const char *))) {
            append(sv);
        }
    } else {
        while((Sv = va_arg(ap, std::string *))) {
            append(Sv->c_str(), Sv->size());
        }
    }
    return *this;
}

}
