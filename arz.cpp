#include "arz.h"

// will need to use the old files to get at what we are expecting and
// hopefully use that to figure out how it has changed...
NormalPool::NormalPool()
{
    name = weight = levelVar = -1;
}
ChampionPool::ChampionPool()
{
    limit = minPClevel = -1;
}

//
//  Record class
//
Record::Record()
{
    idstringIndex = -1;
    recordType = QString();
    dataOffset = dataCSize = 0;
    unk1 = unk2 = 0;
    //idstring0 = QString();
    cdata = QByteArray();
    data = QByteArray();
}
Record::~Record()
{
}
void Record::readData( DataFile& fp )
{
    char *sp;
    sp = new char[ dataCSize ];
    fp.seek( dataOffset + 24 );
    fp.readRawData( sp, dataCSize );
    cdata.setRawData( sp, (uint)dataCSize );
    //cdata = Utility::ReadCompressed( fp, dataOffset + 24, dataCSize );
    delete[] sp;
}
void Record::decompress()
{
    //data = Utility::Decompress( cdata );
    data = cdata;
}
void    Record::repack()
{
    if( vars.size() > 0 ) {
        QByteArray  ndata;
        QDataStream dp( &ndata, QIODevice::WriteOnly );
        dp.setByteOrder( QDataStream::LittleEndian );
        qint32  vA;
        float   vB;
        for( int i = 0; i < vars.size(); ++i ) {
            dp << vars[i].dataType;
            dp << vars[i].valCount;
            dp << vars[i].nameIndex;
            for( int j = 0; j < vars[i].data.size(); ++j ) {
                switch( vars[i].dataType ) {
                case 0:
                case 2:
                case 3:
                    vA = vars[i].data[j].toInt();
                    dp << vA;
                    break;
                case 1:
                    vB = vars[i].data[j].toFloat();
                    dp << vB;
                    break;
                }
            }
        }
        data = ndata;
        //cdata = Utility::Compress( data );
        cdata = data;
        dataCSize = cdata.size();
    }

}

//
//  ARZ class
//
int ARZ::decodeRecord( const int& ridx )
{
    if( records[ridx].data.size() == 0 ) {
        if( records[ridx].cdata.size() == 0 ) {
            return( -1 );
        }
        records[ridx].decompress();
    }
    log << "DEBUG " << recordNames[ridx] <<
           " " << records[ridx].data.size() <<
           " (1) "  << records[ridx].unk1 <<
           " (2) " << records[ridx].unk2 <<
           " (3) " << records[ridx].unk3 << endl;
    log << "DEBUG unk1 a string ? " << string(records[ridx].unk1 ) << endl;

    // make list of variables
    QDataStream  dp( &records[ridx].data, QIODevice::ReadOnly );
    dp.setByteOrder( QDataStream::LittleEndian );


    // BEGIN
    Variable var;
    for( int i = 0; i < 2; ++i ) {
        dp >> var.dataType;
        dp >> var.valCount;
        dp >> var.nameIndex;
        log << "DEBUG var " << var.dataType << " " << var.valCount
            << " " << var.nameIndex << " "
              // << string( var.nameIndex )
               << endl;
    }
    return( 0 );
    // END
    int numDWords = records[ridx].data.size() / 4;
    records[ridx].vars.clear();
    records[ridx].vars.reserve( numDWords / 3 );


    if( numDWords < 0 ) {
        log << "huh?" << endl;
        return( 0 );
    }
    qint16 vs;
    qint32 vA;
    for( int i = 1; i < numDWords && i < 5; ++i ) {
        dp >> vA;
        log << i << " : " << vA << endl;
    }
    float    vB;
    int i = 0;
    while( i >= 0 &&  i < numDWords ) {
         Variable var;
         dp >> var.dataType;
         dp >> var.valCount;
         dp >> var.nameIndex;
         log << " DEBUG " << i << " / " << numDWords << " " << string( var.nameIndex ) << " " << var.dataType << " " << var.valCount << endl;
         i += ( 2 + var.valCount );
         for( int j = 0; j < var.valCount; ++j ) {
             switch(var.dataType ) {
             case 0: // integer
             case 2: // string
             case 3: // bool
                 dp >> vA;
                 var.data << QVariant( vA );
                 break;
             case 1: // float
                    dp >> vB;
                    var.data << QVariant( vB );
                    break;
             default:
                 break;
             }
         }
         records[ridx].vars << var;
     }
    return( records[ridx].vars.size() );
}

