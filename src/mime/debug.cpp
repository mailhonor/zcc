
/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
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
    parser->raw_header_line(h, tmpstr);
    printf(fmt, h, tmpstr.c_str());
    printf(fmt, "", addr.address.c_str());
    printf(fmt, "", addr.name.c_str());
    printf(fmt, "", addr.name_utf8.c_str());
}

static void _debug_show_addr_vector(mail_parser *parser, const char *h, const std::list<mime_address> &address)
{
    const char *fmt = "%15s: %s\n";
    char nh[256];
    std::string tmpstr;
    int i = 0;

    parser->raw_header_line(h, tmpstr);
    printf(fmt, h, tmpstr.c_str());

    std_list_walk_begin(address, addr) {
        if (i == 0) {
            sprintf(nh, "%s (1)", h);
        } else {
            sprintf(nh, "(%d)", i+1);
        }
        i ++;
        printf(fmt, nh, addr.address.c_str());
        printf(fmt, "", addr.name.c_str());
        printf(fmt, "", addr.name_utf8.c_str());
    } std_list_walk_end;
}

void mail_parser::debug_show()
{
    const char *fmt = "%15s: %s\n";
    std::string tmpstr;
    size_t i = 0;

    printf(fmt, "Date", date().c_str());
    printf("%15s: %ld\n","", date_unix());
    printf("\n");
    printf(fmt, "Subject", subject().c_str());
    printf(fmt, "", subject_utf8().c_str());
    printf("\n");
    _debug_show_addr(this, "From", from_utf8());
    printf("\n");
    _debug_show_addr(this, "Sender", sender());
    printf("\n");
    _debug_show_addr(this, "Reply-To", reply_to());
    printf("\n");
    _debug_show_addr(this, "Disposition-Notification-To", receipt());
    printf("\n");
    printf(fmt, "In-Reply-To", in_reply_to().c_str());
    printf("\n");
    _debug_show_addr_vector(this, "To", to_utf8());
    printf("\n");
    _debug_show_addr_vector(this, "Cc", cc_utf8());
    printf("\n");
    _debug_show_addr_vector(this, "Bcc", bcc_utf8());
    printf("\n");
    printf(fmt, "Message-Id", message_id().c_str());

    {
        i = 0;
        printf("\n");
        raw_header_line("References", tmpstr);
        printf(fmt, "References", tmpstr.c_str());
        std_list_walk_begin(references(), r) {
            if (i==0) {
                printf(fmt, "References", r.c_str());
            } else {
                printf(fmt, "", r.c_str());
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
        printf(fmt, buf, m->type().c_str());
        raw_header_line("Content-Type", tmpstr);
        printf(fmt, "Content-Type", tmpstr.c_str());
        raw_header_line("Content-Transfer-Encoding", tmpstr);
        printf(fmt, "Content-Transfer-Encoding", tmpstr.c_str());
        raw_header_line("Content-Disposition", tmpstr);
        printf(fmt, "Content-Disposition", tmpstr.c_str());
        printf(fmt, "disposition", m->disposition().c_str());
        printf(fmt, "name", m->name().c_str());
        printf(fmt, "name_utf8", m->name_utf8().c_str());
        printf(fmt, "filename", m->filename().c_str());
        printf(fmt, "filename2231", m->filename2231().c_str());
        printf(fmt, "filename_utf8", m->filename_utf8().c_str());
        printf(fmt, "content_id", m->content_id().c_str());
        printf(fmt, "encoding", m->encoding().c_str());
        printf(fmt, "boundary", m->boundary().c_str());
        printf(fmt, "section", m->imap_section().c_str());
        sprintf(buf, "%zd,%zd,%zd,%zd", m->header_offset(), m->header_size(), m->body_offset(), m->body_size());
        printf(fmt, "", buf);
        i++;
    } std_list_walk_end;

    i = 0;
    const std::list<mail_parser_mime *> &textm = text_mimes();
    std_list_walk_begin(textm, m) {
        printf("\n");
        (*m).decoded_content_utf8(tmpstr);
        printf(fmt, m->type().c_str(),  tmpstr.c_str());
    } std_list_walk_end;
}

}
