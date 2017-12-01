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
    printf("USAGE: %s eml_filename [ eml_fn2 ... ] [--att ] \n", zcc::var_progname);
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
        tp.parse(dcon.c_str(), dcon.size());
        const std::list<zcc::tnef_parser_mime *> &ams = tp.all_mimes();
        std_list_walk_begin(ams, m) {
            save_att_tnef(&tp, m, j + 1);
            j++;
        } std_list_walk_end;
    }
    return 0;
}

static int save_all_attachments(zcc::mail_parser &parser)
{
    int i = 0;
    const std::list<zcc::mail_parser_mime *> &allm = parser.attachment_mimes();
    std_list_walk_begin(allm, m) {
        i++;
        save_att(&parser, m, i);
    } std_list_walk_end;

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
    zcc::main_parameter_run(argc, argv);
    enable_att = zcc::default_config.get_bool("att", false);

    if (zcc::main_parameter_values.empty()) {
        ___usage(0);
    }
    std_vector_walk_begin(zcc::main_parameter_values, fn) {
        do_parse(fn);
    } std_vector_walk_end;

    return 0;
}
