#ifndef ARZ_H
#define ARZ_H

#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QString>
#include <QStringList>
#include <QRegExp>

#include "utility.h"

/*
inline int BackInt( const QString& s )
{
    QRexgEx
}
*/
class   NormalPool
{
public:
    NormalPool();
    int name, weight, levelVar;
    int nameIdx, weightIdx, levelVarIdx;
};
class   ChampionPool : public NormalPool
{
public:
    ChampionPool();
    int limit, minPClevel;
    int limitIdx, minPClevelIdx;
};
class   BasePool
{
public:
    int spawnMin, spawnMax;
    int championMin;
    int spawnMinIdx, spawnMaxIdx, championMinIdx;
};

class Variable
{
    /* dataTypes:
     * 0 integer
     * 1 float
     * 2 string
     * 3 bool
     */
public:
   // QString name;
    qint32     nameIndex;
    qint16     dataType;
    qint16     valCount;
    QList<QVariant> data;
};

class Record
{
public:
    Record();
    ~Record();


    qint32  idstringIndex;
    QString recordType;
    qint32  dataOffset;
    qint32  dataCSize;
    qint32  unk1, unk2, unk3;
   // QString tmplate;
   // QString idstring0;

    QByteArray data, cdata;
    QList<Variable> vars;

    void readData( DataFile& fp );
    void decompress();
    void repack();
};


class ARZ
{
public:
    ARZ();
    ~ARZ();

    static const int RD_POS = 1;
    static const int RD_SIZE = 2;
    static const int RD_CNT = 3;
    static const int ST_POS = 4;
    static const int ST_SIZE = 5;

    QString string( const int& idx ) const;
    int     stringIndex( const QString& string ) const;

    void doit();
    bool write( const QString& fname );
    bool    readArz();
    void    buildHelpers();
    int     decodeRecord( const int& ridx );

    // modding functions
    //
    void    maxPlayerLevel( const int& x = 50 );
    void    xmax( const int& x );
    void    flattenProxyWeights();
    void    inflateVariance();
    void    inflateVariance2();
    void    showVariable( const int& r, const int& v );
    void    showSubset( const int& r, const QList<int>& vtab );

protected:
    static const int HEAD_SIZE = 6;
    static const int FOOT_SIZE = 4;
    static const int SKIP_HEAD = 24;

    qint32 header[ HEAD_SIZE ];
    qint32 footer[ FOOT_SIZE ];

    DataFile    ifp;
    TextFile    cout;
    TextFile    log;
    Byter   byter;

    Record  *records;
    int     s_records;
    QStringList recordNames;
   // QStringList recordTemplates;

    QStringList     strings;
    int             s_strings;
    qint32          instringsCnt;

    QHash<QString,int>  rslt; // reverse string lookup table
    QHash<int,QList<int> > tlt; // template lookup table
};


#endif // ARZ_H
