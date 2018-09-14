/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2016-04-28
 * ================================
 */

#include "zcc.h"

namespace zcc
{

/* tnef */
class tnef_parser_mime_engine
{
public:
    tnef_parser_mime_engine(tnef_parser_engine *_parser);
    ~tnef_parser_mime_engine();
    unsigned char filename_utf8_flag:1;
    unsigned char show_name_flag:1;
    std::string type;
    std::string filename;
    std::string filename_utf8;
    std::string *show_name;
    std::string content_id;
    int body_offset;
    int body_len;

    /* relationship */
    tnef_parser_mime *wrap;

    /* */
    tnef_parser_engine *parser;
};

class tnef_parser_engine
{
public:
    tnef_parser_engine();
    ~tnef_parser_engine();
    std::string src_charset_def;
    std::list<tnef_parser_mime_engine *> all_mimes_engine;
    std::list<tnef_parser_mime *> all_mimes;
    /* */
    char *data_orignal;
    char *tnef_data;
    char *tnef_pos;
    int tnef_size;
};

/* ################################################################## */

#define TNEF_SIGNATURE 			 0x223e9f78
#define TNEF_LVL_MESSAGE 		 0x01
#define TNEF_LVL_ATTACHMENT 		 0x02

#define TNEF_STRING 			 0x00010000
#define TNEF_TEXT 			 0x00020000
#define TNEF_BYTE 			 0x00060000
#define TNEF_WORD 			 0x00070000
#define TNEF_DWORD 		 	 0x00080000

#define TNEF_ASUBJECT 			 0x8004|TNEF_DWORD
#define TNEF_AMCLASS 			 0x8008|TNEF_WORD
#define TNEF_BODYTEXT	 		 0x800c|TNEF_TEXT
#define TNEF_ATTACHDATA 		 0x800f|TNEF_BYTE
#define TNEF_AFILENAME 			 0x8010|TNEF_STRING
#define TNEF_ARENDDATA 			 0x9002|TNEF_BYTE
#define TNEF_AGRIDIATTRS 		 0x9005|TNEF_BYTE
#define TNEF_AVERSION 			 0x9006|TNEF_DWORD

#define TNEF_GRIDI_NULL 			 0x0001
#define TNEF_GRIDI_SHORT 		 0x0002
#define TNEF_GRIDI_INT 			 0x0003
#define TNEF_GRIDI_FLOAT 		 0x0004
#define TNEF_GRIDI_DOUBLE 		 0x0005
#define TNEF_GRIDI_CURRENCY 		 0x0006
#define TNEF_GRIDI_APPTIME 		 0x0007
#define TNEF_GRIDI_ERROR 		 0x000a
#define TNEF_GRIDI_BOOLEAN 		 0x000b
#define TNEF_GRIDI_OBJECT 		 0x000d
#define TNEF_GRIDI_INT8BYTE 		 0x0014
#define TNEF_GRIDI_STRING 		 0x001e
#define TNEF_GRIDI_UNICODE_STRING 	 0x001f
#define TNEF_GRIDI_SYSTIME 		 0x0040
#define TNEF_GRIDI_CLSID 		 0x0048
#define TNEF_GRIDI_BINARY 		 0x0102

#define TNEF_GRIDI_ATTACH_MIME_TAG 	 0x370E
#define TNEF_GRIDI_ATTACH_LONG_FILENAME 	 0x3707
#define TNEF_GRIDI_ATTACH_DATA 		 0x3701
#define TNEF_GRIDI_ATTACH_CID 		 0x3712

static int ___mime_decode_tnef(tnef_parser_engine * parser, std::list<tnef_parser_mime_engine *> &mime_engine_list);

#define ___LEFT(parser) 	((parser)->tnef_size - ((parser)->tnef_pos - (parser)->tnef_data))

static inline int tnef_geti8(tnef_parser_engine * parser)
{
    int v;
    unsigned char *p;

    if (___LEFT(parser) < 1) {
        return -1;
    }

    p = (unsigned char *)(parser->tnef_pos);
    v = p[0];

    parser->tnef_pos += 1;

    return v;
}

static inline int tnef_geti16(tnef_parser_engine * parser)
{
    int v;
    unsigned char *p;

    if (___LEFT(parser) < 2) {
        return -1;
    }

    p = (unsigned char *)(parser->tnef_pos);
    v = p[0] + (p[1] << 8);

    parser->tnef_pos += 2;

    return v;
}

static inline int tnef_geti32(tnef_parser_engine * parser)
{
    int v;
    unsigned char *p;

    if (___LEFT(parser) < 4) {
        return -1;
    }

    p = (unsigned char *)(parser->tnef_pos);
    v = p[0] + (p[1] << 8) + (p[2] << 16) + (p[3] << 24);

    parser->tnef_pos += 4;

    return v;
}

static inline int tnef_getx(tnef_parser_engine * parser, int value_len, char **value)
{
    if (___LEFT(parser) < value_len) {
        return -1;
    }

    *value = parser->tnef_pos;

    parser->tnef_pos += value_len;

    return 0;
}

static int tnef_decode_fragment(tnef_parser_engine * parser, int *attribute, char **value, int *value_len)
{
    if ((*attribute = tnef_geti32(parser)) == -1) {
        return -1;
    }

    if ((*value_len = tnef_geti32(parser)) == -1) {
        return -1;
    }

    if (tnef_getx(parser, *value_len, value) == -1) {
        return -1;
    }

    if (tnef_geti16(parser) == -1) {
        return -1;
    }

    return 0;
}

static int tnef_decode_message(tnef_parser_engine * parser, std::list<tnef_parser_mime_engine *> &mime_engine_list)
{
    int ret;
    int attribute;
    char *val;
    int val_len;

    ret = tnef_decode_fragment(parser, &attribute, &val, &val_len);

    return ret;
}

static int extract_mapi_attrs(tnef_parser_engine * parser, std::list<tnef_parser_mime_engine *> &mime_engine_list)
{
    int att_type, att_name;
    char *val;
    int val_len;
    tnef_parser_mime_engine *cmime;
    tnef_parser_engine parser2;

    if (mime_engine_list.empty()) {
        cmime = 0;
    } else {
        cmime = mime_engine_list.back();
    }

    /* number of attributes */
    if (tnef_geti32(parser) == -1) {
        return -1;
    }
    while (___LEFT(parser) > 0) {
        val = 0;
        val_len = 0;
        att_type = tnef_geti16(parser);
        att_name = tnef_geti16(parser);
        switch (att_type) {
        case TNEF_GRIDI_SHORT:
            if (tnef_getx(parser, 2, &val) == -1) {
                return -1;
            }
            break;
        case TNEF_GRIDI_INT:
        case TNEF_GRIDI_BOOLEAN:
        case TNEF_GRIDI_FLOAT:
            if (tnef_getx(parser, 4, &val) == -1) {
                return -1;
            }
            break;

        case TNEF_GRIDI_DOUBLE:
        case TNEF_GRIDI_SYSTIME:
            if (tnef_getx(parser, 8, &val) == -1) {
                return -1;
            }
            break;

        case TNEF_GRIDI_STRING:
        case TNEF_GRIDI_UNICODE_STRING:
        case TNEF_GRIDI_BINARY:
        case TNEF_GRIDI_OBJECT:
            {
                int num_vals = tnef_geti32(parser), i, length, buflen;
                if (num_vals == -1) {
                    return -1;
                }
                for (i = 0; i < num_vals; i++)  // usually just 1
                {
                    length = tnef_geti32(parser);
                    if (length == -1) {
                        return -1;
                    }
                    buflen = length + ((4 - (length % 4)) % 4); // pad to next 4 byte boundary
                    if (tnef_getx(parser, buflen, &val) == -1) {
                        return -1;
                    }
                    val_len = length;
                }
            }
            break;

        default:
            break;
        }
        switch (att_name) {
        case TNEF_GRIDI_ATTACH_LONG_FILENAME:  // used in preference to AFILENAME value
            if (val && cmime) {
                cmime->filename.clear();
                cmime->filename.append(val, val_len);
            }
            break;

        case TNEF_GRIDI_ATTACH_MIME_TAG:   // Is this ever set, and what is format?
            if (val && cmime && (!cmime->type.empty())) {
                cmime->type.append(val, val_len);
            }
            break;

        case TNEF_GRIDI_ATTACH_DATA:
            parser2.data_orignal = parser->data_orignal;
            parser2.tnef_data = val;
            parser2.tnef_pos = val;
            parser2.tnef_size = val_len;
            if (tnef_getx(&parser2, 16, &val) == -1) {
                return -1;
            }
            if (!mime_engine_list.empty()) {
                cmime = mime_engine_list.back();
                mime_engine_list.pop_back();
                delete cmime;
            }
            cmime = 0;

#if  0
            parser2.data_orignal = parser->data_orignal;
            parser2.tnef_data = val;
            parser2.tnef_pos = val;
            parser2.tnef_size = val_len;
#endif
            if (___mime_decode_tnef(&parser2, mime_engine_list) == -1) {
                return -1;
            }
            break;
        case TNEF_GRIDI_ATTACH_CID:
            if (val && cmime && (cmime->content_id.empty())) {
                cmime->content_id.append(val, val_len);
            }
            break;

        default:
            break;
        }

    }

    return 0;
}

static int tnef_decode_attachment(tnef_parser_engine * parser, std::list<tnef_parser_mime_engine *> &mime_engine_list)
{
    int ret;
    int attribute;
    char *val;
    int val_len;
    tnef_parser_mime_engine *cmime;
    tnef_parser_engine parser2;

    ret = tnef_decode_fragment(parser, &attribute, &val, &val_len);
    if (ret < 0) {
        return -1;
    }

    if (mime_engine_list.empty()) {
        cmime = 0;
    } else {
        cmime = mime_engine_list.back();
    }

    switch (attribute) {
    case TNEF_ARENDDATA:
        mime_engine_list.push_back(new tnef_parser_mime_engine(parser));
        break;
    case TNEF_AFILENAME:
        if (cmime && (cmime->filename.empty())) {
            cmime->filename.append(val, val_len);
        }
        break;
    case TNEF_ATTACHDATA:
        if (cmime) {
            cmime->body_len = val_len;
            cmime->body_offset = val - (parser->data_orignal);
        }
        break;
    case TNEF_AGRIDIATTRS:
        parser2.data_orignal = parser->data_orignal;
        parser2.tnef_data = val;
        parser2.tnef_pos = val;
        parser2.tnef_size = val_len;
        if (extract_mapi_attrs(&parser2, mime_engine_list) == -1) {
            return -1;
        }
        break;
    default:
        break;
    }

    return 0;
}

static int ___mime_decode_tnef(tnef_parser_engine * parser, std::list<tnef_parser_mime_engine *> &mime_engine_list)
{
    int ret;
    int signature, type;

    signature = tnef_geti32(parser);
    if (signature != TNEF_SIGNATURE) {
        return -1;
        if (parser->data_orignal == parser->tnef_data) {
            return -1;
        }
    }
    tnef_geti16(parser);

    while (___LEFT(parser)) {
        type = tnef_geti8(parser);
        ret = 0;
        if (type == TNEF_LVL_MESSAGE) {
            ret = tnef_decode_message(parser, mime_engine_list);
        } else if (type == TNEF_LVL_ATTACHMENT) {
            ret = tnef_decode_attachment(parser, mime_engine_list);
        } else {
            return -1;
        }
        if (ret < 0) {
            return -1;
        }
    }

    return 0;
}

/* ################################################################## */
tnef_parser_mime_engine::tnef_parser_mime_engine(tnef_parser_engine *_parser)
{
    filename_utf8_flag = 0;
    show_name_flag = 0;
    parser = _parser;
}

tnef_parser_mime_engine::~tnef_parser_mime_engine()
{
}


tnef_parser_mime::tnef_parser_mime(tnef_parser_mime_engine *engine)
{
    ___data = engine;
}

tnef_parser_mime::~tnef_parser_mime()
{
}

const std::string &tnef_parser_mime::type()
{
    return ___data->type;
}

const std::string& tnef_parser_mime::show_name()
{
    if (!___data->show_name_flag) {
        std::string *n = &(___data->filename_utf8);
        if (!___data->filename_utf8_flag) {
            filename_utf8();
        }
        if (n->empty()) {
            n = &(___data->filename);
        }
        ___data->show_name = n;
    }
    return *(___data->show_name);
}

const std::string &tnef_parser_mime::filename()
{
    return ___data->filename;
}

const std::string &tnef_parser_mime::filename_utf8()
{
    if (!___data->filename_utf8_flag) {
        mime_header_line_get_utf8(___data->parser->src_charset_def.c_str() ,___data->filename.c_str(), ___data->filename.size(), ___data->filename_utf8);
        ___data->filename_utf8_flag = 1;
    }
    return ___data->filename_utf8;
}

const std::string &tnef_parser_mime::content_id()
{
    return ___data->content_id;
}

size_t tnef_parser_mime::body_offset()
{
    return ___data->body_offset;
}

size_t tnef_parser_mime::body_size()
{
    return ___data->body_len;
}

/* ################################################################## */
tnef_parser_engine::tnef_parser_engine()
{
    data_orignal = 0;
    tnef_data = 0;
    tnef_pos = 0;
    tnef_size = 0;
}

tnef_parser_engine::~tnef_parser_engine()
{
}

tnef_parser::tnef_parser()
{
    ___data = new tnef_parser_engine();
}

tnef_parser::~tnef_parser()
{
    std_list_walk_begin(___data->all_mimes_engine, m) {
        delete m;
    } std_list_walk_end;

    std_list_walk_begin((___data->all_mimes), m) {
        delete m;
    } std_list_walk_end;
    delete ___data;
}

void tnef_parser::set_src_charset_def(const char *src_charset_def)
{
    ___data->src_charset_def = src_charset_def;
}

void tnef_parser::parse(const char *mail_data, size_t mail_data_len)
{
    tnef_parser_engine *parser = ___data;

    parser->data_orignal = const_cast<char *>(mail_data);
    parser->tnef_data = const_cast<char *>(mail_data);
    parser->tnef_pos = const_cast<char *>(mail_data);
    parser->tnef_size = mail_data_len;

    std::list<tnef_parser_mime_engine *> mime_engine_list;
    if (___mime_decode_tnef(parser, mime_engine_list) < 0) {
        return;
    }

    std_list_walk_begin(mime_engine_list, me) {
        ___data->all_mimes_engine.push_back(me);
        tnef_parser_mime *m = new tnef_parser_mime(me);
        ___data->all_mimes.push_back(m);
    } std_list_walk_end;
}

const char *tnef_parser::data()
{
    return ___data->tnef_data;
}

size_t tnef_parser::size()
{
    return ___data->tnef_size;
}

const std::list<tnef_parser_mime *> &tnef_parser::all_mimes()
{
    return (___data->all_mimes);
}

}
