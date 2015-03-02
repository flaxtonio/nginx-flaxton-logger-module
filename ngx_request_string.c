#include "ngx_request_string.h"

ngx_str_t *fx_request_headers_tostring(ngx_http_request_t *r)
{
    ngx_str_t *ret_val;
    ngx_list_part_t  *part;
    ngx_table_elt_t  *header;
    unsigned int i, len=0;
    ret_val = (ngx_str_t *)ngx_palloc(r->pool, sizeof(ngx_str_t));
    part = &r->headers_in.headers.part;
    header = part->elts;

    // Caluclateing headers length
    for (i = 0; /* void */; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            header = part->elts;
            i = 0;
        }

        if (header[i].hash == 0) {
            continue;
        }

        len += header[i].key.len + sizeof(": ") - 1 + header[i].value.len + sizeof(CRLF) - 1;
    }
    len += r->request_line.len + HEADER_SEPERATOR_SIZE + sizeof(CRLF) - 1;
    ret_val->data = (u_char*)ngx_palloc(r->pool, len);
    ret_val->len = len;
    int index = 0;
    memcpy(&ret_val->data[index], HEADER_SEPERATOR, HEADER_SEPERATOR_SIZE);
    index += HEADER_SEPERATOR_SIZE;
    memcpy(&ret_val->data[index], r->request_line.data, r->request_line.len);
    index += r->request_line.len;
    // Appending headers
    for (i = 0; /* void */; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            header = part->elts;
            i = 0;
        }

        if (header[i].hash == 0) {
            continue;
        }

        memcpy(&ret_val->data[index], header[i].key.data, header[i].key.len);
        index += header[i].key.len;
        memcpy(&ret_val->data[index], ": ", sizeof(": "));
        index += sizeof(": ") - 1;
        memcpy(&ret_val->data[index], header[i].value.data, header[i].value.len);
        index += header[i].value.len;
        memcpy(&ret_val->data[index], CRLF, sizeof(CRLF));
        index += sizeof(CRLF) - 1;
    }
    memcpy(&ret_val->data[index], CRLF, sizeof(CRLF));

    return ret_val;
}


ngx_str_t *fx_request_body_tostring(ngx_http_request_t *r)
{
    ngx_str_t *ret_val;
    ngx_chain_t * chain;
    ngx_buf_t * buf;
    int index=0, len=0;
    ret_val = (ngx_str_t *)ngx_palloc(r->pool, sizeof(ngx_str_t));
    chain = r->request_body->bufs;
    // calculating length
    while(chain)
    {
        buf = chain->buf;
        len += buf->last - buf->pos;
        chain = chain->next;
    }
    len += 2*(sizeof(CRLF) - 1);
    ret_val->data = ngx_palloc(r->pool, len);
    ret_val->len = len;
    index=0;
    chain = r->request_body->bufs;
    // appending text
    while(chain)
    {
        buf = chain->buf;
        len = buf->last - buf->pos;
        memcpy(&ret_val->data[index], buf->pos, len);
        index += len;
        chain = chain->next;
    }
    memcpy(&ret_val->data[index], CRLF, sizeof(CRLF));
    index += sizeof(CRLF) - 1;
    memcpy(&ret_val->data[index], CRLF, sizeof(CRLF));
    return ret_val;
}
