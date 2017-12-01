/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

namespace zcc
{

static const size_t var_mime_address_name_max_length = 10240;
const size_t var_mime_header_line_max_length    = 1024000;
const size_t var_mime_header_line_max_element   = 10240;

struct mime_parser_cache_magic_t {
    int used;
    std::string *tmp_string[10];
    char line_cache[var_mime_header_line_max_length + 10];         /* var_mime_header_line_max_length */
}; 
typedef struct mime_parser_cache_magic_t mime_parser_cache_magic_t;
class mime_parser_cache_magic /* inner */
{
public:
    char *magic;              /* 'M' */
    char *true_data;
    gm_pool *gmp;             /* maybe null */
    mime_parser_cache_magic_t *cache;
    unsigned char tmp_string_idx_start;
    unsigned char tmp_string_used_count;
public:
    mime_parser_cache_magic();
    mime_parser_cache_magic(const mime_parser_cache_magic &_x);
    mime_parser_cache_magic(const void *data);
    ~mime_parser_cache_magic();
    std::string &require_string();
};

class mail_parser_mime_inner
{
public:
    inline mail_parser_mime_inner() {}
    inline ~mail_parser_mime_inner() {}
    char *type;
    char *encoding;
    char *charset;
    char *disposition;
    char *show_name;
    char *name;
    char *name_utf8;
    char *filename;
    char *filename2231;
    char *filename_utf8;
    char *content_id;
    char *boundary;
    /* mime proto, for imapd */
    char *imap_section;
    int header_offset;
    int header_len;
    int body_offset;
    int body_len;

    short int mime_type; /* inner user */
    bool is_tnef;
    bool filename2231_with_charset;

    /* mime original header-logic-line */
    std::list<size_data_t *> header_lines;

    /* relationship */
    mail_parser_mime *wrap;
    mail_parser_mime_inner *next;
    mail_parser_mime_inner *child;
    mail_parser_mime_inner *parent;
    /* */
    mail_parser_inner * parser;
};

class mail_parser_inner
{
public:
    inline mail_parser_inner() {}
    inline ~mail_parser_inner() {}
    char *subject;
    char *subject_utf8;
    char *date;
    long date_unix;
    short int from_flag:2;
    short int sender_flag:2;
    short int reply_to_flag:2;
    short int to_flag:2;
    short int cc_flag:2;
    short int bcc_flag:2;
    short int receipt_flag:2;
    short int references_flag:2;
    short int classify_flag:2;
    short int section_flag:2;
    mime_address from;
    mime_address sender;
    mime_address reply_to;
    std::list<mime_address *> to;
    std::list<mime_address *> cc;
    std::list<mime_address *> bcc;
    mime_address receipt;
    char *in_reply_to;
    char *message_id;
    std::list<char *> references;

    /* mime-tree */
    mail_parser_mime *top_mime;

    /* all-mime-std::list */
    std::list<mail_parser_mime *> all_mimes;

    /* text(plain,html) type mime-list except for attachment */
    std::list<mail_parser_mime *> text_mimes;

    /* similar to the above, 
     * in addition to the case of alternative, html is preferred */
    std::list<mail_parser_mime *> show_mimes;

    /* attachment(and background-image) type mime-list */
    std::list<mail_parser_mime *> attachment_mimes;

    /* option */
    short int mime_max_depth;
    char src_charset_def[32];

    /* other */
    gm_pool *gmp;
    char *mail_data;
    char *mail_pos;
    int mail_size;
    /* tmp or cache */
    mime_parser_cache_magic mcm;
    mime_address_parser addr_parser;
};

}
