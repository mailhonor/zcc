
/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-01-13
 * ================================
 */

#include "zcc.h"
#include "mime.h"

namespace zcc
{

static void _debug_show_addr(mail_parser *parser, const char *h, const mime_address &addr)
{
    const char *fmt = "%15s: %s\n";
    std::string tmpstr;
    parser->header_line(h, tmpstr);
    printf(fmt, h, tmpstr.c_str());
    printf(fmt, "", addr.address());
    printf(fmt, "", addr.name());
    printf(fmt, "", addr.name_utf8());
}

static void _debug_show_addr_vector(mail_parser *parser, const char *h, const std::list<mime_address *> &address)
{
    const char *fmt = "%15s: %s\n";
    char nh[256];
    std::string tmpstr;
    int i = 0;

    parser->header_line(h, tmpstr);
    printf(fmt, h, tmpstr.c_str());

    std_list_walk_begin(address, addr) {
        if (i == 0) {
            sprintf(nh, "%s (1)", h);
        } else {
            sprintf(nh, "(%d)", i+1);
        }
        i ++;
        printf(fmt, nh, addr->address());
        printf(fmt, "", addr->name());
        printf(fmt, "", addr->name_utf8());
    } std_list_walk_end;
}

void mail_parser::debug_show()
{
    const char *fmt = "%15s: %s\n";
    std::string tmpstr;
    size_t i = 0;

    printf(fmt, "Date", date());
    printf("%15s: %ld\n","", date_unix());
    printf("\n");
    printf(fmt, "Subject", subject());
    printf(fmt, "", subject_utf8());
    printf("\n");
    _debug_show_addr(this, "From", from_utf8());
    printf("\n");
    _debug_show_addr(this, "Sender", sender());
    printf("\n");
    _debug_show_addr(this, "Reply-To", reply_to());
    printf("\n");
    _debug_show_addr(this, "Disposition-Notification-To", receipt());
    printf("\n");
    printf(fmt, "In-Reply-To", in_reply_to());
    printf("\n");
    _debug_show_addr_vector(this, "To", to_utf8());
    printf("\n");
    _debug_show_addr_vector(this, "Cc", cc_utf8());
    printf("\n");
    _debug_show_addr_vector(this, "Bcc", bcc_utf8());
    printf("\n");
    printf(fmt, "Message-Id", message_id());

    {
        i = 0;
        printf("\n");
        const std::list<char *> &rf = references();
        header_line("References", tmpstr);
        printf(fmt, "References", tmpstr.c_str());
        std_list_walk_begin(rf, r) {
            if (i==0) {
                printf(fmt, "References", r);
            } else {
                printf(fmt, "", r);
            }
            i++;
        } std_list_walk_end;
    }

    const std::list<mail_parser_mime *> &allm = all_mimes();
    i = 0;
    std_list_walk_begin(allm, m) {
        printf("\n");
        char buf[128];
        sprintf(buf, "Mime (%zd)", i+1);
        printf(fmt, buf, m->type());
        header_line("Content-Type", tmpstr);
        printf(fmt, "Content-Type", tmpstr.c_str());
        header_line("Content-Transfer-Encoding", tmpstr);
        printf(fmt, "Content-Transfer-Encoding", tmpstr.c_str());
        header_line("Content-Disposition", tmpstr);
        printf(fmt, "Content-Disposition", tmpstr.c_str());
        printf(fmt, "disposition", m->disposition());
        printf(fmt, "name", m->name());
        printf(fmt, "name_utf8", m->name_utf8());
        printf(fmt, "filename", m->filename());
        printf(fmt, "filename2231", m->filename2231());
        printf(fmt, "filename_utf8", m->filename_utf8());
        printf(fmt, "content_id", m->content_id());
        printf(fmt, "encoding", m->encoding());
        printf(fmt, "boundary", m->boundary());
        printf(fmt, "section", m->imap_section());
        sprintf(buf, "%zd,%zd,%zd,%zd", m->header_offset(), m->header_size(), m->body_offset(), m->body_size());
        printf(fmt, "", buf);
        i++;
    } std_list_walk_end;

    i = 0;
    const std::list<mail_parser_mime *> &textm = text_mimes();
    std_list_walk_begin(textm, m) {
        printf("\n");
        (*m).decoded_content_utf8(tmpstr);
        printf(fmt, m->type(),  tmpstr.c_str());
    } std_list_walk_end;
}

}
