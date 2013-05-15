#ifndef ARZ_H
#define ARZ_H

#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QString>

#include "utility.h"

inline QString  ReadCString( QDataStream& fp, QTextStream& cout )
{
    QString rv;
    //qint32  len;

   // fp >> len;
   // cout << "DEBUG String length " << len << endl;

    uint rlen;
    char *sp;
    fp.readBytes( sp, rlen );

    cout << "DEBUG read length " << rlen << endl;
    //char *sp = new char[ len + 1 ];
    //fp.readRawData( sp, len + 1 );
    //rv.fromRawData( sp, len );
    cout << "DEBUG sp = " << sp << endl;
    rv = QString::fromLocal8Bit( sp );
    delete[] sp;
    return( rv );
}

class ARZ
{
public:
    ARZ();
    ~ARZ();
//    static const char* dbfilename = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Grim Dawn\\database";
    void doit();

    QFile       coutfile, dbfile;
    QDataStream dbfp;
    QTextStream cout;

    qint32 header[6];
};

#endif // ARZ_H