ARZ::ARZ()
{
    cout.open( "stdout", QIODevice::WriteOnly );
    records = NULL;
    s_records = 0;
}
ARZ::~ARZ()
{
}
QString ARZ::string( const int& idx ) const
{
    if( idx >= 0 ) {
        if( idx < s_strings ) {
            return( strings[idx] );
        }
    }
    return( "" );
}
int ARZ::stringIndex( const QString& s ) const
{
    if( rslt.contains( s ) ) {
        return( rslt[ s ] );
    } else {
        return( -1 );
    }
}
void ARZ::doit()
{
    QString driveLetter = "D:";
    QString dbLocation = "Steam\\steamapps\\common\\Grim Dawn\\database";
    //QString dbName = "stock_b9_database.arz";
    QString dbName = "database.arz";
    QString outName = "modded_database.arz";
    QString logName = "modded_log.txt";
    QString dbPath = QString( "%1\\%2" ).arg( driveLetter ).arg( dbLocation );

    QString ifname = QString( "%1\\%2" ).arg( dbPath ).arg( dbName );
    QString lfname = QString( "%1\\%2" ).arg( dbPath ).arg( logName );
    QString ofname = QString( "%1\\%2" ).arg( dbPath ).arg( outName );

    if( !ifp.open( ifname, QIODevice::ReadOnly ) ) {
        cout << "ERROR: failed to read " << ifname << endl;
        return;
    }
    if( !log.open( lfname, QIODevice::WriteOnly ) ) {
        cout << "ERROR: failed to write " << lfname << endl;
    }
    cout << "DEBUG send to readArz" << endl;
    readArz();
    buildHelpers();
   // maxPlayerLevel( 50 );
   // xmax( 2 );
    flattenProxyWeights();
  // inflateVariance2();

   //write( ofname );
}

