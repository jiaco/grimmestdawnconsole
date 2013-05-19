#ifndef UTILITY_H
#define UTILITY_H

#include <QFile>
#include <QDataStream>
#include <QByteArray>
#include <QTextStream>
#include <zlib.h>

class TextFile : public QTextStream
{
public:
    TextFile( const QString& fname = "", const QIODevice::OpenMode& mode = QIODevice::ReadOnly );
    ~TextFile();

    bool open( const QString& fname, const QIODevice::OpenMode& mode );
    int seek( const qint32& pos = 0 );
protected:
    QFile   file;
};

class DataFile : public QDataStream
{
public:
    DataFile( const QString& fname = "", const QIODevice::OpenMode& mode = QIODevice::ReadOnly );
    ~DataFile();

    bool    open( const QString& fname, const QIODevice::OpenMode& mode );
    int     seek( const qint32& pos = 0 );
    int     pos();
    int     size();
protected:
    QFile   file;
};

class Byter
{
public:
    Byter();
    ~Byter();

    QString GetCString( QDataStream& fp );
    //char*   GetCharp( QDataStream& fp, qint32 pos, qint32 len );
    //QByteArray GetBytes( QDataStream& fp, qint32 pos, qint32 len );
    char    *sp;
    int     s_sp;
};

class Utility
{
public:
    Utility();
    static QString  ReadCString( QDataStream& fp );
    static QByteArray   ReadCompressed( DataFile& fp, const qint32& offset, const qint32& size );
    static QByteArray Decompress( const QByteArray &in );
    static QByteArray Compress( const QByteArray &in );
    static QByteArray DecompressDebug( const QByteArray &in, QTextStream& ofp );
    static QByteArray CompressDebug( const QByteArray &in, QTextStream& ofp );
    static QByteArray Compress2( const QByteArray &in, const int& compressionLevel, QTextStream& ofp );

};
#endif // UTILITY_H
