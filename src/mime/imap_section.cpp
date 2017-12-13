/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-10
 * ================================
 */

#include "zcc.h"
#include "mime.h"

namespace zcc
{

static inline int ___child_count(mail_parser_mime_engine * m)
{
    int count = 0;
    for (m = m->child; m; m = m->next) {
        count++;
    }
    return count;
}

static void ___mime_section(mail_parser_engine * parser, mail_parser_mime_engine * mime, char *section)
{
    int i, count;
    char nsection[10240], intbuf[16];
    mail_parser_mime_engine *cm, *fm;
    argv zav;
    std::string zb;

    mime->imap_section = section;

    zav.split(section, ".");
    count = ___child_count(mime);
    fm = mime->child;
    cm = fm;
    for (i = 0; i < count; i++, cm = cm->next) {
        if (!strcmp(section, "")) {
            sprintf(nsection, "%d", i + 1);
        } else if ((zav.size()> 1) && (strncasecmp(fm->type.c_str(), "multipart/", 11))) {
            zb.clear();
            for (size_t k = 0; k < zav.size() - 1; k++) {
                zb.append(zav[k]);
                zb.push_back('.');
            }
            sprintf(intbuf, "%d", i + 1);
            zb.append(intbuf);
            strcpy(nsection, zb.c_str());
        } else {
            if (!strcmp(zav[zav.size() - 1], "0")) {
                zb.clear();
                for (size_t k = 0; k < zav.size() - 1; k++) {
                    zb.append(zav[k]);
                    zb.push_back('.');
                }
                sprintf(intbuf, "%d", i + 1);
                zb.append(intbuf);
                strcpy(nsection, zb.c_str());
            } else {
                strcpy(nsection, section);
            }
        }
        if (___child_count(cm)) {
            if (!((!strncasecmp(cm->type.c_str(), "message/", 8))
                    && (!strncasecmp(((mail_parser_mime_engine *)(cm->child))->type.c_str(), "multipart/", 11)))) {
                strcat(nsection, ".0");
            }
        }
        ___mime_section(parser, cm, nsection);
    }
}

void mime_set_imap_section(mail_parser_engine * parser)
{
    if (parser->section_flag) {
        return;
    }
    parser->section_flag = 1;
    ___mime_section(parser, parser->top_mime_engine, const_cast<char *>("0"));
}

}
