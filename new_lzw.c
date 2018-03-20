/*
* CSCI3280 Introduction to Multimedia Systems
*
* --- Declaration ---
*
* I declare that the assignment here submitted is original except for source
* material explicitly acknowledged. I also acknowledge that I am aware of
* University policy and regulations on honesty in academic work, and of the
* disciplinary guidelines and procedures applicable to breaches of such policy
* and regulations, as contained in the website
* http://www.cuhk.edu.hk/policy/academichonesty/
*
* Assignment 2
* Name :
* Student ID :
* Email Addr :
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define CODE_SIZE  12
#define TRUE 1
#define FALSE 0


/* function prototypes */
unsigned int read_code( FILE*, unsigned int );
void write_code( FILE*, unsigned int, unsigned int );
void writefileheader( FILE*, char**, int );
void readfileheader( FILE*, char**, int* );
void compress( FILE*, FILE* );
void decompress( FILE*, FILE* );


typedef struct DictionaryNode
{
    int value; // the position in the list
    int prefix; // prefix for byte > 255
    int character; // the last byte of the string
    struct DictionaryNode* next;
} DictionaryNode;

// the dictionary
struct Dictionary
{
    struct DictionaryNode* dictionary;
    struct DictionaryNode* tail;
    int cnt;
};

struct DictionaryArray
{
    unsigned int used;
    struct DictionaryNode* array;
};

static void dictionaryArrayReInit();
struct Dictionary* dictionary_init();
static void dictionary_appendNode( struct Dictionary* dict, struct DictionaryNode* node );
static int dictionary_Lookup( struct Dictionary* dict, int prefix, int character );
static void dictionary_Add( struct Dictionary* dict, int prefix, int character, int value );
static void dictionary_Destroy( struct Dictionary* dict );
static struct Dictionary* g_dictionary;
static struct DictionaryArray* g_NodeArray;

int main( int argc, char** argv )
{
    int printusage = 0;
    int no_of_file;
    char** input_file_names;
    char* output_file_names;
    FILE* lzw_file;


    if ( argc >= 3 )
    {
        if ( strcmp( argv[1], "-c" ) == 0 )
        {
            /* compression */
            lzw_file = fopen( argv[2], "wb" );

            /* write the file header */
            input_file_names = argv + 3;
            no_of_file = argc - 3;
            writefileheader( lzw_file, input_file_names, no_of_file );
            {
                int i = 0;
                // Initialize the dictionary
                g_dictionary = dictionary_init();
                for ( i = 0; i < no_of_file; ++i )
                {
                    char* filename = input_file_names[i];
                    FILE* inputFile = fopen( filename, "rb" );
                    compress( inputFile, lzw_file );
                    fclose( inputFile );
                }
                write_code( lzw_file, 0, CODE_SIZE );
                dictionary_Destroy( g_dictionary );
            }
            /* ADD CODES HERE */

            fclose( lzw_file );
        }
        else if ( strcmp( argv[1], "-d" ) == 0 )
        {
            /* decompress */
            lzw_file = fopen( argv[2], "rb" );

            /* read the file header */
            no_of_file = 0;
            readfileheader( lzw_file, &output_file_names, &no_of_file );

            /* ADD CODES HERE */
            {
                int i = 0;
                char* tok = strtok( output_file_names, "\n" );
                // Initialize the dictionary
                dictionaryArrayReInit();
                for ( i = 0; i < no_of_file; ++i )
                {
                    FILE* outputFile = fopen( tok, "wb" );
                    decompress( lzw_file, outputFile );
                    tok = strtok( NULL, "\n" );
                    fclose( outputFile );
                }
                free( g_NodeArray->array );
                free( g_NodeArray );
            }

            fclose( lzw_file );
            free( output_file_names );
        }
        else
        {
            printusage = 1;
        }
    }
    else
    {
        printusage = 1;
    }

    if ( printusage )
    {
        printf( "Usage: %s -<c/d> <lzw filename> <list of files>\n", argv[0] );
    }

    return 0;
}

/*****************************************************************
*
* writefileheader() -  write the lzw file header to support multiple files
*
****************************************************************/
static void writefilesizeheader( FILE* lzw_file, char** input_file_names, int no_of_files )
{
    int i;
    /* write the file header */
    for ( i = 0; i < no_of_files; i++ )
    {
        int size = 0;
        FILE* fp = fopen( input_file_names[i], "rb" );
        fseek( fp, 0, SEEK_END );
        size = ( int )ftell( fp );
        fclose( fp );
        fprintf( lzw_file, "%d\n", size );
    }
}


