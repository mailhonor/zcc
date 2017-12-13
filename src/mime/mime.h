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

class mail_parser_mime_engine
{
public:
    mail_parser_mime_engine(mail_parser_engine *parser);
    ~mail_parser_mime_engine();
    unsigned short int encoding_flag:1;
    unsigned short int disposition_flag:1;
    unsigned short int show_name_flag:1;
    unsigned short int name_flag:1;
    unsigned short int name_utf8_flag:1;
    unsigned short int filename_flag:1;
    unsigned short int filename2231_flag:1;
    unsigned short int filename_utf8_flag:1;
    unsigned short int content_id_flag:1;
    std::string type;
    std::string encoding;
    std::string charset;
    std::string disposition;
    std::string *show_name;
    std::string name;
    std::string name_utf8;
    std::string filename;
    std::string filename2231;
    std::string filename_utf8;
    std::string content_id;
    std::string boundary;
    /* mime proto, for imapd */
    std::string imap_section;
    int header_offset;
    int header_len;
    int body_offset;
    int body_len;

    short int mime_type; /* inner user */
    bool is_tnef;
    bool filename2231_with_charset;

    /* mime original header-logic-line */
    std::list<size_data_t> raw_header_lines;

    /* relationship */
    mail_parser_mime *wrap;
    mail_parser_mime_engine *next;
    mail_parser_mime_engine *child;
    mail_parser_mime_engine *parent;
    /* */
    mail_parser_engine * parser;
};

class mail_parser_engine
{
public:
    mail_parser_engine();
    ~mail_parser_engine();
    unsigned short int parsed_flag:1;
    unsigned short int subject_flag:1;
    unsigned short int subject_utf8_flag:1;
    unsigned short int date_flag:1;
    unsigned short int date_unix_flag:1;
    unsigned short int message_id_flag:1;
    unsigned short int in_reply_to_flag:1;
    unsigned short int from_flag:2;
    unsigned short int sender_flag:2;
    unsigned short int reply_to_flag:2;
    unsigned short int to_flag:2;
    unsigned short int cc_flag:2;
    unsigned short int bcc_flag:2;
    unsigned short int receipt_flag:2;
    unsigned short int references_flag:2;
    unsigned short int classify_flag:2;
    unsigned short int section_flag:2;
    std::string subject;
    std::string subject_utf8;
    std::string date;
    long date_unix;
    mime_address from;
    mime_address sender;
    mime_address reply_to;
    std::list<mime_address> to;
    std::list<mime_address> cc;
    std::list<mime_address> bcc;
    mime_address receipt;
    std::string in_reply_to;
    std::string message_id;
    std::list<std::string> references;

    /* mime-tree */
    mail_parser_mime *top_mime;
    mail_parser_mime_engine *top_mime_engine;

    /* all-mime-std::list */
    std::list<mail_parser_mime *> all_mimes;
    std::list<mail_parser_mime_engine *> all_mimes_engine;

    /* text(plain,html) type mime-list except for attachment */
    std::list<mail_parser_mime *> text_mimes;

    /* similar to the above, 
     * in addition to the case of alternative, html is preferred */
    std::list<mail_parser_mime *> show_mimes;

    /* attachment(and background-image) type mime-list */
    std::list<mail_parser_mime *> attachment_mimes;

    /* option */
    short int mime_max_depth;
    std::string src_charset_def;

    /* other */
    char *mail_data;
    char *mail_pos;
    int mail_size;
    file_mmap fmmap;
};

}
