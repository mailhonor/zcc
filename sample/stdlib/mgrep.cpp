/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2018-06-07
 * ================================
 */

#include "zcc.h"

static void ___usage()
{
    printf("USAGE: \n");
    printf("\t%s -words 'abc|def|sss|oppfs'-f filename\n", zcc::var_progname);
    printf("\t%s -word_file word_list_file -f filename\n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    char *attr;
    std::string words, dcon, matched;
    std::vector<std::string> word_vector;
    int offset;

    zcc::main_parameter_run(argc, argv);

    attr = zcc::default_config.get_str("words");
    if (zcc::empty(attr)) {
        attr = zcc::default_config.get_str("word_file");
        if (zcc::empty(attr)) {
            ___usage();
        }
        zcc::file_get_contents_sample(attr, words);
        word_vector = zcc::split(words.c_str(), "\r\n");
    } else {
        word_vector = zcc::split(attr, "|");
    }
    if (word_vector.empty()) {
        ___usage();
    }


    attr = zcc::default_config.get_str("f");
    if (zcc::empty(attr)) {
        ___usage();
    }
    zcc::file_get_contents_sample(attr, dcon);


    zcc::mgrep mgrep;
    for(auto it = word_vector.begin(); it != word_vector.end(); it++) {
        mgrep.add_token(*it);
    }
    mgrep.add_token_over();

    for (int i=0;i<10;i++) {
    offset = mgrep.match(matched, dcon.data(), dcon.size());
    }
    if (offset < 0) {
        printf("NOT FOUND\n");
    } else {
        printf("FOUND: offset=%d, token=%s\n", offset, matched.c_str());
    }

    return 0;
}
