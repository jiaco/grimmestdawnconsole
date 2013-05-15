#ifndef UTILITY_H
#define UTILITY_H

#include <QByteArray>
#include <QTextStream>
#include <zlib.h>

class Utility
{
public:
    Utility();

    static QByteArray Decompress( const QByteArray &in, QTextStream& ofp );
    static QByteArray Compress( const QByteArray &in, QTextStream& ofp );
    static QByteArray Compress2( const QByteArray &in, const int& compressionLevel, QTextStream& ofp );

};
#endif // UTILITY_H
