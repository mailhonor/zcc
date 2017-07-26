/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-04-28
 * ================================
 */

#include "zcc.h"
#include "mime.h"

namespace zcc
{

/* tnef */
struct tnef_parser_mime_t {
    char *type;
    char *filename;
    char *filename_utf8;
    char *show_name;
    char *content_id;
    int body_offset;
    int body_len;

    /* relationship */
    tnef_parser_mime_t *all_last;
    tnef_parser_mime_t *all_next;
    tnef_parser_mime *wrap;

    /* */
    short int mime_type;
    tnef_parser_t *parser;
};

struct tnef_parser_t {
    char src_charset_def[32];
    vector<tnef_parser_mime *> *___all_mimes;
    mime_parser_cache_magic *mcm;
    /* */
    gm_pool *gmp;
    char *data_orignal;
    char *tnef_data;
    char *tnef_pos;
    int tnef_size;
};


typedef struct ___mime_list_t ___mime_list_t;
struct ___mime_list_t {
    tnef_parser_mime_t *head;
    tnef_parser_mime_t *tail;
};

static int mime_list_add(tnef_parser_t * parser, ___mime_list_t * mime_list)
{
    tnef_parser_mime *mimew = new(parser->gmp->calloc(1, sizeof(tnef_parser_mime)))tnef_parser_mime(parser);
    tnef_parser_mime_t *mime = ((tnef_parser_mime_t **)mimew)[0];
    zcc_mlink_append(mime_list->head, mime_list->tail, mime, all_last, all_next);
    mime->parser = parser;
    return 0;
}

static int mime_list_pop(___mime_list_t * mime_list, tnef_parser_mime_t ** mime)
{
    *mime = mime_list->tail;
    zcc_mlink_detach(mime_list->head, mime_list->tail, mime_list->tail, all_last, all_next);
    if (*mime) {
        return 1;
    }
    return 0;
}

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

static int ___mime_decode_tnef(tnef_parser_t * parser, ___mime_list_t * mime_list);

#define ___LEFT(parser) 	((parser)->tnef_size - ((parser)->tnef_pos - (parser)->tnef_data))

static inline int tnef_geti8(tnef_parser_t * parser)
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

static inline int tnef_geti16(tnef_parser_t * parser)
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

static inline int tnef_geti32(tnef_parser_t * parser)
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

static inline int tnef_getx(tnef_parser_t * parser, int value_len, char **value)
{
    if (___LEFT(parser) < value_len) {
        return -1;
    }

    *value = parser->tnef_pos;

    parser->tnef_pos += value_len;

    return 0;
}

static int tnef_decode_fragment(tnef_parser_t * parser, int *attribute, char **value, int *value_len)
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

static int tnef_decode_message(tnef_parser_t * parser, ___mime_list_t * mime_list)
{
    int ret;
    int attribute;
    char *val;
    int val_len;

    ret = tnef_decode_fragment(parser, &attribute, &val, &val_len);

    return ret;
}

static int extract_gridi_attrs(tnef_parser_t * parser, ___mime_list_t * mime_list)
{
    int att_type, att_name;
    char *val;
    int val_len;
    tnef_parser_mime_t *cmime;
    tnef_parser_t parser2;

    cmime = mime_list->tail;

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
                if (cmime->filename) {
#if 0
                    parser->gmp->free(cmime->filename);
#endif
                }
                cmime->filename = parser->gmp->memdupnull(val, val_len);
            }
            break;

        case TNEF_GRIDI_ATTACH_MIME_TAG:   // Is this ever set, and what is format?
            if (val && cmime && (!cmime->type)) {
                cmime->type = parser->gmp->memdupnull(val, val_len);
            }
            break;

        case TNEF_GRIDI_ATTACH_DATA:
            memcpy(&parser2, parser, sizeof(tnef_parser_t));
            parser2.tnef_data = val;
            parser2.tnef_pos = val;
            parser2.tnef_size = val_len;
            if (tnef_getx(&parser2, 16, &val) == -1) {
                return -1;
            }
            cmime = 0;
            if (mime_list_pop(mime_list, &cmime)) {
                if (cmime) {
                    cmime->wrap->~tnef_parser_mime(); /* free */
                }
            }
            cmime = 0;

            parser2.data_orignal = parser->data_orignal;
            parser2.tnef_data = val;
            parser2.tnef_pos = val;
            parser2.tnef_size = val_len;
            if (___mime_decode_tnef(&parser2, mime_list) == -1) {
                return -1;
            }
            break;
        case TNEF_GRIDI_ATTACH_CID:
            if (val && cmime && (!cmime->content_id)) {
                cmime->content_id = parser->gmp->memdupnull(val, val_len);
            }
            break;

        default:
            break;
        }

    }

    return 0;
}

