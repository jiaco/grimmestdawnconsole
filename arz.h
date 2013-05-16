#ifndef ARZ_H
#define ARZ_H

#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QString>
#include <QStringList>

#include "utility.h"

class Record
{
public:
    Record();
    Record( DataFile& fp, int baseOffset = 24 );
    ~Record();

    qint32  idstringIndex;
    QString recordType;
    qint32  dataOffset;
    qint32  dataCSize;
    qint32  unk1, unk2;

    QString idstring0;

    QByteArray data, cdata;

    void readData( DataFile& fp );
    void    lookupString0( const QStringList& tab );
    void decompress();
    void decode();
    void    doit( DataFile& fp, const QStringList& stab );
    void show( QTextStream& fp );
};

class ARZ
{
public:
    ARZ();
    ~ARZ();

    bool readRecordInfo(const qint32& pos, const qint32& cnt );
    bool    lookupString0s();
    bool readRecordData();
    bool decompressData();

    bool readStringTable( const qint32& pos, const qint32& size );

//    static const char* dbfilename = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Grim Dawn\\database";
    void doit();

    DataFile    arz;
    TextFile    cout;


protected:
    static const int HEAD_SIZE = 6;
    static const int FOOT_SIZE = 4;
    static const int SKIP_HEAD = 24;

    qint32 header[ HEAD_SIZE ];
    qint32 footer[ FOOT_SIZE ];

    QList<Record>   records;
    QStringList     strings;
    qint32          instringsCnt;
};

#endif // ARZ_H
