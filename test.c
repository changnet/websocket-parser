#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "websocket_parser.h"

void new_masking_key( char mask[4] );
int on_frame_end( struct websocket_parser *parser );
int on_frame_header( struct websocket_parser *parser );
int on_frame_body(
    struct websocket_parser *parser, const char * at, size_t length );
int on_frame_end_pause( struct websocket_parser *parser );
int on_frame_header_pause( struct websocket_parser *parser );
int on_frame_body_pause(
    struct websocket_parser *parser, const char * at, size_t length );

struct websocket_body
{
    int done;
    char *body;
    size_t offset;
    size_t length;
};

void test()
{
    // encode
    const char *ctx = "hey"; // less than 4 to test mask offset
    const char *append_ctx = ",websocket parser";

    websocket_flags flags = WS_OP_TEXT | WS_HAS_MASK;
    size_t size = strlen( ctx ) + strlen( append_ctx );
    size_t length = websocket_calc_frame_size( flags,size );

    uint8_t mask_offset = 0;
    char mask[4] = { 0 };
    new_masking_key( mask );

    char *buffer = malloc( length );
    size_t offset = websocket_build_frame_header( buffer,flags,mask,size );
    offset += websocket_append_frame(
        buffer + offset,flags,mask,ctx,strlen( ctx ),&mask_offset );
    websocket_append_frame( buffer + offset,
        flags,mask,append_ctx,strlen( append_ctx ),&mask_offset );

    // encode finish,decode now
    websocket_parser_settings settings;
    websocket_parser_settings_init(&settings);

    settings.on_frame_end    = on_frame_end;
    settings.on_frame_body   = on_frame_body;
    settings.on_frame_header = on_frame_header;

    struct websocket_body ws_body;
    memset( &ws_body,0,sizeof(ws_body) );

    struct websocket_parser parser;
    websocket_parser_init( &parser );
    parser.data = &ws_body;

    size_t nparser =
        websocket_parser_execute( &parser,&settings,buffer,length );

    assert( nparser == length );

    assert( 0 == parser.error );
    assert( ws_body.done ); // verify parser finish
    assert( 0 == strncmp( ws_body.body,ctx,strlen(ctx)) ); // verify ctx

    // verify append ctx
    const char *append_offset = ws_body.body + strlen(ctx);
    assert( 0 == strncmp( append_offset,append_ctx,strlen(append_ctx)) );

    free( buffer );
    free( ws_body.body );;
}

void test_char_by_char()
{
    // encode
    const char *ctx = "hey"; // less than 4 to test mask offset
    const char *append_ctx = ",websocket parser";

    websocket_flags flags = WS_OP_TEXT | WS_HAS_MASK;
    size_t size = strlen( ctx ) + strlen( append_ctx );
    size_t length = websocket_calc_frame_size( flags,size );

    uint8_t mask_offset = 0;
    char mask[4] = { 0 };
    new_masking_key( mask );

    char *buffer = malloc( length );
    size_t offset = websocket_build_frame_header( buffer,flags,mask,size );
    offset += websocket_append_frame(
        buffer + offset,flags,mask,ctx,strlen( ctx ),&mask_offset );
    websocket_append_frame( buffer + offset,
        flags,mask,append_ctx,strlen( append_ctx ),&mask_offset );

    // encode finish,decode now
    websocket_parser_settings settings;
    websocket_parser_settings_init(&settings);

    settings.on_frame_end    = on_frame_end;
    settings.on_frame_body   = on_frame_body;
    settings.on_frame_header = on_frame_header;

    struct websocket_body ws_body;
    memset( &ws_body,0,sizeof(ws_body) );

    struct websocket_parser parser;
    websocket_parser_init( &parser );
    parser.data = &ws_body;

    char *pbuffer = buffer;
    while (length > 0)
    {
        size_t nparser =
            websocket_parser_execute( &parser,&settings,pbuffer,1 );
        assert(nparser > 0);
        length --;
        pbuffer ++;
    }

    assert( 0 == length );

    assert( 0 == parser.error );
    assert( ws_body.done ); // verify parser finish
    assert( 0 == strncmp( ws_body.body,ctx,strlen(ctx)) ); // verify ctx

    // verify append ctx
    const char *append_offset = ws_body.body + strlen(ctx);
    assert( 0 == strncmp( append_offset,append_ctx,strlen(append_ctx)) );

    free( buffer );
    free( ws_body.body );;
}

