/* output.c
 *
 * Copyright (C) 2013 Richard Smith <richard@ex-parrot.com> 
 * All rights reserved.
 */

/* The C library fflush() */
fflush( stream ) {
    auto count = stream[2] - stream[3];
    if ( count ) {
        if ( write( stream[0], stream[3], count ) != count )
            return -1;
    }
    return 0;
}

/* The C library fwrite() */
fwrite( ptr, size, nmemb, stream ) {
    auto len = size * nmemb;
    auto written = 0;

    while ( len ) {
        auto space = stream[1] - (stream[2] - stream[3]);

        /* If the buffer is partially full, and after filling and emptying it,
         * it still won't have space for the string, then flush the buffer
         * immediately. */
        if ( stream[2] != stream[3] && len >= space + stream[1] ) {
            if ( fflush( stream ) == -1 )
                return written;
        }

        /* If the buffer is empty and not big enough to hold the string,
         * then write the string out directly. */
        else if ( stream[2] == stream[3] && len >= stream[1] ) {
            auto n = write( stream[0], ptr, len );
            if ( n == -1 ) return written;
            written += n;
            if ( n != len ) return written;
            len = 0;
        }

        /* Otherwise append as much as possible to the buffer. */
        else {
            auto chunk = space < len ? space : len;
            memcpy( stream[2], ptr, chunk );
            stream[2] = stream[2] + chunk;
            ptr += chunk;
            len -= chunk;
            written += chunk;

            /* Flush the buffer now if it's full. */
            if ( chunk == space && fflush( stream ) == -1 )
                return written;
        }
    }

    return written;
}


/* The C library fputs() */
fputs(s, stream) {
    return fwrite( s, 1, strlen(s), stream );
}

/* The B library putstr() -- unlike C's puts, it doesn't add a '\n' */
putstr(s) {
    return fputs( s, stdout );
}

/* The C library puts() */
puts(s) {
    return putstr(s) + putchar('\n');
}

/* The C library fputc() */
fputc(c, stream) {
    return fwrite( &c, 1, 1, stream ) == 1 ? c : -1;
}

/* The C library putc() */
putc(s, stream) {
    return putc( s, stream );
}

/* The C library putchar() */
putchar(c) {
    return fputc( c, stdout );
}

/* The C library vfprintf() */
vfprintf(stream, fmt, ap) {
    auto written = 0, c, n;
    while ( c = char(fmt, 0) ) {
        /* Pass normal characters straight through.
         * TODO:  We could strchr for '%' and push through the whole lot
         * with fputs */
        if (c != '%') {
            if ( fputc(*fmt, stream) == -1 )
                return -1;
            ++written;
            ++fmt;
            continue;
        }

        c = char(++fmt, 0);
        
        if (c == 'c') {
            ap += 4;
            if ( fputc(*ap, stream) == -1 )
                return -1;
            ++written;
        }
        else if (c == 's') {
            ap += 4;
            if ( ( n = fputs(*ap, stream) ) == -1 )
                return -1; 
            written += n;
        }
        else if (c == 'd') {
            ap += 4;
            if (*ap == 0) { 
                if ( fputc('0', stream) == -1 )
                    return -1;
                ++written;
            }
            else {
                /* We don't currently support arrays. */
                auto buffer[4] = {0,0,0,0},  b = 14,  i = *ap;

                if (i < 0) {
                    if ( fputc('-', stream) == -1 )
                        return -1;
                    ++written;
                    i = -i;
                }

                while (i) {
                    lchar(buffer, b, i % 10 + '0');
                    i /= 10; --b;
                }

                if ( ( n = fputs(buffer+b+1, stream) ) == -1 )
                    return -1; 
                written += n;
            }
        }
        else {
            if ( fputc(c, stream) == -1 )
                return -1;
            ++written;
        }

        ++fmt;
    }

    return written;
}

/* The C library printf() */
printf(fmt) {
    return vfprintf(stdout, fmt, &fmt);
}
