#include <liburing.h>
#include <sys/types.h>
#include <unistd.h>

#include "http_parser.h"
#include "iohandler.h"
#include "iouring_utils.h"

#define QD (1024)

static void io_uring_co_http_handler(coroutine *co, void *data)
{
    struct server_connection *conn = (struct server_connection *)data;

    string_t *fresstr = NULL;
    struct http_request req;
    struct http_response res;
    struct kv_pair static_kv_pairs[60];

    req.headers = KV_INIT_WITH_BUF(&static_kv_pairs[0], 20);
    req.query = KV_INIT_WITH_BUF(&static_kv_pairs[20], 20);
    res.headers = KV_INIT_WITH_BUF(&static_kv_pairs[40], 20);

    if (UNLIKELY((res.str = string_init(1024)) == NULL)) {
        LOG_ERROR("failed to init string struct...\n");
        goto EXIT;
    }
    if (UNLIKELY((fresstr = string_init(2048)) == NULL)) {
        LOG_ERROR("failed to init string struct...\n");
        goto EXIT;
    }
    req.conn = conn;

    while (1) {
        int to_yield = 0;
        int64_t yielded = co->yielded;

        switch (conn->action) {
        case IO_URING_OP_RECV: {
            if (UNLIKELY(yielded < 0)) {
                LOG_ERROR("read error: %ld...\n", yielded);
                goto YIELD;
            }

            if (UNLIKELY(yielded == 0)) {
                LOG_INFO("remote peer closed\n");
                goto RESET;
            }

            // request entity too large
            if (UNLIKELY(yielded == (int64_t)conn->str->capacity)) {
                char buf[] = RESPONSE_413;
                io_uring_push_send((struct io_uring *)conn->svr->ring, conn->fd,
                                   buf, strlen(buf), (void *)conn, 0, 0);
                goto YIELD;
            }

            if (!http_parse_request(&req, conn->str->buf, yielded)) {
                char buf[] = RESPONSE_400;
                io_uring_push_send((struct io_uring *)conn->svr->ring, conn->fd,
                                   buf, strlen(buf), (void *)conn, 0, 0);
                goto YIELD;
            }

            const struct router *rt = NULL;
            if ((rt = find_router(req.path, req.method)) == NULL) {
                char buf[] = RESPONSE_404;
                io_uring_push_send((struct io_uring *)conn->svr->ring, conn->fd,
                                   buf, strlen(buf), (void *)conn, 0, 0);
                goto YIELD;
            }

            int handle_err =
                (rt->filepath)
                    ? ((int (*)(struct http_request *, struct http_response *,
                                const char *))rt->fp)(&req, &res, rt->filepath)
                    : ((int (*)(struct http_request *,
                                struct http_response *))rt->fp)(&req, &res);

            if (UNLIKELY(handle_err == HANDLER_ERR_FILE_NOT_FOUND)) {
                char buf[] = RESPONSE_404;
                io_uring_push_send((struct io_uring *)conn->svr->ring, conn->fd,
                                   buf, strlen(buf), (void *)conn, 0, 0);
                goto YIELD;
            }

            if (UNLIKELY(handle_err == HANDLER_ERR_GENERAL)) {
                char buf[] = RESPONSE_500;
                io_uring_push_send((struct io_uring *)conn->svr->ring, conn->fd,
                                   buf, strlen(buf), (void *)conn, 0, 0);
                goto YIELD;
            }

            if (http_compose_response(&req, &res, fresstr)) {
#ifdef WATCH_STATIC_FILES
                io_uring_push_send((struct io_uring *)conn->svr->ring, conn->fd,
                                   fresstr->buf, fresstr->len, (void *)conn,
                                   MSG_MORE, IOSQE_IO_LINK);
                io_uring_push_splice((struct io_uring *)conn->svr->ring,
                                     res.file_fd, 0, conn->svr->pipefds[1], -1,
                                     res.file_sz, (void *)conn, IOSQE_IO_LINK);
                io_uring_push_splice((struct io_uring *)conn->svr->ring,
                                     conn->svr->pipefds[0], -1, conn->fd, -1,
                                     res.file_sz, (void *)conn, 0);
#else
                io_uring_push_send((struct io_uring *)conn->svr->ring, conn->fd,
                                   fresstr->buf, fresstr->len, (void *)conn, 0,
                                   0);
#endif
            } else {
                char buf[] = RESPONSE_400;
                io_uring_push_send((struct io_uring *)conn->svr->ring, conn->fd,
                                   buf, strlen(buf), (void *)conn, 0, 0);
            }

            goto YIELD;
        }
        case IO_URING_OP_SEND:
        case IO_URING_OP_SPLICE: {
            if (++conn->cur_steps == conn->total_steps)
                goto RESET;

            // this mean we've write all data
            /* if (yielded < (int64_t)conn->str->capacity) { */
            /*     goto RESET; */
            /* } */
            goto YIELD;
        }
        }
    RESET:
        __close(conn->fd);
        conn->cur_steps = 0;
        conn->total_steps = 0;
        // reset request
        req.method = 0;
        req.path = NULL;
        req.protocol = NULL;
        // reset response
        res.status = 0;
        res.file = NULL;
        res.file_sz = 0;
        res.file_fd = 0;
        // clear kv
        req.headers.size = 0;
        req.query.size = 0;
        res.headers.size = 0;
        memset(static_kv_pairs, 0, sizeof(static_kv_pairs));
        // clear string
        string_reset(conn->str);
        string_reset(res.str);
        string_reset(fresstr);

    YIELD:
        co_yield(co, to_yield);
    }

EXIT:
    if (res.str)
        string_free(res.str);
}