void test_pause()
{
    // encode
    const char *ctx = "hey"; // less than 4 to test mask offset
    const char *append_ctx = ",websocket parser";

    websocket_flags flags = WS_OP_TEXT | WS_HAS_MASK;
    size_t size = strlen( ctx ) + strlen( append_ctx );
    size_t length = websocket_calc_frame_size( flags,size );

    uint8_t mask_offset = 0;
    char mask[4] = { 0 };
    new_masking_key( mask );

    char *buffer = malloc( length );
    size_t offset = websocket_build_frame_header( buffer,flags,mask,size );
    offset += websocket_append_frame(
        buffer + offset,flags,mask,ctx,strlen( ctx ),&mask_offset );
    websocket_append_frame( buffer + offset,
        flags,mask,append_ctx,strlen( append_ctx ),&mask_offset );

    // encode finish,decode now
    websocket_parser_settings settings;
    websocket_parser_settings_init(&settings);

    settings.on_frame_end    = on_frame_end_pause;
    settings.on_frame_body   = on_frame_body_pause;
    settings.on_frame_header = on_frame_header_pause;

    struct websocket_body ws_body;
    memset( &ws_body,0,sizeof(ws_body) );

    struct websocket_parser parser;
    websocket_parser_init( &parser );
    parser.data = &ws_body;

    char *pbuffer = buffer;
    size_t nparser =
        websocket_parser_execute( &parser,&settings,pbuffer,1 );

    // is not a pause
    assert( nparser == 1 );
    assert( 0 == parser.error );

    // continue parse
    pbuffer ++;
    length --;
    while (length > 0) {
        nparser =
            websocket_parser_execute( &parser,&settings,pbuffer,length );

        assert(nparser > 0);
        length -= nparser;
        pbuffer += nparser;
        if (length > 0) assert(WPE_PAUSE == parser.error);
    }

    assert( 0 == length );
    assert( WPE_PAUSE == parser.error );
    assert( ws_body.done ); // verify parser finish
    assert( 0 == strncmp( ws_body.body,ctx,strlen(ctx)) ); // verify ctx

    // verify append ctx
    const char *append_offset = ws_body.body + strlen(ctx);
    assert( 0 == strncmp( append_offset,append_ctx,strlen(append_ctx)) );

    free( buffer );
    free( ws_body.body );;
}

int main()
{
    test();
    test_char_by_char();
    test_pause();

    return 0;
}

int on_frame_header( struct websocket_parser *parser )
{
    struct websocket_body *ws_body = (struct websocket_body *)( parser->data );

    ws_body->offset = 0;
    if( parser->length && ws_body->length < parser->length )
    {
        if ( ws_body->length ) free( ws_body->body );

        ws_body->body= malloc( parser->length );
        ws_body->length = parser->length;
    }
    return 0;
}

int on_frame_body(
    struct websocket_parser *parser, const char * at, size_t length )
{
    struct websocket_body *ws_body = (struct websocket_body *)( parser->data );

    if( parser->flags & WS_HAS_MASK ) {
        websocket_parser_decode(
            ws_body->body + ws_body->offset, at, length, parser );
    }
    else
    {
        memcpy( ws_body->body + ws_body->offset, at, length );
    }
    ws_body->offset += length;
    assert( ws_body->offset <= ws_body->length );

    return 0;
}

int on_frame_end( struct websocket_parser *parser )
{
    struct websocket_body *ws_body = (struct websocket_body *)( parser->data );

    assert( !ws_body->done );

    ws_body->done = 1;
    return 0;
}

int on_frame_header_pause( struct websocket_parser *parser )
{
    struct websocket_body *ws_body = (struct websocket_body *)( parser->data );

    ws_body->offset = 0;
    if( parser->length && ws_body->length < parser->length )
    {
        if ( ws_body->length ) free( ws_body->body );

        ws_body->body= malloc( parser->length );
        ws_body->length = parser->length;
    }
    return WPE_PAUSE;
}

int on_frame_body_pause(
    struct websocket_parser *parser, const char * at, size_t length )
{
    struct websocket_body *ws_body = (struct websocket_body *)( parser->data );

    if( parser->flags & WS_HAS_MASK ) {
        websocket_parser_decode(
            ws_body->body + ws_body->offset, at, length, parser );
    }
    else
    {
        memcpy( ws_body->body + ws_body->offset, at, length );
    }
    ws_body->offset += length;
    assert( ws_body->offset <= ws_body->length );

    return WPE_PAUSE;
}

int on_frame_end_pause( struct websocket_parser *parser )
{
    struct websocket_body *ws_body = (struct websocket_body *)( parser->data );

    assert( !ws_body->done );

    ws_body->done = 1;
    return WPE_PAUSE;
}

void new_masking_key( char mask[4] )
{
    /* George Marsaglia  Xorshift generator
     * www.jstatsoft.org/v08/i14/paper
     */
    static unsigned long x=123456789, y=362436069, z=521288629;

    //period 2^96-1
    unsigned long t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

   t = x;
   x = y;
   y = z;
   z = t ^ x ^ y;

   memcpy( mask,&z,4 );
}