bool    ARZ::readArz()
{
    // HEADER : First 24 bytes are a 6 qint32 header
    //
    for( int i = 0; i < HEAD_SIZE; ++i ) {
        ifp >> header[i];
        cout << "DEBUG " << header[i] << endl;
    }
    // STRING TABLE get this first
    //
    ifp.seek( header[ST_POS] );
    ifp >> s_strings;
    strings.reserve( s_strings );
    for( int i = 0; i < s_strings; ++i ) {
        strings << byter.GetCString( ifp );
       // log << i << "\t" << strings[i] << endl;
    }
    cout << "DEBUG " << strings.size() << " strings " << header[ST_POS] << " " << header[ST_SIZE] << endl;
    //
    // After header is compressed record data, but need info first
    //
    ifp.seek( header[RD_POS] );
    s_records = header[RD_CNT];
    records = new Record[ s_records ];
    for( int i = 0; i < s_records; ++i ) {
        ifp >> records[i].idstringIndex;
        records[i].recordType = byter.GetCString( ifp );
        ifp >> records[i].dataOffset;
        ifp >> records[i].dataCSize;
        ifp >> records[i].unk1;
        ifp >> records[i].unk2;
        ifp >> records[i].unk3;
        //cout << "DEBUG unk " << records[i].unk1 << " " << records[i].unk2 << " " << records[i].unk3 << endl;
    }
    cout << "DEBUG " << header[RD_CNT] <<  " Records " <<  header[RD_POS] + header[RD_SIZE] << " == " << ifp.pos() << endl;

   // SHOULD BE AT FOOTER AFTER STRING TABLE
    //
    // 4 qint32 with first being string table size count ??
   //
   ifp.seek( header[ST_POS] + header[ST_SIZE] );
   for( int i = 0; i < FOOT_SIZE; ++i ) {
       ifp >> footer[i];
       cout << "DEBUG FOOTER " << i << " " << footer[i] << endl;
   }
   cout << "DEBUG endof file pos " << ifp.pos() << endl;
   cout << "DEBUG file size is: " << ifp.size() << endl;

   qint16 i16;
   ifp.seek( ifp.size() - 16 );
   for( int i = 0; i < 8; ++i ) {
       ifp >> i16;
       cout << "DEBUG footer in 16 bit ints : " << i << " " << i16 << endl;
   }

   // Now can go back and read each record data
   //
   recordNames.clear();
   recordNames.reserve( s_records );
   for( int i = 0; i < s_records; ++i ) {
       //cout << "DEBUG " << i << " / " << s_records << " " << records[i].dataOffset << " " << string(records[i].idstringIndex ) << endl;

       if( records[i].dataOffset < 0 || ( records[i].dataOffset + records[i].dataCSize ) > ifp.size() ) {
           cout << "ERROR skip " << recordNames[i] << " bad offset" << endl;
           continue;
       }
       records[i].readData( ifp );
       recordNames << string( records[i].idstringIndex );
       // no need to decompress everything
      // records[i].decompress();
   }
   cout << "DEBUG ARZ::readFile() completed" << endl;
   return( true );
}
void    ARZ::buildHelpers()
{
    for( int i = 0; i < s_strings; ++i ) {
        rslt.insert( strings[i], i );
    }
    /*
    int tmpIdx = stringIndex( "templateName" );
    for( int i = 0; i < s_records; ++i ) {
        cout << "DEBUG decoding record " << i << " / " << s_records << endl;
        decodeRecord( i );
        for( int j = 0; j < records[i].vars.size(); ++j ) {
            if( records[i].vars[j].nameIndex == tmpIdx && records[i].vars[j].data.size() > 0 ) {
                int sidx = records[i].vars[j].data.at(0).toInt();
                // while data.size() can be zero, it makes no sense to templateName to be an array

                if( tlt.contains( sidx ) ) {
                    tlt[ sidx ] << i;
                } else {
                    QList<int> t;
                    t << i;
                    tlt.insert( sidx, t );
                }
            }
        }
    }*/
}
//  modding functions
//
void    ARZ::xmax( const int& x )
{
    // in records/proxies/proxypoolequation_01.dbr
    // can apply xmax by just changing poolValue * 1 to poolValue * X
    //
    // should check to make sure that that dbr is the only one referencing this string
    //
    strings[ stringIndex( "poolValue * 1" ) ] = QString( "poolValue * %1" ).arg( x );
}
void    ARZ::maxPlayerLevel( const int& x )
{
    cout << "DEBUG find maxPlayer level" << endl;
    int     tgt = stringIndex( "maxPlayerLevel" );
    if( tgt == -1 ) {
        cout << "DEBUG not found" << endl;
        return;
    }
    cout << "maxPlayerLevel " << tgt << endl;
    return;
    for( int r = 0; r < recordNames.size(); ++r ) {
        if( recordNames[r] != "records/creatures/pc/playerlevels.dbr" ) {
            continue;
        }
        records[r].decompress();
        decodeRecord( r );
        for( int j = 0; j < records[r].vars.size(); ++j ) {
            if( records[r].vars[j].nameIndex == tgt ) {
                records[r].vars[j].data[0] = QVariant( x );
            }
        }
    }
}

