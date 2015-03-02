#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_request_string.h"

#define LOG_FULL_REQUEST  1
#define LOG_HEADERS  2
#define LOG_BODY 3

typedef struct {
    ngx_str_t file_path;
    int fd; // Log File Handle
    int is_active;
    int logging_level;
} fx_logger;

static ngx_str_t *fx_request_to_string(ngx_http_request_t *r, unsigned int level);
static void fx_log_full_request(ngx_http_request_t *r);
static void fx_log_headers_request(ngx_http_request_t *r);
static ngx_int_t flaxton_logger_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_flaxton_logger_handler(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);
static fx_logger flaxton_logger = {
    .is_active = 0,
    .logging_level = LOG_FULL_REQUEST
};

static char *flaxton_logger_turn_on_off(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *flaxton_logger_set_file(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *flaxton_logger_log_level(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_command_t ngx_http_flaxton_logger_commands[] = {

    { ngx_string("flaxton_log"), /* directive */
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_HTTP_LMT_CONF|NGX_CONF_TAKE1,
      flaxton_logger_turn_on_off, /* configuration setup function */
      NGX_HTTP_LOC_CONF_OFFSET, /* No offset. Only one context is supported. */
      0, /* No offset when storing the module configuration on struct. */
      NULL},

    { ngx_string("flaxton_log_level"), /* directive */
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_HTTP_LMT_CONF|NGX_CONF_TAKE1,
      flaxton_logger_log_level, /* configuration setup function */
      NGX_HTTP_LOC_CONF_OFFSET, /* No offset. Only one context is supported. */
      0, /* No offset when storing the module configuration on struct. */
      NULL},

    { ngx_string("flaxton_log_file"), /* directive */
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_HTTP_LMT_CONF|NGX_CONF_TAKE1,
      flaxton_logger_set_file, /* configuration setup function */
      NGX_HTTP_LOC_CONF_OFFSET, /* No offset. Only one context is supported. */
      0, /* No offset when storing the module configuration on struct. */
      NULL},

    ngx_null_command /* command termination */
};


static ngx_http_variable_t  ngx_http_flaxton_logger_vars[] = {

    { ngx_string("flaxton_logger"), NULL, ngx_http_flaxton_logger_handler, 0,
      NGX_HTTP_VAR_CHANGEABLE|NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 }
};

/* The module context. */
static ngx_http_module_t ngx_http_flaxton_logger_module_ctx = {
    flaxton_logger_init, /* preconfiguration */
    NULL, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    NULL, /* create location configuration */
    NULL /* merge location configuration */
};

/* Module definition. */
ngx_module_t ngx_http_flaxton_logger_module = {
    NGX_MODULE_V1,
    &ngx_http_flaxton_logger_module_ctx, /* module context */
    ngx_http_flaxton_logger_commands, /* module directives */
    NGX_HTTP_MODULE, /* module type */
    NULL, /* init master */
    NULL, /* init module */
    NULL, /* init process */
    NULL, /* init thread */
    NULL, /* exit thread */
    NULL, /* exit process */
    NULL, /* exit master */
    NGX_MODULE_V1_PADDING
};

static char *flaxton_logger_turn_on_off(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_str_t *value;
    value = cf->args->elts;
    if(ngx_strcasecmp((u_char*)"on", value[1].data) == 0)
    {
        flaxton_logger.is_active = 1;
    }
    else
        if(ngx_strcasecmp((u_char*)"off", value[1].data) == 0)
        {
            flaxton_logger.is_active = 0;
        }
        else
            return NGX_CONF_ERROR;


    return NGX_CONF_OK;
}

static char *flaxton_logger_log_level(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_str_t *value;
    value = cf->args->elts;
    if(ngx_strcasecmp((u_char*)"full", value[1].data) == 0)
    {
        flaxton_logger.logging_level = LOG_FULL_REQUEST;
    }
    else
    {
        if(ngx_strcasecmp((u_char*)"headers", value[1].data) == 0)
        {
            flaxton_logger.logging_level = LOG_HEADERS;
        }
        else
        {
            if(ngx_strcasecmp((u_char*)"body", value[1].data) == 0)
            {
                flaxton_logger.logging_level = LOG_BODY;
            }
        }
    }

    return NGX_CONF_OK;
}

static char *flaxton_logger_set_file(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_str_t *value;
    value = cf->args->elts;
    flaxton_logger.file_path = value[1];
    // We will open file at the startup and keep it in memory
    flaxton_logger.fd = open((const char*)flaxton_logger.file_path.data, O_WRONLY | O_APPEND|O_CREAT, 0777);
    if(flaxton_logger.fd < 0)
    {
        puts("Could not open Flaxton Logger file");
        return NGX_CONF_ERROR;
    }
    close(flaxton_logger.fd);
    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_flaxton_logger_handler(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    if(flaxton_logger.is_active)
    {
        ngx_int_t rc;
        if ((r->method & (NGX_HTTP_POST|NGX_HTTP_PUT)) && flaxton_logger.logging_level == LOG_FULL_REQUEST) {
            rc = ngx_http_read_client_request_body(r, fx_log_full_request);
            if (rc == NGX_ERROR || rc >= NGX_HTTP_SPECIAL_RESPONSE) {
                    return rc;
            }
        }
        else
        {
            if(flaxton_logger.logging_level != LOG_BODY)
            {
                fx_log_headers_request(r);
            }
        }
    }
    return NGX_OK;
}

static ngx_int_t flaxton_logger_init(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var, *v;

    for (v = ngx_http_flaxton_logger_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}


// Core Functions for logging

static ngx_str_t *fx_request_to_string(ngx_http_request_t *r, unsigned int level)
{
    ngx_str_t *ret_data;
    if(level != LOG_BODY)
    {
        // headrs here
        ret_data = fx_request_headers_tostring(r);
    }

    if(level != LOG_HEADERS)
    {
        if(ret_data)
        {
            // Hedares Already converted
            ngx_str_t *body_data;
            u_char *tmp;
            body_data = fx_request_body_tostring(r);
            //Append Headers to Body for full logg
            tmp = ret_data->data;
            ret_data->data = ngx_palloc(r->pool, body_data->len + ret_data->len);
            memcpy(ret_data->data, tmp,ret_data->len);
            memcpy(&ret_data->data[ret_data->len], body_data->data,body_data->len);
            ret_data->len += body_data->len;
        }
        else
        {
            ret_data = fx_request_body_tostring(r);
        }
    }
    return ret_data;
}

static void fx_log_full_request(ngx_http_request_t *r)
{
    ngx_str_t *log_data;
    log_data = fx_request_to_string(r, LOG_FULL_REQUEST);
    flaxton_logger.fd = open((const char*)flaxton_logger.file_path.data, O_WRONLY | O_APPEND|O_CREAT, 0777);
    if(flaxton_logger.fd < 0)
    {
        puts("Could not open Flaxton Logger file");
        return;
    }
    // Need to make some locking Maybe
    ngx_write_fd(flaxton_logger.fd, log_data->data, log_data->len);
    close(flaxton_logger.fd);

}

static void fx_log_headers_request(ngx_http_request_t *r)
{
    ngx_str_t *log_data;
    log_data = fx_request_to_string(r, LOG_HEADERS);

    flaxton_logger.fd = open((const char*)flaxton_logger.file_path.data, O_WRONLY | O_APPEND|O_CREAT, 0777);
    if(flaxton_logger.fd < 0)
    {
        puts("Could not open Flaxton Logger file");
        return;
    }
    // Need to make some locking Maybe
    ngx_write_fd(flaxton_logger.fd, log_data->data, log_data->len);
    close(flaxton_logger.fd);
}
