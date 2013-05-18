#include "utility.h"


TextFile::TextFile( const QString& fname, const QIODevice::OpenMode& mode )
{
    if( !fname.isEmpty() ) {
        open( fname, mode );
    }
}
TextFile::~TextFile()
{
    file.close();
}
bool    TextFile::open( const QString& fname, const QIODevice::OpenMode& mode )
{
    bool rv;
    if( fname == "stdout" ) {
        rv = file.open( stdout, mode );
    } else if( fname == "stdin" ) {
        rv = file.open( stdin, mode );
    } else {
        file.setFileName( fname );
        rv = file.open( mode );
    }
    setDevice( &file );
    return( rv );
}
int TextFile::seek( const qint32& pos )
{
    return( file.seek( pos ) );
}

DataFile::DataFile( const QString& fname, const QIODevice::OpenMode& mode )
{
    if( !fname.isEmpty() ) {
        open( fname, mode );
    }
}
DataFile::~DataFile()
{

    file.close();
}
bool    DataFile::open( const QString& fname, const QIODevice::OpenMode& mode )
{
    bool rv;
    file.setFileName( fname );
    rv = file.open( mode );
    setDevice( &file );
    setByteOrder( QDataStream::LittleEndian );
    return( rv );

}
int DataFile::seek( const qint32& pos )
{
    return( file.seek( pos ) );
}
int DataFile::pos()
{
    return( file.pos() );
}

Byter::Byter()
{
    sp = NULL;
    s_sp = 0;
}

Byter::~Byter()
{
    if( sp ) {
        delete[] sp;
        sp = NULL;
    }
}

QString Byter::GetCString( QDataStream& fp )
{
    qint32 size;

    fp >> size;
    if( size > s_sp ) {
        s_sp = size * 2;
        if( sp ) {
            delete[] sp;
            sp = NULL;
        }
        sp = new char[ s_sp ];
    }
    fp.readRawData( sp, size );
    return( QString::fromLocal8Bit(sp, size ) );
}

Utility::Utility()
{
}
QString  Utility::ReadCString( QDataStream& fp )
{
    QString rv;
    uint rlen;
    char *sp;
    fp.readBytes( sp, rlen );
    rv = QString::fromLocal8Bit( sp );
    delete[] sp;
    return( rv );
}
QByteArray  Utility::ReadCompressed(DataFile &fp, const qint32 &offset, const qint32 &size)
{
    char    *sp;
    QByteArray  rv;
    int rsize;

    fp.seek( offset );
    sp = new char[ size ];
    rsize = fp.readRawData( sp, size );
    if( rsize != size ) {
        return( rv );
    }
    rv.setRawData( sp, (uint)size );
    return( rv );

}

QByteArray Utility::Decompress( const QByteArray &in )
{
    QByteArray rv;
    Bytef *compr = (Bytef*)( in.data() );
    uLong comprLen = (uLong)( in.size() );
    Bytef *out = new Bytef[10240];
    uLong outLen = 10240;

    int err = uncompress( out, &outLen, compr, comprLen );
    rv.setRawData( (char*)out, outLen );
    return( rv );
}
QByteArray Utility::Compress( const QByteArray& in )
{
    QByteArray rv;
    Bytef *data = (Bytef*)( in.data() );
    uLong dataLen = (uLong)( in.size() );
    Bytef *cdata = new Bytef[ dataLen ];
    uLong cdataLen = dataLen;

    int err = compress( cdata, &cdataLen, data, dataLen );
    rv.setRawData( (char*)cdata, cdataLen );
    return( rv );
}

QByteArray Utility::DecompressDebug( const QByteArray &in, QTextStream& ofp )
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
QByteArray Utility::CompressDebug( const QByteArray& in, QTextStream& ofp )
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
