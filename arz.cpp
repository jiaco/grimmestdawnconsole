#include "arz.h"

Record::Record()
{
    idstringIndex = -1;
    recordType = QString();
    dataOffset = dataCSize = 0;
    unk1 = unk2 = 0;
    idstring0 = QString();
    cdata = QByteArray();
    data = QByteArray();
}
Record::Record( DataFile& fp, int baseOffset )
{
    fp >> idstringIndex;
    recordType = Utility::ReadCString( fp );
    fp >> dataOffset;
    dataOffset += baseOffset;
    fp >> dataCSize;
    fp >> unk1;
    fp >> unk2;
    //idstring0 = Utility::ReadCString( fp );
}
void Record::doit( DataFile& fp, const QStringList& stab )
{
    if( idstring0.isEmpty() ) {
        lookupString0( stab );
    }
    if( cdata.size() == 0 ) {
        readData( fp );
    }
        decompress();
        decode();

}

void Record::show( QTextStream& fp )
{
    if( idstring0.isEmpty() ) {
        fp << "NO STRING";
    } else {
        fp << idstring0;
    }
    fp << "\t" << recordType << "\t" << dataOffset << "\t" << dataCSize << endl;
    fp << "C: " << cdata.size() << " vs D: " << data.size() << endl;
    fp << "//" << endl;
}

Record::~Record()
{
}
void Record::lookupString0( const QStringList& tab )
{
    if( idstringIndex >= 0 && idstringIndex < tab.size() ) {
        idstring0 = tab.at( idstringIndex );
    } else {
        idstring0 = QString();
    }
}

void Record::readData( DataFile& fp )
{
    cdata = Utility::ReadCompressed( fp, dataOffset, dataCSize );
}
void Record::decompress()
{
    data = Utility::Decompress( cdata );
}
void Record::decode()
{
    // make list of variables
}

ARZ::ARZ()
{
    cout.open( "stdout", QIODevice::WriteOnly );
    cout << "DEBUG" << endl;
}
ARZ::~ARZ()
{
    //coutfile.close();
}
/*
public void Decode(BinaryReader inReader, int baseOffset, ARZFile arzFile)
{
    // Record Entry Format
    // 0x0000 int32 stringEntryID (dbr filename)
    // 0x0004 int32 string length
    // 0x0008 string (record type)
    // 0x00?? int32 offset
    // 0x00?? int32 length in bytes
    // 0x00?? int32 timestamp?
    // 0x00?? int32 timestamp?
    m_idstringIndex = inReader.ReadInt32();
    m_recordType = ReadCString(inReader);

    m_offset = inReader.ReadInt32() + baseOffset;
    m_compressedSize = inReader.ReadInt32();
    m_crap1 = inReader.ReadInt32();
    m_crap2 = inReader.ReadInt32();

    // Get the ID string
    m_id = arzFile.Getstring(m_idstringIndex);
}*/

bool    ARZ::readRecordInfo(const qint32 &pos, const qint32& cnt )
{
    arz.seek( pos );
    for( int i = 0; i < cnt; ++i ) {
        records << Record( arz );
    }
    return( true );
}
bool    ARZ::readRecordData()
{
    for( int i = 0; i < records.size(); ++i ) {
        records[ i ].readData( arz );
    }
    return( true );
}
bool    ARZ::lookupString0s()
{
    for( int i = 0; i < records.size(); ++i ) {
        records[ i ].lookupString0( strings );
    }
    return( true );
}

bool    ARZ::decompressData()
{
    for( int i = 0; i < records.size(); ++i ) {
        records[ i ].decompress();
    }
    return( true );
}
bool    ARZ::readStringTable( const qint32& pos, const qint32& size )
{
    Q_UNUSED( size );
    arz.seek( pos );
    arz >> instringsCnt;

    for( int i = 0; i < instringsCnt; ++i ) {
        strings << Utility::ReadCString( arz );
    }
    return( true );
}


void    ARZ::doit(){
   // QString fname = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Grim Dawn\\database\\database.arz";
    QString fname = "D:\\Steam\\steamapps\\common\\Grim Dawn\\database\\database.arz";

    if( !arz.open( fname, QIODevice::ReadOnly ) ) {
        cout << "READ ERRROR: " << fname << endl;
    }

    cout << "DEBUG 2 " << endl;

    for( int i = 0; i < HEAD_SIZE; ++i ) {
        arz >> header[i];
        cout << "DEBUG [ " << i << " ] " << header[ i ] << endl;
    }
    readStringTable( header[ 4 ], header[ 5 ] );
    cout << "DEBUG string table contains " << instringsCnt << endl;

    readRecordInfo( header[ 1 ], header[ 2 ] );
    cout << "DEBUG record table info " << records.size() << endl;
    lookupString0s();
    cout << "DEBUG record table has strings " << records.size() << endl;

    for( int i = 0; i < records.size(); ++i ) {
        if( records.at(i).idstring0.contains( "records/proxies/lv" ) ) {
            cout << records.at(i).idstring0 << endl;
        }
    }

    QStringList types;
    for( int i = 0; i < records.size(); ++i ) {
        if( !types.contains( records.at(i).recordType ) ) {
            types << records.at(i).recordType;
        }
    }
    cout << "DEBUG types has " << types.size() << endl;
    /*
    foreach( QString s, types ) {
        cout << s << endl;
    }
    */
    if( types.contains( "prox" ) ) {
        cout << "Yes" << endl;
    }
    goto FINI;

    for( int i = 0; i < records.size() /2; ++i ) {
        cout << i << " " << records[i].recordType << " " << records[i].idstring0 << endl;
        if( records[i].idstring0.contains( "lvl1_weak") ) {
            cout << "RECORD AT " << i << endl;
            records[ i ].doit( arz, strings );
            records[ i ].show( cout );
            return;
        }
    }

    /*
    records[ 0 ].doit( arz, strings );
    records[ 0 ].show( cout );
    return;
    readRecordData();
    cout << "DEBUG record table data " << records.size() << endl;

    decompressData();
    cout << "DEBUG record table data decompressed " << records.size() << endl;

*/
    /*dbfile.setFileName( "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Grim Dawn\\database\\database.arz" );
    dbfile.open( QIODevice::ReadOnly );
    cout << "DEBUG file size = " << dbfile.size() << endl;
    dbfp.setDevice( &dbfile );
    dbfp.setByteOrder( QDataStream::LittleEndian );*/


    /*
   // arz.seek( header[ 1 ] );

    //dbfile.seek( header[1] );
   // qint32 idstringIndex;
    //dbfp >> idstringIndex;
    //QString recType;
    //cout << "DEBUG pos before " << dbfile.pos() << endl;

    //recType = Utility::ReadCString( dbfp );

    //cout << "DEBUG pos after " << dbfile.pos() << endl;

    //cout << "DEBUG " << idstringIndex << " '" << recType << "'" << endl;

    //qint32 offset, csize32;
    //int csize;
    //dbfp >> offset;
    //offset += 24;
    //dbfp >> csize32;
    //csize = csize32;

    //cout << "DEBUG offset " << offset << endl;
    //cout << "DEBUG csize " << csize << endl;

    //qint32  unknown1, unknown2;
    //dbfp >> unknown1;
    //dbfp >> unknown2;

    //cout << "DEBUG unknowns: " << unknown1 << " " << unknown2 << endl;

//
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
 //

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
/
   // cout << "DEBUG " << sp << endl;
   // cba = QByteArray::fromRawData( sp, csize );
   // ba = qUncompress( ba );



    dbfile.close();
*/
    FINI:;
    cout << "DONE" << endl;
}