/*****************************************************************
*
* writefileheader() -  write the lzw file header to support multiple files
*
****************************************************************/
void writefileheader( FILE* lzw_file, char** input_file_names, int no_of_files )
{
    int i;
    /* write the file header */
    for ( i = 0; i < no_of_files; i++ )
    {
        fprintf( lzw_file, "%s\n", input_file_names[i] );

    }
    fputc( '\n', lzw_file );

}

/*****************************************************************
*
* readfileheader() - read the fileheader from the lzw file
*
****************************************************************/
void readfileheader( FILE* lzw_file, char** output_filenames, int* no_of_files )
{
    int noofchar;
    char c, lastc;

    noofchar = 0;
    lastc = 0;
    *no_of_files = 0;
    /* find where is the end of double newline */
    while ( ( c = fgetc( lzw_file ) ) != EOF )
    {
        noofchar++;
        if ( c == '\n' )
        {
            if ( lastc == c )
                /* found double newline */
            {
                break;
            }
            ( *no_of_files )++;
        }
        lastc = c;
    }

    if ( c == EOF )
    {
        /* problem .... file may have corrupted*/
        *no_of_files = 0;
        return;

    }
    /* allocate memeory for the filenames */
    *output_filenames = ( char* )malloc( sizeof( char ) * noofchar );
    /* roll back to start */
    fseek( lzw_file, 0, SEEK_SET );

    fread( ( *output_filenames ), 1, ( size_t )noofchar, lzw_file );

    return;
}

/*****************************************************************
*
* read_code() - reads a specific-size code from the code file
*
****************************************************************/
unsigned int read_code( FILE* input, unsigned int code_size )
{
    static int input_bit_count = 0;
    static unsigned long input_bit_buffer = 0L;

    int code = fgetc( input );
    if ( code == EOF ) { return 0; }

    if ( input_bit_count > 0 )
    {
        code = ( input_bit_buffer << 8 ) + code;

        input_bit_count = 0;
    }
    else
    {
        int nextCode = fgetc( input );

        input_bit_buffer = nextCode & 0xF; // save leftover, the last 00001111
        input_bit_count = 1;

        code = ( code << 4 ) + ( nextCode >> 4 );
    }
    return code;
}

/*****************************************************************
*
* write_code() - write a code (of specific length) to the file
*
****************************************************************/
void write_code( FILE* output, unsigned int code, unsigned int code_size )
{
    static int output_bit_count = 0;
    static unsigned long output_bit_buffer = 0L;
    /* Each output code is first stored in output_bit_buffer,    */
    /*   which is 32-bit wide. Content in output_bit_buffer is   */
    /*   written to the output file in bytes.                    */

    /* output_bit_count stores the no. of bits left              */

    output_bit_buffer |= ( unsigned long )code << ( 32 - code_size - output_bit_count );
    output_bit_count += code_size;

    while ( output_bit_count >= 8 )
    {
        putc( output_bit_buffer >> 24, output );
        output_bit_buffer <<= 8;
        output_bit_count -= 8;
    }


    /* only < 8 bits left in the buffer                          */

}

/*****************************************************************
*
* compress() - compress the source file and output the coded text
*
****************************************************************/
void compress( FILE* input, FILE* output )
{
    // ADD code here
    int C;
    int nextCode = g_dictionary->cnt;
    int index = -1;
    int P = -1;

    int count = 0;
    C = getc( input );
    while ( C != EOF )
    {
        // Search for the String <P,C> in the dictionary
        index = dictionary_Lookup( g_dictionary, P, C );
        if ( index != -1 )
        {
            // If Found
            // Remember the code,
            // append C to the end of string P
            P = index;
        }
        else
        {
            // If Not Found
            // Output the code correspond to P
            write_code( output, P, CODE_SIZE );

            // Add to the dictionary the new entry <P,C>
            dictionary_Add( g_dictionary, P, C, nextCode );
            ++nextCode;

            if ( nextCode >= ( ( 1 << CODE_SIZE ) - 1 ) )
            {
                // full
                dictionary_Destroy( g_dictionary );
                g_dictionary = dictionary_init();
                nextCode = g_dictionary->cnt;
            }
            // P=C
            P = C;
        }
        C = getc( input );
    }
    // Output the code that corresponde to P
    write_code( output, P, CODE_SIZE );
    write_code( output, ( 1 << CODE_SIZE ) - 1, CODE_SIZE );
}

static int decode( int code, FILE* outputFile, struct DictionaryNode* array );