void    ARZ::inflateVariance()
{
    QRegExp rx( "\\(\\(averagePlayerLevel\\+\\(averagePlayerLevel\\/([0-9]+)\\)\\)([+-][0-9]+)\\)");
    for( int i = 0; i < s_strings; ++i ) {
        if( strings[i].startsWith( "((averagePlayerLevel" ) ) {
            QString s = strings[i];
            if( rx.indexIn( s ) != -1 ) {
                QChar   fillch = '0';
                QString s1 = rx.cap( 1 );
                int t1 = s1.toInt();
                int fw = s1.size();
                int t2 = rx.cap( 2 ).toInt();
                //cout << t1 << " " << t2 << " ";
                t1 -= 50;
                t2 += ( t2 / 2 );
                if( t2 <= 0 ) {
                    t2 = 1;
                }

                if( t1 <= 0 ) {
                    t1 = 10;
                    t2 += 1;
                }
                cout << i << "\t" << strings[i] << endl;

                s = QString( "((averagePlayerLevel+(averagePlayerLevel/%1))+%2)*(0.99+(numberOfPlayers/100))" ).arg( t1, fw, 10, fillch  ).arg( t2 );
                cout << i << "\t" << s << endl;
                strings[i] = s;
            }


        }
    }
}
void    ARZ::inflateVariance2()
{

    QStringList dbrs;
    dbrs << "lv1_weak" << "lv1_weak+" << "lv2_normal" << "lv2_normal+"
         << "lv3_strong" << "lv3_strong+" << "lv4_champion" << "lv4_champion+"
         << "lv5_elitechampion" << "lv5_elitechampion+"
         << "lv6_hero" << "lv6_hero+"
         << "lv7_uber hero" << "lv7_uber hero+"
         << "lv8_boss" << "lv8_boss+";
    int s_dbrs = dbrs.size();

    QList<int> dbrIdxs;

    for( int i = 0; i < s_dbrs; ++i ) {
        dbrs[i] = QString( "records/proxies/%1.dbr" ).arg( dbrs[i] );
        dbrIdxs << stringIndex( dbrs[i] );
        cout << dbrIdxs[i] << "\t" << dbrs[i] << endl;
    }
    /*
    for( int i = 0; i < s_strings; ++i ) {
        if( strings[i].startsWith( "records/proxies/lv" ) ) {
            for( int j = 0; j < s_dbrs; ++j ) {
                if( strings[i] == dbrs[j ] ) {
                    dbrIdxs[ j ] = i;
                    break;
                }
            }
        }
    }*/
    /*
    int tgt = stringIndex( "minPlayerLevelEquationNormal" );
    for( int i = 0; i < s_records; ++i ) {
        if( recordNames[i] == "records/proxies/limit_area001.dbr" ) {
            records[i].decompress();
            decodeRecord( i );
            for( int j = 0; j < records[i].vars.size(); ++j ) {
                if( records[i].vars[j].nameIndex == tgt ) {
                    records[i].vars[j].data[0] = QVariant( 5 );
                }
            }

        }
    }*/
}

