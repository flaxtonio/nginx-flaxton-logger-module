#ifndef _FLAXTON_REQUEST_STRING
#define _FLAXTON_REQUEST_STRING

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#define HEADER_SEPERATOR "*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*"
#define HEADER_SEPERATOR_SIZE (sizeof(HEADER_SEPERATOR) - 1)

ngx_str_t *fx_request_headers_tostring(ngx_http_request_t *r);
ngx_str_t *fx_request_body_tostring(ngx_http_request_t *r);

#endif