void io_uring_listen_loop(pthread_t tid, struct server_info *svr)
{
    struct io_uring ring;
    struct io_uring_params ring_params;
    struct io_uring_cqe *cqe;
    memset(&ring_params, 0, sizeof(struct io_uring_params));

    int tmp = io_uring_queue_init_params(QD, &ring, &ring_params);
    if (UNLIKELY(tmp < 0)) {
        LOG_ERROR("[%lu] Failed to init io_uring: %s, %d\n", tid,
                  strerror(errno), tmp);
        return;
    }
    if (UNLIKELY(!(ring_params.features & IORING_FEAT_FAST_POLL))) {
        LOG_ERROR("IORING_FAST_POLL is not available");
        return;
    }

    // set this ring into server info
    svr->ring = (void *)&ring;

    struct server_connection *conn = &svr->conns[svr->listen_fd];
    io_uring_push_accept((struct io_uring *)svr->ring, svr->listen_fd, conn, 0);
    while (1) {
        io_uring_submit_and_wait((struct io_uring *)svr->ring, 1);
        int ret;
        unsigned head;
        unsigned count = 0;
        io_uring_for_each_cqe((struct io_uring *)svr->ring, head, cqe)
        {
            count++;
            ret = cqe->res;
            conn = (struct server_connection *)cqe->user_data;

            if (UNLIKELY(!conn)) {
                LOG_ERROR("no coroutine, abort!\n");
                exit(-1);
            }

            if (conn->action == IO_URING_OP_ACCEPT) {
                // check accept4 return
                if (UNLIKELY(ret < 0)) {
                    if (ret == -EAGAIN)
                        goto ACCEPT_AGAIN;

                    LOG_ERROR("[%lu] Something went wrong with accept: %d\n",
                              tid, ret);
                    exit(1);
                }

                conn = &svr->conns[ret];
                if (UNLIKELY(!conn->coro &&
                             __prepare_connection(
                                 svr, ret, io_uring_co_http_handler) < 0)) {
                    LOG_ERROR("Failed to create connection...\n");
                    __close(ret);
                    goto ACCEPT_AGAIN;
                }

                // push new fd to wait for recv
                io_uring_push_recv((struct io_uring *)svr->ring, ret,
                                   conn->str->buf, conn->str->capacity,
                                   (void *)conn, 0);
            ACCEPT_AGAIN:
                io_uring_push_accept((struct io_uring *)svr->ring,
                                     svr->listen_fd,
                                     &svr->conns[svr->listen_fd], 0);
                continue;
            }

            co_resume_value(conn->coro, ret);
        }
        /* io_uring_cqe_seen(&ring, cqe); */
        io_uring_cq_advance((struct io_uring *)svr->ring, count);
    }
}