void    ARZ::flattenProxyWeights()
{
    // first get list of records in records/proxies
    //
    QList<int> proxyRecords;
    for( int r = 0; r < recordNames.size(); ++r ) {
        if( recordNames[r].startsWith( "records/proxies/" ) ) {
            proxyRecords << r;
        }
    }
    log << "Found " << proxyRecords.size() << " out of " << recordNames.size() << " proxies" << endl;

    // send to log file a list of the current variance types:
    //
    foreach( int r, proxyRecords ) {
        if( recordNames[r].startsWith( "records/proxies/lv" ) ) {
            log << r << "\t" << recordNames[r] << endl;
        }
    }

    QStringList dbrs;
    dbrs << "lv1_weak" << "lv1_weak+" << "lv2_normal" << "lv2_normal+"
         << "lv3_strong" << "lv3_strong+" << "lv4_champion" << "lv4_champion+"
         << "lv5_elitechampion" << "lv5_elitechampion+"
         << "lv6_hero" << "lv6_hero+"
         << "lv7_uber hero" << "lv7_uber hero+"
         << "lv8_boss" << "lv8_boss+";
    int s_dbrs = dbrs.size();

    QList<int> dbrIdxs;

    for( int i = 0; i < s_dbrs; ++i ) {
        dbrs[i] = QString( "records/proxies/%1.dbr" ).arg( dbrs[i] );
        dbrIdxs << stringIndex( dbrs[i] );
        cout << dbrIdxs[i] << "\t" << dbrs[i] << endl;
    }
    QMap<int,int>   lvchanger;

    for( int i = 0; i < s_dbrs - 1; ++i ) {
        lvchanger.insert( dbrIdxs[i], dbrIdxs[i+1] );
    }
    lvchanger.insert( dbrIdxs[ s_dbrs - 1 ], dbrIdxs[ s_dbrs-1 ] );
    //
    // database variables to modify:
    //
    // proxy
    // records/proxies/area001
    //  pool# : weight#
    //

    // proxypool
    // records/proxies/pools
    //  -spawnMin
    //  -championMin
    //  -name#
    //      weight#
    //      levelVarianceEquation#

    //  -nameChampion#
    //      weightChampion#
    //      limitChampion#
    //      levelVarianceEquationChampion#
    //      minPlayerLevelChampion#
    //
    QStringList nbaseflds;
    nbaseflds << "name" << "weight" << "levelVarianceEquation";
    QStringList cbaseflds;
    cbaseflds << "nameChampion" << "weightChampion" << "limitChampion" << "levelVarianceEquationChampion" << "minPlayerLevelChampion";


    QStringList flds;
    QMap<QString,int> fldToIdx;
    QStringList nflds;
    QMap<QString,int> nfldToIdx;
    QStringList cflds;
    QMap<QString,int> cfldToIdx;

    flds << "spawnMin";
    flds << "spawnMax";
    flds << "championMin";
    for( int i = 1; i < 16; ++i ) {
        foreach( QString base, nbaseflds ) {
            QString s = QString( "%1%2" ).arg( base ).arg( i );
            if( strings.contains( s ) ) {
                nflds << s;
            }
        }
        foreach( QString base, cbaseflds ) {
            QString s = QString( "%1%2" ).arg( base ).arg( i );
            if( strings.contains( s ) ) {
                cflds << s;
            }
        }
    }
    flds << nflds;
    flds << cflds;
    foreach( QString s, flds ) {
        fldToIdx.insert( s, stringIndex( s ) );
    }
    foreach( QString s, nflds ) {
        nfldToIdx.insert( s, stringIndex( s ) );
    }
    foreach( QString s, cflds ) {
        cfldToIdx.insert( s, stringIndex( s ) );
    }
    QRegExp backInt = QRegExp( "\\d+$" );
    // since template field is possibly unset try using path:
    //
    cout << "DEBUG records to search: " << proxyRecords.size() << endl;
    foreach( int r, proxyRecords ) {
        if( !recordNames[r].startsWith( "records/proxies/pools/" ) ) {
            continue;
        }
        records[r].decompress();
        decodeRecord( r );
        BasePool    bp;
        NormalPool   np[16];
        ChampionPool cp[16];
        int npool;

        log << "DEBUG " << records[r].vars.size() << endl;
        // by putting all the weights to 100, it does not seem to help matters
        // want to check if > 50 reduce and if < 50 then increase
        //
        for( int j = 0; j < records[r].vars.size(); ++j ) {
            if( records[r].vars[j].data.size() == 0 ) {
                continue;
            }
            QString fieldName = string( records[r].vars[j].nameIndex );
            if( !flds.contains( fieldName ) ) {
                continue;
            }
            if( records[r].vars[j].nameIndex == fldToIdx[ "spawnMin" ] ) {
                bp.spawnMin = records[r].vars[j].data.at(0).toInt();
                bp.spawnMinIdx = j;
                continue;
            }
            if( records[r].vars[j].nameIndex == fldToIdx[ "spawnMax" ] ) {
                bp.spawnMax = records[r].vars[j].data.at(0).toInt();
                bp.spawnMaxIdx = j;
                continue;
            }
            if( records[r].vars[j].nameIndex == fldToIdx[ "championMin" ] ) {
                bp.championMin = records[r].vars[j].data.at(0).toInt();
                bp.championMinIdx = j;
                continue;
            }
            // now we are either in a field for name or champion
            //
            if( nflds.contains( fieldName ) ) {
                int pos = backInt.indexIn( fieldName );
                if( pos > 0 ) {
                    npool = backInt.cap(0).toInt();
                    if( fieldName.startsWith( "name" ) ) {
                        np[ npool ].name = records[r].vars[j].data[0].toInt();
                        np[ npool ].nameIdx = j;
                    } else if( fieldName.startsWith( "weight" ) ) {
                        np[ npool ].weight = records[r].vars[j].data[0].toInt();
                        np[ npool ].weightIdx = j;
                    } else if( fieldName.startsWith( "levelVarianceEquation" ) ) {
                        np[ npool ].levelVar = records[r].vars[j].data[0].toInt();
                        np[ npool ].levelVarIdx = j;
                    }
                }
                continue;
            }
            if( cflds.contains( fieldName ) ) {
                int pos = backInt.indexIn( fieldName );
                if( pos > 0 ) {
                    npool = backInt.cap(0).toInt();
                    if( fieldName.startsWith( "nameChampion" ) ) {
                        cp[ npool ].name = records[r].vars[j].data[0].toInt();
                        cp[ npool ].nameIdx = j;
                    } else if( fieldName.startsWith( "weightChampion" ) ) {
                        cp[ npool ].weight = records[r].vars[j].data[0].toInt();
                        cp[ npool ].weightIdx = j;
                    } else if( fieldName.startsWith( "levelVarianceEquationChampion" ) ) {
                        cp[ npool ].levelVar = records[r].vars[j].data[0].toInt();
                        cp[ npool ].levelVarIdx = j;
                    } else if( fieldName.startsWith( "limitChampion" ) ) {
                        cp[ npool ].limit = records[r].vars[j].data[0].toInt();
                        cp[ npool ].limitIdx = j;
                    } else if( fieldName.startsWith( "minPlayerLevelChampion" ) ) {
                        cp[ npool ].minPClevel = records[r].vars[j].data[0].toInt();
                        cp[ npool ].minPClevelIdx = j;
                    }
                }
            }
        }
        if( bp.spawnMax > 1 ) {
            log << "Modding " << recordNames[r] << " spawns " << bp.spawnMin << " -> " << bp.spawnMax << " champs " << bp.championMin << endl;
            bp.spawnMin += 4;
            bp.spawnMax += 6;
            bp.championMin += 2;

            records[r].vars[bp.spawnMinIdx].data[0] = QVariant( bp.spawnMin );
            records[r].vars[bp.spawnMaxIdx].data[0] = QVariant( bp.spawnMax );
            records[r].vars[bp.championMinIdx].data[0] = QVariant( bp.championMin );
        } else {
            log << "Skipping " << recordNames[r] << " due to spawnMax == 1" << endl;
            continue;
        }

        int ncnt = 0;
        int wsum = 0;
        int nweight = 0;
        for( int i = 1; i < 16; ++i ) {
            if( np[i].name == -1 ) {
                continue;
            }
            ncnt += 1;
            wsum += np[i].weight;
        }
        if( ncnt > 0 ) {
            log << "Normal Have " << ncnt << " entries with wsum = " << wsum << endl;
            nweight = wsum / ncnt;

            for( int i = 1; i < 16; ++i ) {
                if( np[i].name == -1 ) {
                    continue;
                }
                records[r].vars[ np[i].weightIdx ].data[0] = QVariant( nweight );
                records[r].vars[ np[i].levelVarIdx ].data[0] = QVariant( lvchanger[ np[i].levelVar ] );

               log << "NORMAL POOL " << string( np[i].name ) << "\t" << np[i].weight << "\t" << string( np[i].levelVar ) <<  endl;
               log << "UPGRADED TO " << nweight << "\t" << string( lvchanger[ np[i].levelVar ] ) << endl;
            }
        }
        ncnt = 0;
        wsum = 0;
        nweight = 0;
        for( int i = 1; i < 16; ++i ) {
            if( cp[i].name == -1 ) {
                continue;
            }
            ncnt += 1;
            if( cp[i].weight == -1 ) {
                log << "THIS RECORD HAS name " << string( cp[i].nameIdx ) << " but no weight" << endl;
            } else {
                wsum += cp[i].weight;
            }
        }
        if( ncnt > 0 ) {
            log << "Champs Have " << ncnt << " entries with wsum = " << wsum << endl;
            nweight = wsum / ncnt;
            for( int i = 1; i < 16; ++i ) {
                if( cp[i].name == -1 ) {
                    continue;
                }
                if( cp[i].weight != -1 ) {
                    log << "CHAMP POOL " << string( cp[i].name ) << " weight " << cp[i].weight << " set to " << nweight << endl;
                    records[r].vars[ cp[i].weightIdx ].data[0] = QVariant( nweight );
                }
                if( cp[i].minPClevel != -1 ) {
                    int lvl = records[r].vars[cp[i].minPClevelIdx ].data.at( 0 ).toInt();
                    lvl -= ( lvl / 10 ) + 1;
                    log << "      minPC " << cp[i].minPClevel << " set to " << lvl << endl;

                    records[r].vars[cp[i].minPClevelIdx ].data[ 0 ] = QVariant( lvl );

                }
            }
        }
        log << endl;
    }
    cout << "DEBUG end of method" << endl;
}
        /*
        // now inside of records[r].var[j].nameIndex is an int and we
        // want to know which of those ints are in variables of interest
        int vNameNormal, vWeightNormal, vLvlvarNormal;
    int nGot = 0;
        cout << "DEBUG modding " << recordNames[r] << endl;
        for( int j = 0; j < records[r].vars.size(); ++j ) {
            if( records[r].vars[j].nameIndex == nameNormal[ 0 ] ) {
                vNameNormal = j;
                ++nGot;
            }
            if( records[r].vars[j].nameIndex == weightNormal[ 0 ] ) {
                vWeightNormal = j;
                ++nGot;
            }
            if( records[r].vars[j].nameIndex == lvlvarNormal[ 0 ] ) {
                vLvlvarNormal = j;
                ++nGot;
            }
            if( nGot == 3 ) {
            QList<int> vset;
            vset << vNameNormal << vWeightNormal << vLvlvarNormal;
            showSubset( r, vset );
            }
        }
        */