/*****************************************************************
*
* decompress() - decompress a compressed file to the orig. file
*
****************************************************************/
void decompress( FILE* input, FILE* output )
{
    /* ADD CODES HERE */
    int previousCode;
    int currentCode;
    int firstChar;
    int count = 0;
    currentCode = read_code( input, CODE_SIZE );
    fputc( currentCode, output );
    while ( currentCode != ( ( 1 << CODE_SIZE ) - 1 ) )
    {
        previousCode = currentCode;
        currentCode = read_code( input, CODE_SIZE );
        if ( currentCode == ( ( 1 << CODE_SIZE ) - 1 ) ) { break; }
//        if ( g_NodeArray->used >= ( ( 1 << CODE_SIZE ) - 1 ) )
//        {
//            // full, re-initialize it
//            printf( "full %d\n", ++count );
//            dictionaryArrayReInit();
//            previousCode = currentCode;
//        }
        if ( currentCode >= g_NodeArray->used )
        {
            // not found
            firstChar = decode( previousCode, output, g_NodeArray->array );
            fputc( firstChar, output );
        }
        else
        {
            // found
            firstChar = decode( currentCode, output, g_NodeArray->array );
        }
        g_NodeArray->array[g_NodeArray->used].prefix = previousCode;
        g_NodeArray->array[g_NodeArray->used].character = firstChar;
        ++g_NodeArray->used;
        if ( g_NodeArray->used >= ( ( 1 << CODE_SIZE ) - 1 ) )
        {
            // full, re-initialize it
            dictionaryArrayReInit();
        }
    }
}

static void dictionaryArrayReInit()
{
    int i;
    if ( g_NodeArray == NULL )
    {
        g_NodeArray = ( struct DictionaryArray* )malloc( sizeof( struct DictionaryArray ) );
    }
    else
    {
        free( g_NodeArray->array );
    }
    memset( g_NodeArray, 0, sizeof( struct DictionaryArray ) );
    g_NodeArray->array = ( struct DictionaryNode* )malloc( sizeof( struct DictionaryNode ) * ( ( 1 << CODE_SIZE ) ) );
    memset( g_NodeArray->array, 0, sizeof( struct DictionaryNode ) * ( ( 1 << CODE_SIZE ) ) );
    for ( i = 0; i < 255; ++i )
    {
        g_NodeArray->array[i].character = i;
        g_NodeArray->array[i].prefix = -1;
    }
    g_NodeArray->used = 256;

}

static int decode( int code, FILE* outputFile, struct DictionaryNode* array )
{
    int character;
    int temp;

    if ( code > 255 )
    {
        // decode
        character = array[code].character;
        temp = decode( array[code].prefix, outputFile, array ); // recursion
    }
    else
    {
        character = code; // ASCII
        temp = code;
    }
    {
        fputc( character, outputFile );
    }
    return temp;
}

// initialize the dictionary of ASCII characters @12bits
struct Dictionary* dictionary_init()
{
    struct Dictionary* ret;
    struct DictionaryNode* node;
    int i;
    ret = ( struct Dictionary* )malloc( sizeof( struct Dictionary ) );
    memset( ret, 0, sizeof( struct Dictionary ) );
    for ( i = 0; i < 256; i++ ) // ASCII
    {
        node = ( struct DictionaryNode* )malloc( sizeof( struct DictionaryNode ) );
        node->value = i;
        node->prefix = -1;
        node->character = i;
        node->next = NULL;
        dictionary_appendNode( ret, node );
    }
    ret->cnt = 256;
    return ret;
}


// add node to the list
static void dictionary_appendNode( struct Dictionary* dict, struct DictionaryNode* node )
{
    if ( dict->dictionary != NULL ) { dict->tail->next = node; }
    else { dict->dictionary = node; }
    dict->tail = node;
    node->next = NULL;
    dict->cnt += 1;
}

// destory the whole dictionary down to NULL
static void dictionary_Destroy( struct Dictionary* dict )
{
    while ( dict->dictionary != NULL )
    {
        struct DictionaryNode* del = dict->dictionary;
        dict->dictionary = dict->dictionary->next; /* the head now links to the next element */
        free( del );
    }
    free( dict );
}

// is prefix + character in the dictionary?
static int dictionary_Lookup( struct Dictionary* dict, int prefix, int character )
{
    struct DictionaryNode* node;
    for ( node = dict->dictionary; node != NULL; node = node->next ) // ...traverse forward
    {
        if ( node->prefix == prefix && node->character == character ) { return node->value; }
    }
    return -1;
}

// add prefix + character to the dictionary
static void dictionary_Add( struct Dictionary* dict, int prefix, int character, int value )
{
    struct DictionaryNode* node;
    node = ( struct DictionaryNode* )malloc( sizeof( struct DictionaryNode ) );
    node->value = value;
    node->prefix = prefix;
    node->character = character;
    dictionary_appendNode( dict, node );
}
