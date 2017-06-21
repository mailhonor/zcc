/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-10
 * ================================
 */

#include "zcc.h"

namespace zcc
{

static inline int ___child_count(mail_parser_mime * mime)
{
    int count;

    count = 0;
    for (mime = ((mail_parser_mime_t *)mime)->child; mime; mime = ((mail_parser_mime_t *)mime)->next) {
        count++;
    }

    return count;
}

void ___mime_section(mail_parser_t * parser, mail_parser_mime_t * mime, mail_parser_mime *mimew, char *section)
{
    mem_pool &mp = *(parser->mpool);
    int i, count;
    char nsection[10240], intbuf[16];
    mail_parser_mime *cmw, *fmw;
    mail_parser_mime_t *cm, *fm;
    argv zav;
    ZCC_STACK_STRING(zb, 10240);

    mime->imap_section = mp.strdup(section);

    zav.split(section, ".");
    count = ___child_count(mimew);
    fmw = mime->child;
    fm = (mail_parser_mime_t *)fmw;
    cmw = fmw;
    for (i = 0; i < count; i++, cmw = cm->next) {
        cm = (mail_parser_mime_t *)cmw;
        if (!strcmp(section, "")) {
            sprintf(nsection, "%d", i + 1);
        } else if ((zav.size()> 1) && (strncasecmp(fm->type, "multipart/", 11))) {
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
        if (___child_count(cmw)) {
            if (!((!strncasecmp(cm->type, "message/", 8))
                        && (!strncasecmp(((mail_parser_mime_t *)(cm->child))->type, "multipart/", 11)))) {
                strcat(nsection, ".0");
            }
        }
        ___mime_section(parser, cm, cmw, nsection);
    }
}

void mime_set_imap_section(mail_parser_t * parser)
{
    if (parser->section_flag) {
        return;
    }
    parser->section_flag = 1;
    ___mime_section(parser, (mail_parser_mime_t *)(parser->top_mime), parser->top_mime, const_cast<char *>("0"));
}

}