/*
   QString proxytpl = "database/templates/proxy.tpl";
   int     proxysid = stringIndex( proxytpl );

   QList<int>   proxyRecords;
   QList<int>   poolFields, weightFields;

   for( int i = 0; i < s_strings; ++i ) {
       if( strings[i].startsWith( "pool" ) ) {
           poolFields << i;
       }
       if( strings[i].startsWith( "weight" ) ) {
           weightFields << i;
       }
   }
   foreach( int i, poolFields ) {
       cout << i << "\t" << string(i) << endl;
   }
   foreach( int i, weightFields ) {
       cout << i << "\t" << string(i) << endl;
   }

   if( !tlt.contains( proxysid ) ) {
       return;
   }
   foreach( int i, tlt[ proxysid ] ) {
        // area001 proxies do not typically have multiple entries
       if( !records[i].idstring0.startsWith( "records/proxy/area001" ) ) {

        continue;
    }
    for( int j = 0; j < records[i].vars.size(); ++j ) {
        if( poolFields.contains( records[i].vars[j].nameIndex ) ) {
            cout << string( records[i].vars[j].nameIndex ) << "\t" <<
                    string( records[i].vars[j].data.at(0).toInt() ) << endl;
        }
        if( weightFields.contains( records[i].vars[j].nameIndex ) ) {
            cout << string( records[i].vars[j].nameIndex ) << "\t" <<
                    records[i].vars[j].data.at(0).toInt() << endl;
        }
    }

   }
*/
   /*

   QString ofname = "D:\\Steam\\steamapps\\common\\Grim Dawn\\database\\mod_database.arz";
   cout << "DEBUG writing" << endl;
   write( ofname );
    */

