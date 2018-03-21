/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

namespace zcc
{

static const int var_redis_slot_count = 16384;

/* static */ void ___redis_msg_printf(const std::string &msgbuf, const char *fmt, ...)
{
    std::string &mf = (std::string &)msgbuf;
    char buf[1024 + 1];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 1024, fmt, ap);
    va_end(ap);
    mf = buf;
}

static void ___redis_msg_puts(const std::string &msgbuf, const char *msg)
{
    std::string &mf = (std::string &)msgbuf;
    mf = msg;
}

redis_client_query::redis_client_query(const char *_cmd)
{
    number_ret = 0;
    string_ret = 0;
    list_ret = 0;
    json_ret = 0;
    timeout_millisecond = 0;
    req_data = 0;
    req_len = 0;
    msg_info = 0;
    cmd = _cmd;
    flag_write = false;
}

redis_client_query::~redis_client_query()
{
}

int redis_client_basic_engine::query_protocol_io(redis_client_query &query, stream &fp)
{
    std::string rstr;
    char *rp, crlf[3];
    int rlen, num, numi, num2,firstch;

    if (query.number_ret) {
        *(query.number_ret) = -1;
    }
    if (query.string_ret) {
        query.string_ret->clear();
    }
    if (query.list_ret) {
        query.list_ret->clear();
    }
    if (query.json_ret) {
        query.json_ret->reset();
    }
    if ((!query.req_data)) {
        return -1;
    }
    fp.write(query.req_data, query.req_len?query.req_len:strlen(query.req_data));
    if (fp.gets(rstr) < 3) {
        return -2;
    }
    rp = (char *)rstr.c_str();
    rlen = (int)rstr.size() - 2;
    rp[rlen] = 0;
    firstch = rp[0];
    if (firstch == '-') {
        if (query.msg_info) {
            (*query.msg_info) = rp;
        }
        return -1;
    }
    if (firstch == '+') {
        if (query.msg_info) {
            (*query.msg_info) = rp;
        }
        return 1;
    }
    if (firstch == '*') {
        num = atoi(rp + 1);
        if (num <1) {
            return 1;
        }
        for (numi = 0; numi < num ;numi++) {
            if (fp.gets(rstr) < 3) {
                if (query.msg_info) {
                    (*query.msg_info) =  "ERR the length of response < 3";
                }
                return -2;
            }
            rp = (char *)rstr.c_str();
            rlen = (int)rstr.size() - 2;
            rp[rlen] = 0;
            firstch = rp[0];
            if (firstch  == '*') {
                int tmp = atoi(rp + 1);
                if (tmp < 1) {
                    if (query.list_ret) {
                        query.list_ret->push_back("");
                    }
                } else {
                    num += tmp;
                }
                continue;
            }
            if (firstch == ':') {
                if (query.list_ret) {
                    query.list_ret->push_back(rp+1);
                }
                continue;
            }
            if (firstch == '+') {
                continue;
            }
            if (firstch != '$') {
                if (query.msg_info) {
                    (*query.msg_info)  = "ERR the initial of response shold be $";
                }
                return -2;
            }
            num2 = atoi(rp+1);
            std::string tmpstr;
            if (num2 >0){
                fp.readn(tmpstr, num2);
            }
            if (num2 > -1) {
                fp.readn(crlf, 2);
            }
            if (fp.is_exception()) {
                if (query.msg_info) {
                    (*query.msg_info) = "ERR redis read/write";
                }
                return -2;
            }
            if (query.list_ret) {
                query.list_ret->push_back(tmpstr);
            }
        }
        return 1;
    }

    if (firstch == ':') {
        long l = atol(rp + 1);
        if (query.number_ret) {
            *(query.number_ret) = l;
            return 1;
        }
        return (l>0?1:0);
    }
    if (firstch == '$') {
        num = atoi(rp+1);
        if (query.number_ret) {
            *(query.number_ret) = atol(rp+1);
        }
        if (num < 0) {
            return 0;
        }
        if (num >0){
            if (query.string_ret) {
                fp.readn(*(query.string_ret), num);
            } else {
                fp.readn(0, num);
            }
        }
        fp.readn(crlf, 2);
        if (fp.is_exception()) {
            if (query.msg_info) {
                (*query.msg_info)  = "ERR redis read/write";
            }
            return -2;
        }
        return 1;
    }
    if (query.msg_info) {
        (*query.msg_info) = "ERR redis read/write";
    }
    return -2;
}

int redis_client_basic_engine::query_protocol(redis_client_query &query)
{
    return -1;
}

int redis_client_basic_engine::timed_wait_readable(long timeout_millisecond)
{
    return 1;
}

class redis_client_standalone_engine: public redis_client_basic_engine
{
public:
    redis_client_standalone_engine(const char *_destination, const char *_password);
    ~redis_client_standalone_engine();
    int query_protocol(redis_client_query &query);
private:
    void open();
    void close();
    std::string destination;
    std::string password;
    int fd;
};

redis_client_standalone_engine::redis_client_standalone_engine(const char *_destination, const char *_password)
{
    destination = _destination;
    password = _password;
    fd = -1;
}

