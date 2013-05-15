#include "utility.h"

Utility::Utility()
{
}
QByteArray Utility::Decompress( const QByteArray &in, QTextStream& ofp )
{
    QByteArray rv;
   /*
    int ret;
    z_stream strm;
    static const int CHUNK_SIZE = 1024;
    char out[ CHUNK_SIZE ];

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = in.size();
    strm.next_in = (Bytef*)(in.data());

    ret = inflateInit2( &strm, 15 + 32 );
*/
    Bytef *compr = (Bytef*)( in.data() );
    uLong comprLen = (uLong)( in.size() );
    Bytef *out = new Bytef[10240];
        uLong outLen = 10240;

        ofp << "DEBUG " << comprLen << endl;
    int err = uncompress( out, &outLen, compr, comprLen );
    ofp << "DEBUG " << err << "  :  " << outLen << endl;
    rv.setRawData( (char*)out, outLen );
    return( rv );
    //return( QByteArray( (char*)out ) );
}
QByteArray Utility::Compress( const QByteArray& in, QTextStream& ofp )
{
    QByteArray rv;

    Bytef *data = (Bytef*)( in.data() );
    uLong dataLen = (uLong)( in.size() );
    Bytef *cdata = new Bytef[ dataLen ];
    uLong cdataLen = dataLen;

    int err = compress( cdata, &cdataLen, data, dataLen );
    rv.setRawData( (char*)cdata, cdataLen );

    ofp << "DEBUG " << err << "  :  " << cdataLen << endl;

    return( rv );

}
QByteArray Utility::Compress2( const QByteArray& in, const int& compressionLevel, QTextStream& ofp )
{
    QByteArray rv;

    Bytef *data = (Bytef*)( in.data() );
    uLong dataLen = (uLong)( in.size() );
    Bytef *cdata = new Bytef[ dataLen ];
    uLong cdataLen = dataLen;

    int err = compress2( cdata, &cdataLen, data, dataLen, compressionLevel );
    rv.setRawData( (char*)cdata, cdataLen );

    ofp << "DEBUG " << err << "  :  " << cdataLen << endl;

    return( rv );

}
