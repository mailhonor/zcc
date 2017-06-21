/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-11
 * ================================
 */

#include "zcc.h"

static int enable_att = 0;

static void ___usage(char *parameter)
{
    printf("USAGE: %s -f eml_filename [ eml_fn2 ... ] [-att ] \n", zcc::var_progname);
    exit(1);
}

static char *name_char_validate(char *abc)
{
    char key[] = "?<>\"'|/\\*", *p = abc, ch;

    while((ch=*p)) {
        if (strchr(key, ch)) {
            *p = ' ';
        }
        p++;
    }
    return abc;

}

static int save_att_tnef(zcc::tnef_parser * parser, zcc::tnef_parser_mime * mime, int i)
{
    const char *sname;
    char tmpname[256];

    sname = mime->show_name();
    if (zcc::empty(sname)) {
        sprintf(tmpname, "atts/tnef_unknown_%d.dat", i);
    } else {
        snprintf(tmpname, 255, "atts/tnef_%s", sname);
        name_char_validate(tmpname+5);
    }
    printf("save tnef attachment %s\n", tmpname);
    if (zcc::file_put_contents(tmpname, parser->data() + mime->body_offset(), mime->body_size()) < 0) {
        printf("ERR save_att_tnef save %m\n");
    }

    return 0;
}

static int save_att(zcc::mail_parser * parser, zcc::mail_parser_mime * mime, int i)
{
    const char *sname;
    char tmpname[256];

    sname = mime->show_name();
    if (zcc::empty(sname)) {
        sprintf(tmpname, "atts/unknown_%d.dat", i);
    } else {
        snprintf(tmpname, 255, "atts/%s", sname);
        name_char_validate(tmpname+5);
    }
    std::string dcon;
    mime->decoded_content(dcon);

    printf("save attachment %s\n", tmpname);
    if (zcc::file_put_contents(tmpname, dcon.c_str(), dcon.size()) < 0) {
        printf("ERR decode_mime_body: save %m\n");
    }

    if (mime->tnef()) {
        int j = 0;
        zcc::tnef_parser tp;
        zcc::tnef_parser_mime *m;
        tp.parse(dcon.c_str(), dcon.size());
        const std::vector<zcc::tnef_parser_mime *> &ams = tp.all_mimes();
        std::vector<zcc::tnef_parser_mime *>::const_iterator it;
        for (it = ams.begin(); it != ams.end(); it++) {
            m = *it;
            save_att_tnef(&tp, m, j + 1);
            j++;
        };
    }
    return 0;
}

static int save_all_attachments(zcc::mail_parser &parser)
{
    int i = 0;
    const std::vector<zcc::mail_parser_mime *> &allm = parser.attachment_mimes();
    std::vector<zcc::mail_parser_mime *>::const_iterator it;
    zcc::mail_parser_mime *m;
    for (it = allm.begin(); it != allm.end(); it++) {
        i++;
        m = *it;
        save_att(&parser, m, i);
    }

    return 0;
}

static void do_parse(char *eml_fn)
{
    zcc::file_mmap reader;
    if (!reader.mmap(eml_fn)) {
        printf("ERR open %s (%m)\n", eml_fn);
        exit(1);
    }

    zcc::mail_parser parser;
    parser.parse(reader.data(), reader.size());
    parser.debug_show();

    if (enable_att) {
        save_all_attachments(parser);
    }
}

int main(int argc, char **argv)
{
    std::vector<char *> fn_vec;
    zcc_main_parameter_begin() {
        if (!strcmp(optname, "-att")) {
            enable_att = 1;
            opti+=1;
            continue;
        }
        if (!optval) {
            ___usage(0);
        }
        if (!strcmp(optname, "-f")) {
            if (optval_count < 1) {
                ___usage(optname);
            }
            int i;
            for (i=0;i<optval_count;i++) {
                fn_vec.push_back(argv[opti + 1 + i]);
            }
            opti += 1 + optval_count;
            continue;
        }
    } zcc_main_parameter_end;

    if (fn_vec.size() < 1) {
        ___usage(0);
    }

    for (std::vector<char *>::iterator it = fn_vec.begin(); it != fn_vec.end(); it++) {
        do_parse(*it);
    }

    return 0;
}