redis_client_standalone_engine::~redis_client_standalone_engine()
{
    close();
}

int redis_client_standalone_engine::query_protocol(redis_client_query &query)
{
    if (fd == -1) {
        open();
    }
    if (fd == -1) {
        return -2;
    }
    stream fp(fd);
    int ret = query_protocol_io(query, fp);
    fp.close();
    if (ret == -2) {
        if (fd > -1) {
            ::close(fd);
            fd = -1;
        }
    }
    return ret;
}

void redis_client_standalone_engine::open()
{
    close();
    std::list<std::string> netpath_list;
    netpaths_expand(destination.c_str(), netpath_list);
    std_list_walk_begin(netpath_list, np) {
        while (fd == -1) {
            char *npp = (char *)(np.c_str());
            char *p = strchr(npp, ':');
            if (p) {
                *p++ = 0;
                fd = inet_connect(npp, atoi(p));
            } else {
                fd = unix_connect(npp);
            }
            if (fd == -1) {
                break;
            }
            break;
        }
    } std_list_walk_end;

    if (!password.empty()) {
    }
}

void redis_client_standalone_engine::close()
{
    if (fd > -1) {
        ::close(fd);
        fd = -1;
    }
}

#if 0
class redis_client_engine_node
{
public:
    redis_client_engine_node(int concurrency);
    ~redis_client_engine_node();
    void close();
    unsigned int last_access;
    int addr_int;
    int port;
    int *fds;
    short int slot_count;
    short int slave_count;
    redis_client_engine_node *next;
    redis_client_engine_node *prev;
    bool is_damaged;
    bool master_detact;
    bool is_master;
};

redis_client_engine_node::redis_client_engine_node(int concurrency)
{
    last_access = 0;
    addr_int = 0;
    port = 0;
    slot_count = 0;
    slot_count = 0;
    next = 0;
    prev = 0;
    is_damaged = false;
    master_detact = false;
    is_master = false;
    is_standalone = true;
    fds = (int *)calloc(concurrency, sizeof(int));
    for (int i = 0; i < concurrency; i++) {
        fds[i] = -1;
    }
}

redis_client_engine_node::~redis_client_engine_node()
{
    close();
}

void redis_client_engine_node::close()
{
}

class redis_client_engine_cluster_info
{
public:
    redis_client_engine_cluster_info();
    ~redis_client_engine_cluster_info();
    redis_client_engine_node *node_vector[var_redis_slot_count];
    std::map<unsigned long, redis_client_engine_node *> node_tree;
}
redis_client_engine_cluster_info::redis_client_engine_cluster_info()
{
    memset(node_vector, 0, var_redis_slot_count * sizeof(redis_client_engine_node *));
}

redis_client_engine_cluster_info::~redis_client_engine_cluster_info()
{
    std::map<unsigned long, redis_client_engine_node *>::iterator it;
    while(1) {
        it = _node_tree.begin();
        if (it == node_tree.end()) {
            break;
        }
        delete it->second;
    }
}

class redis_client_engine
{
public:
    redis_client_engine();
    ~redis_client_engine(); 
    void close();
    connect
    void require_slot(redis_cmd_args &args);
    void active_slot(int slot,  redis_client_engine_node_t * en);
    void detact_slot(redis_cmd_args &args);
    int query_do(redis_cmd_args &args, long *nval, std::string *sval, std::list<std::string> *lval, json *jval);
    int ping_every_minute(int slot = -1);
    const std::string &get_msg();
    int _query_do(redis_cmd_args &args);
    int _sort(const char *key, const char *by, long offset, long count, const std::list<std::string> *get, bool desc, bool alpha, std::list<std::string> *lval, const char *store, long *nval);
    int _scan(const char *cmd, std::list<std::string> &result, const char *key, size_t klen, long &cursor, const char *pattern, size_t plen, long count);
    int _blpop(std::string &key_return, std::string &val_return, const char *cmd, long timeout, const char *redis_fmt, ...);
    bool opened;
    bool cluster_mode;
    bool exception;
    std::string msg;
    std::string destination;
    std::string password;
    redis_client_engine_node *standalone_node;
    redis_client_engine_cluster_info * cluster_node_info;
};

redis_client_engine::redis_client_engine()
{
    opened = false;
    exception = false;
    cluster_mode = false;
    alone_fd = -1;
    alone_addr = 0;
    alone_port = 0;
    alone_db_idx = 0;
    alone_last_access = 0;
    standalone_node = 0;
    cluster_info = 0;
}

redis_client_engine::~redis_client_engine()
{
    close();
}

void redis_client_engine::open()
{
    std::list<std::string> netpath_list;
    netpaths_expand(destination, netpath_list);
    std_list_walk_begin(netpath_list, np) {
        while (r_engine->alone_fd == -1) {
            int fd;
            char *npp = (char *)(np.c_str());
            char *p = strchr(npp, ':');
            if (p) {
                *p++ = 0;
                fd = inet_connect(npp, atoi(p));
            } else {
                fd = unix_connect(npp);
            }
            if (fd == -1) {
                break;
            }
            r_engine->alone_fd = fd;
            if (p && (netpath_list.size() > 1)) {
                if (ping() < 1) {
                    r_engine->close(-1);
                }
            }
            break;
        }
    } std_list_walk_end;

    if (r_engine->alone_fd == -1) {
        r_msg = "connection or ping error for";
        r_msg += destination;
    }
}