void    ARZ::showVariable( const int& r, const int& j )
{
    cout << string( records[r].vars[j].nameIndex );
    cout << "\t";
    if( records[r].vars[j].data.size() > 0 ) {
        switch( records[r].vars[j].dataType ) {
          case 0:
        case 3:
            cout << records[r].vars[j].data.at(0).toInt();
            break;
          case 1:
            cout << records[r].vars[j].data.at(0).toFloat();
            break;
        case 2:
            cout << string( records[r].vars[j].data.at(0).toInt() );
            break;


        }
    }
    cout << endl;
}

void    ARZ::showSubset( const int& r, const QList<int>& vtab )
{
    foreach( int j, vtab ) {
        showVariable( r, j );
    }
}
 /*
   QString target = "records/proxies/proxypoolequation_01.dbr";

   for( int i = 0; i < s_strings; ++i ) {
       rslt.insert( strings[i], i );
   }
   int tgtIndex = rslt[ target ];
   cout << "DEBUG found " << target << " at " << tgtIndex << endl;

    int tgtRecord = -1;


   for( int i = 0; i < s_records; ++i ) {
       if( records[i].idstringIndex == tgtIndex ) {
           tgtRecord = i;
       }
       records[i].idstring0 = string(records[i].idstringIndex );
   }
   cout << "DEBUG strings assigned" << endl;

   cout << "DEBUG target found? " << tgtRecord << endl;
*/

