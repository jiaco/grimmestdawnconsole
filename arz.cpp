#include "arz.h"

ARZ::ARZ()
{
    coutfile.open( stdout, QIODevice::WriteOnly );

    cout.setDevice( &coutfile );

    cout << "DEBUG" << endl;
}
ARZ::~ARZ()
{
    coutfile.close();
}

void    ARZ::doit(){
    cout << "DEBUG 2 " << endl;
    dbfile.setFileName( "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Grim Dawn\\database\\database.arz" );
    dbfile.open( QIODevice::ReadOnly );
    cout << "DEBUG file size = " << dbfile.size() << endl;
    dbfp.setDevice( &dbfile );
    dbfp.setByteOrder( QDataStream::LittleEndian );

    for( int i = 0; i < 6; ++i ) {
        dbfp >> header[i];
        cout << "DEBUG [ " << i << " ] " << header[ i ] << endl;

    }
    dbfile.seek( header[1] );
    qint32 idstringIndex;
    dbfp >> idstringIndex;
    QString recType;
    cout << "DEBUG pos before " << dbfile.pos() << endl;

    recType = ReadCString( dbfp, cout );

    cout << "DEBUG pos after " << dbfile.pos() << endl;

    cout << "DEBUG " << idstringIndex << " '" << recType << "'" << endl;

    qint32 offset, csize32;
    int csize;
    dbfp >> offset;
    offset += 24;
    dbfp >> csize32;
    csize = csize32;

    cout << "DEBUG offset " << offset << endl;
    cout << "DEBUG csize " << csize << endl;

    qint32  unknown1, unknown2;
    dbfp >> unknown1;
    dbfp >> unknown2;

    cout << "DEBUG unknowns: " << unknown1 << " " << unknown2 << endl;

    dbfile.seek( offset );
    QByteArray cba, ba;
    char* sp = new char[ csize ];
    int rsize = dbfp.readRawData( sp, csize );

    cout << "DEBUG read size = " << rsize << endl;

    cba.setRawData( sp, (uint)csize );
//    cba = QByteArray( sp );

    cout << "DEBUG cba size = " << cba.size() << endl;

    ba = Utility::Decompress( cba, cout );
    cout << "DEBUG uncompressed size: " << ba.size() << endl;

    // DecompressBytes method

    if( ba.size() % 4 != 0 ) {
        cout << "NOT GOING TO WORK" << endl;
    }
    int numDWords = ba.size() / 4;
    int numVariables = numDWords / 3;

    cout << "DEBUG DWords/Variables " << numDWords << " " << numVariables << endl;

    QDataStream bs( ba );
    bs.setByteOrder( QDataStream::LittleEndian );

    int i = 0;
    while(  i < numDWords ) {
        qint16 dataType, valCount;
        qint32 variableID;
        bs >> dataType;
        bs >> valCount;
        bs >> variableID;

        cout << "DEBUG variable " << dataType << " " << valCount << endl;
        break;

    }
    cba = Utility::Compress( ba,cout );

    cout << "DEBUG re-compressed data at = " << cba.size() << endl;
    /*

    QByteArray( bysize );
    bysize.fill( 0, 4 );
//    char bysize[4];
    unsigned long bylen = csize;
    bysize[0] = ( bylen >> 24 ) & 0xFF;
    bysize[1] = ( bylen >> 16 ) & 0xFF;
    bysize[2] = ( bylen >> 8 ) & 0xFF;
    bysize[3] = bylen & 0xFF;

    cba.prepend( bysize, 4 );

    ba = qUncompress( cba );
*/
   // cout << "DEBUG " << sp << endl;
   // cba = QByteArray::fromRawData( sp, csize );
   // ba = qUncompress( ba );



    dbfile.close();
}
