#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void print(const char* data, size_t data_length)
{
	for ( size_t i = 0; i < data_length; i++ )
		putchar((int) ((const unsigned char*) data)[i]);
}

int printf(const char* restrict format, ...)
{
	va_list parameters;
	va_start(parameters, format);

	int written = 0;
	size_t amount;
	bool rejected_bad_specifier = false;

	while ( *format != '\0' )
	{
		if ( *format != '%' )
		{
		print_c:
			amount = 1;
			while ( format[amount] && format[amount] != '%' )
				amount++;
			print(format, amount);
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format;

		if ( *(++format) == '%' )
			goto print_c;

		if ( rejected_bad_specifier )
		{
		incomprehensible_conversion:
			rejected_bad_specifier = true;
			format = format_begun_at;
			goto print_c;
		}

		if ( *format == 'c' )
		{
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			print(&c, sizeof(c));
		}
        else if ( *format == 'd' )
        {
            format++;
            int d = (int) va_arg(parameters, int);
            int size = 0;

            //Zero override
            if (d==0) {
                char buffer[1];
                buffer[0] = '0';
                print(buffer, 1);
            }

            /* Determine size of buffer */
            int temp = d;
            while(temp) { ++size; temp /= 10; }
            //if (temp == 0) {size = 1;}
            char buffer[size];


            /* Get each individual digit and add to buffer as char*/
            int i;
            for(i=0; d; i++)
            {
                buffer[size-i-1] = (d % 10) + '0';
                d /= 10;
            }

            print(buffer, size);
        }
        else if ( *format == 'x' )
        {
            format++;
            int d = (int) va_arg(parameters, int);
            int size = 0;
            int divisor = 16;
            //Zero override
            if(d == 0) {
                char buffer[1];
                buffer[0] = '0';
                print(buffer, 1);
            }

            // deterine size!
            int temp = d;
            while(temp) { ++size; temp /= 16; }
            char buffer[size];

            int loop = 0;
            do {
                int remainder = d % divisor;

                buffer[loop++] = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;

            } while (d /= divisor);

            buffer[loop] = 0;

            // reverse array
            int tmp;
            int start = 0;
            int end = size - 1;
            while(start < end) {
                tmp = buffer[start];
                buffer[start] = buffer[end];
                buffer[end] = tmp;
                start++;
                end--;
            }

            print(buffer, size);
        }
		else if ( *format == 's' )
		{
			format++;
			const char* s = va_arg(parameters, const char*);
			print(s, strlen(s));
		}
		else
		{
			goto incomprehensible_conversion;
		}
	}

	va_end(parameters);

	return written;
}