/*
   int r = tgtRecord;
   QDataStream  dp( &records[r].data, QIODevice::ReadOnly );
   dp.setByteOrder( QDataStream::LittleEndian );
   int numDWords = records[r].data.size() / 4;
   //int numVariables = numDWords / 3;

   QList<Variable> vtab;

   cout << "DEBUG begin numDWords = " << numDWords << " " << numDWords / 3 << endl;
   qint32 vA;
   float    vB;
    int i = 0;
    int pos = 0;
    while( i < numDWords ) {
        Variable var;
        dp >> var.dataType;
        dp >> var.valCount;
        dp >> var.nameIndex;
        var.name = string( var.nameIndex );

        //variableName = string( variableId );
        i += ( 2 + var.valCount );
        for( int j = 0; j < var.valCount; ++j ) {
            switch(var.dataType ) {
            case 0: // integer
            case 2: //string
            case 3: // bool
                dp >> vA;
                var.data << QVariant( vA );
                break;
            case 1: // float
                   dp >> vB;
                   var.data << QVariant( vB );
                   break;
            case 4: // huh
                cout << " huh ? " << var.dataType << endl;
                break;
            default:
                cout << "Fell off the edge with " << var.dataType << " !" <<endl;
                goto FAIL;
                break;
            }
        }
        vtab << var;
       // cout << "DEBUG " << i << " " << numDWords << " ( " << variableId << " ) "  << variableName << endl;
    }
    FAIL:;
    cout << "DEBUG vtab size = " << vtab.size() << endl;

    for( int i = 0; i < vtab.size(); ++i ) {
        cout << vtab[i].name << "\t";
        switch( vtab[i].dataType ) {
        case    0:
        case    3:
            cout << vtab[i].data[0].toInt();
            for( int j = 1; j < vtab[i].data.size(); ++j ) {
                cout << "," << vtab[i].data[j].toInt();
            }
            break;
        case 2:
            cout << string( vtab[i].data[0].toInt() );
            for( int j = 1; j < vtab[i].data.size(); ++j ) {
                cout << "," << string( vtab[i].data[j].toInt() );
            }
            break;
        case 1:cout << vtab[i].data[0].toFloat();
            for( int j = 1; j < vtab[i].data.size(); ++j ) {
                cout << "," << vtab[i].data[j].toFloat();
            }
            break;
        }
        cout << endl;
    }
    cout << "DEBUG out of vtab loop" << endl;
}
*/
bool    ARZ::write( const QString& fname )
{
    qint32 newHeader[ HEAD_SIZE ];

       DataFile fp;
       if( !fp.open( fname, QIODevice::WriteOnly ) ) {
           return( false );
       }
       for( int i = 0; i < HEAD_SIZE; ++i ) {
           fp << header[ i ];
           newHeader[ i ] = header[ i ];
       }
       for( int i = 0; i < s_records; ++i ) {
           records[i].repack();
           records[i].dataOffset = fp.pos() - 24;
           fp.writeRawData( records[i].cdata, records[i].dataCSize );
       }
       newHeader[ RD_POS ] = fp.pos();

       for( int i = 0; i < s_records; ++i ) {
           fp << records[i].idstringIndex;
           fp.writeBytes( records[i].recordType.toUtf8(), records[i].recordType.size() );
           fp << records[i].dataOffset;
           fp << records[i].dataCSize;
           fp << records[i].unk1;
           fp << records[i].unk2;
       }
       newHeader[ RD_CNT ] = s_records;
       newHeader[ RD_SIZE ] = fp.pos() - newHeader[ RD_POS ];

       newHeader[ ST_POS ] = fp.pos();
       fp << (qint32)s_strings;
       for( int i = 0; i < s_strings; ++i ) {
           fp.writeBytes( strings[i].toUtf8(), strings[i].size() );
       }
       newHeader[ ST_SIZE ] = fp.pos() - newHeader[ ST_POS ];
       cout << "CHANGE IN STRING TABLE " << header[ ST_SIZE ] << " -> " << newHeader[ ST_SIZE ] << endl;
       for( int i = 0; i < FOOT_SIZE; ++i ) {
           fp << footer[ i ];
       }

       fp.seek( 0L );
       for( int i = 0; i < HEAD_SIZE; ++i ) {
           fp << newHeader[i];
       }
       return( true );
}

  /*  readStringTable( header[ 4 ], header[ 5 ] );
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
    cout << "DEBUG types has " << types.size() << endl;*/
    /*
    foreach( QString s, types ) {
        cout << s << endl;
    }
    */
    /*if( types.contains( "prox" ) ) {
        cout << "Yes" << endl;
    }*/
/*
    for( int i = 0; i < records.size() /2; ++i ) {
        cout << i << " " << records[i].recordType << " " << records[i].idstring0 << endl;
        if( records[i].idstring0.contains( "lvl1_weak") ) {
            cout << "RECORD AT " << i << endl;
            records[ i ].doit( arz, strings );
            records[ i ].show( cout );
            return;
        }
    }
*/
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