void redis_client_engine::close()
{
    if (!opened) {
        return;
    }

    opened = false;
    if (standalone_node)  {
        delete standalone_node;
        standalone_node = 0;
    }
    if (cluster_info) {
        delete cluster_info;
        cluster_info = 0;
    }

    msg.clear();
    cluster_mode = false;
    exception = false;
}

void redis_client_engine::require_slot(redis_cmd_args &args)
{
    struct timeval tv;
    gettimeofday(&tv, 0); tv.tv_usec;
    long rand_number = tv.tv_usec;
    int i, idx;

    while(1) {
        /* lock */
        if (!cluster_mode) {
            for (i = 0; i< concurrency; i++) {
                if (standalone_node.fds = 
            }
        } else {
        }
        /* unlock */
    }
}

void redis_client_engine::active_slot(int slot,  redis_client_engine_node_t * en)
{
    if (!en) {
        return;
    }
    if (en->fd == -1) {
        char *p = (char *)strchr(en->destination, ':');
        if (p) {
            int ch = *p;
            *p = 0;
            en->fd = zcc::inet_connect(en->destination, atoi(p+1));
            *p = ch;
        } else {
            en->fd = zcc::unix_connect(en->destination);
        }
        if (en->fd == -1) {
            return;
        }
    } else {
        /* ping */
    }
}

void redis_client_engine::detact_slot(redis_cmd_args &args)
{
    redis_client_engine_node_t *enode;
    if (args.a_slot == -1) {
        if (!args.a_key.empty()) {
            args.a_slot = get_crc16_result(args.a_key.c_str(), args.a_key.size());
            enode = cluster_info->node_vector[args.a_slot];
            if (enode) {
                active_slot(args.a_slot, enode);
            }
        }
        if ((args.a_slot==-1) || (!cluster_info->node_vector[args.a_slot])) {
            args.a_slot = -1;
        }
    } else {
        enode = cluster_info->node_vector[args.a_slot];
        if (!enode) {
            rbtree_node_t *rbn;
            redis_client_engine_node_t vn;
            strcpy(vn.destination, args.a_slot_destination.c_str());
            rbn= rbtree_find(&(cluster_info->node_tree), &(vn.rbnode_ipp));
            if (rbn) {
                enode = zcc_container_of(rbn, redis_client_engine_node_t, rbnode_ipp);
            }
        }
        if (enode) {
            if (!strcmp(enode->destination, args.a_slot_destination.c_str())) {
                enode->count++;
            } else {
                if (!enode->damaged) {
                    rbtree_detach(&(cluster_info->node_tree), &(enode->rbnode_ipp));
                    enode->damaged = true;
                    if (enode->fd > -1) {
                        ::close(enode->fd);
                        enode->fd = -1;
                    }
                }
                enode->count --;
                if (enode->count == 0) {
                    free(enode);
                }
                enode = cluster_info->node_vector[args.a_slot] = 0;
            }
        }
        if (!enode) {
            enode = (redis_client_engine_node_t *)calloc(1, sizeof(redis_client_engine_node_t));
            cluster_info->node_vector[args.a_slot] = enode;
            enode->fd = -1;
            enode->count = 1;
            enode->damaged =false;
            strcpy(enode->destination, args.a_slot_destination.c_str());
        }
        active_slot(args.a_slot, enode);
    }
}


int redis_client::query_do(redis_cmd_args &args, long *nval, std::string *sval, std::list<std::string> *lval, json *jval)
{
    int ret;
    if (r_engine == 0) {
        ___redis_msg_puts(r_msg, "ERR not open some-redis-address yet");
        return -2;
    }
    args.a_nval = nval;
    args.a_sval = sval;
    args.a_lval = lval;
    args.a_jval = jval;
    args.a_slot = -2;
    args.n_slot = -2;
    args.n_key_slot = -1;

    require_slot(args);

    if (!r_engine->cluster_mode) {
        if (r_engine->alone_fd == -1) {
            open(0);
        }
        if (r_engine->alone_fd == -1) {
            return -2;
        }
        ret = _query_do(args);
        if (!r_engine->cluster_mode) {
            return (ret==-9)?-1:ret;
        }
    }
    if (r_engine->cluster_info == 0) {
        r_engine->cluster_info = (redis_client_engine_cluster_info_t *)calloc(1, sizeof(redis_client_engine_cluster_info_t));
        rbtree_init(&(r_engine->cluster_info->node_tree), ___redis_client_engine_node_tree_cmp);
    }

    r_engine->detact_slot(args);
    ret = _query_do(args);
    if (ret == -9) {
        ret = _query_do(args);
        return (ret==-9)?-1:ret;
    }
    return ret;
}
#endif

}