static int tnef_decode_attachment(tnef_parser_t * parser, ___mime_list_t * mime_list)
{
    int ret;
    int attribute;
    char *val;
    int val_len;
    tnef_parser_mime_t *cmime;
    tnef_parser_t parser2;

    ret = tnef_decode_fragment(parser, &attribute, &val, &val_len);
    if (ret < 0) {
        return -1;
    }

    cmime = mime_list->tail;
    switch (attribute) {
    case TNEF_ARENDDATA:
        mime_list_add(parser, mime_list);
        break;
    case TNEF_AFILENAME:
        if (cmime && (!cmime->filename)) {
            cmime->filename = parser->gmp->memdupnull(val, val_len);
        }
        break;
    case TNEF_ATTACHDATA:
        if (cmime) {
            cmime->body_len = val_len;
            cmime->body_offset = val - (parser->data_orignal);
        }
        break;
    case TNEF_AGRIDIATTRS:
        memcpy(&parser2, parser, sizeof(tnef_parser_t));
        parser2.tnef_data = val;
        parser2.tnef_pos = val;
        parser2.tnef_size = val_len;
        if (extract_gridi_attrs(&parser2, mime_list) == -1) {
            return -1;
        }
        break;
    default:
        break;
    }

    return 0;
}

static int ___mime_decode_tnef(tnef_parser_t * parser, ___mime_list_t * mime_list)
{
    int ret;
    int signature, type;

    signature = tnef_geti32(parser);
    if (signature != TNEF_SIGNATURE) {
        return -1;
    }
    tnef_geti16(parser);

    while (___LEFT(parser)) {
        type = tnef_geti8(parser);
        ret = 0;
        if (type == TNEF_LVL_MESSAGE) {
            ret = tnef_decode_message(parser, mime_list);
        } else if (type == TNEF_LVL_ATTACHMENT) {
            ret = tnef_decode_attachment(parser, mime_list);
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
tnef_parser_mime::tnef_parser_mime(tnef_parser_t *parser)
{
    ___data = (tnef_parser_mime_t *)parser->gmp->calloc(1, sizeof(tnef_parser_mime_t));
    ___data->parser = parser;
    ___data->wrap = this;
    ___data->type = blank_buffer;
    ___data->filename = blank_buffer;
    ___data->content_id = blank_buffer;
}

tnef_parser_mime::~tnef_parser_mime()
{
#if 0
    gm_pool &gmp = *(___data->parser->mpool);
    gmp->free(___data.type);
    gmp->free(___data.filename);
    gmp->free(___data.filename_utf8);
    gmp->free(___data.content_id);
#endif
}

const char *tnef_parser_mime::type()
{
    return ___data->type;
}

const char *tnef_parser_mime::show_name()
{
    if (!___data->show_name) {
        char *n = const_cast<char *>(filename_utf8());
        if(n == blank_buffer) {
            n = const_cast<char *>(filename());
        }
        ___data->show_name = n;
    }
    return ___data->show_name;
}

const char *tnef_parser_mime::filename()
{
    return ___data->filename;
}

const char *tnef_parser_mime::filename_utf8()
{
    if (!___data->filename_utf8) {
        mime_parser_cache_magic mcm(*(___data->parser->mcm));
        std::string &uname = mcm.require_string();
        mcm.true_data = ___data->filename;
        mime_header_line_get_utf8(___data->parser->src_charset_def,(char *)(&mcm), strlen(___data->filename), uname);
        ___data->filename_utf8 = ___data->parser->gmp->memdupnull(uname.c_str(), uname.size());
    }
    return ___data->filename_utf8;
}

const char *tnef_parser_mime::content_id()
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
tnef_parser::tnef_parser()
{
    gm_pool *gmp = new gm_pool();
    ___data = (tnef_parser_t *)gmp->calloc(1, sizeof(tnef_parser_t));
    ___data->gmp = gmp;
    ___data->___all_mimes=new(gmp->calloc(1,sizeof(vector<tnef_parser_mime*>)))vector<tnef_parser_mime*>();
    ___data->mcm = new(gmp->calloc(1, sizeof(mime_parser_cache_magic)))mime_parser_cache_magic();
    ___data->mcm->gmp = gmp;
}

tnef_parser::~tnef_parser()
{
    gm_pool *gmp = ___data->gmp;
    zcc_vector_walk_begin(*(___data->___all_mimes), m) {
        m->~tnef_parser_mime();
        /* gmp->free(m); */
    } zcc_vector_walk_end;
    ___data->___all_mimes->~vector<tnef_parser_mime *>(); /* free */
    /* gmp->free(___data); */

    ___data->mcm->~mime_parser_cache_magic(); /* free */
    delete gmp;
}

void tnef_parser::option_src_charset_def(const char *src_charset_def)
{
    if (src_charset_def) {
        snprintf(___data->src_charset_def, 31, "%s", src_charset_def);
    }
}

void tnef_parser::parse(const char *mail_data, size_t mail_data_len)
{
    tnef_parser_t *parser = ___data;
    ___mime_list_t mime_list;
    tnef_parser_mime_t *m;

    parser->data_orignal = const_cast<char *>(mail_data);
    parser->tnef_data = const_cast<char *>(mail_data);
    parser->tnef_pos = const_cast<char *>(mail_data);
    parser->tnef_size = mail_data_len;

    /* mime chain */
    memset(&mime_list, 0, sizeof(___mime_list_t));
    mime_list.head = 0;
    mime_list.tail = 0;

    if (___mime_decode_tnef(parser, &mime_list) < 0) {
        return;
    }

    for (m = mime_list.head; m; m = m->all_next) {
        ___data->___all_mimes->push_back(m->wrap);
    }
}

const char *tnef_parser::data()
{
    return ___data->tnef_data;
}

size_t tnef_parser::size()
{
    return ___data->tnef_size;
}

const vector<tnef_parser_mime *> &tnef_parser::all_mimes()
{
    return (*(___data->___all_mimes));
}

}
