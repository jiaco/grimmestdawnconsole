#include "arz.h"

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
    cdata = Utility::ReadCompressed( fp, dataOffset + 24, dataCSize );
}
void Record::decompress()
{
    data = Utility::Decompress( cdata );
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
        cdata = Utility::Compress( data );
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
    // make list of variables
    QDataStream  dp( &records[ridx].data, QIODevice::ReadOnly );
    dp.setByteOrder( QDataStream::LittleEndian );

    int numDWords = records[ridx].data.size() / 4;
    records[ridx].vars.clear();
    records[ridx].vars.reserve( numDWords / 3 );

    qint32 vA;
    float    vB;
    int i = 0;
    while( i < numDWords ) {
         Variable var;
         dp >> var.dataType;
         dp >> var.valCount;
         dp >> var.nameIndex;
         //var.name = string( var.nameIndex );
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
    // QString fname = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Grim Dawn\\database\\database.arz";
    QString fname = "D:\\Steam\\steamapps\\common\\Grim Dawn\\database\\stock_database.arz";

   if( !openInputFile( fname ) ) {
       cout << "ERROR reading input file " << fname << endl;
       return;
   }
   readArz();

   buildHelpers();
   //xmax( 2 );
   flattenProxyWeights();
  // inflateVariance2();

   QString ofname = fname;
   ofname.replace( "stock_", "modded_" );
   write( ofname );
}

bool    ARZ::openInputFile( const QString& fname ){
    return( arz.open( fname, QIODevice::ReadOnly ) );
}
bool    ARZ::readArz()
{
    // HEADER
    //
    for( int i = 0; i < HEAD_SIZE; ++i ) {
        arz >> header[i];
    }
//    cout << "DEBUG RD count " << header[RD_CNT] << " pos " << header[RD_POS] << " ( " << header[RD_SIZE] << " )" << endl;
//    cout << "DBEUG ST pos " << header[ST_POS] << " ( " << header[ST_SIZE] << " )" << endl;

    // After header is record data, but need info first
    //
    s_records = header[RD_CNT];
    records = new Record[ s_records ];

    arz.seek( header[RD_POS] );
    for( int i = 0; i < s_records; ++i ) {
        arz >> records[i].idstringIndex;
        records[i].recordType = byter.GetCString( arz );
        arz >> records[i].dataOffset;
        arz >> records[i].dataCSize;
        arz >> records[i].unk1;
        arz >> records[i].unk2;
    }
    cout << "DEBUG " <<  header[RD_POS] + header[RD_SIZE] << endl;
    cout << "DEBUG arz.pos " << arz.pos() << " " << endl;

    // STRING TABLE
    //
    cout << "DEBUG seek " << header[ST_POS] << endl;
    arz.seek( header[ST_POS] );
    arz >> s_strings;
    strings.reserve( s_strings );
    for( int i = 0; i < s_strings; ++i ) {
        strings << byter.GetCString( arz );
    }
   cout << "DEBUG read " << strings.size() << " strings" << endl;

   //SHOULD BE AT FOOTER AFTER STRING TABLE
   //
   for( int i = 0; i < FOOT_SIZE; ++i ) {
       arz >> footer[i];
       cout << "DEBUG FOOT " << i << " " << footer[i] << endl;
   }
   cout << "DEBUG endof file pos " << arz.pos() << endl;

   // Now can go back and read each record data
   //
   recordNames.clear();
   recordNames.reserve( s_records );
   for( int i = 0; i < s_records; ++i ) {
       records[i].readData( arz );
       recordNames << string( records[i].idstringIndex );
      // records[i].decompress();
   }
   cout << "DEBUG record data read" << endl;
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
    cout << "DEBUG records to search: " << recordNames.size() << endl;
    for( int r = 0; r < s_records; ++r ) {
        if( !recordNames[r].startsWith( "records/proxies/pools/" ) ) {
            continue;
        }
        records[r].decompress();
        decodeRecord( r );
        NormalPool   np[16];
        ChampionPool cp[16];
        int npool;

        cout << "DEBUG " << recordNames[r] << endl;
        for( int j = 0; j < records[r].vars.size(); ++j ) {
            if( records[r].vars[j].data.size() == 0 ) {
                continue;
            }
            QString fieldName = string( records[r].vars[j].nameIndex );
            if( !flds.contains( fieldName ) ) {
                continue;
            }
            if( records[r].vars[j].nameIndex == fldToIdx[ "spawnMin" ] ) {
                int v = records[r].vars[j].data.at(0).toInt();
                v += 4;
                records[r].vars[j].data[0] = QVariant( v );
                continue;
            }
            if( records[r].vars[j].nameIndex == fldToIdx[ "spawnMax" ] ) {
                int v = records[r].vars[j].data.at(0).toInt();
                v += 6;
                records[r].vars[j].data[0] = QVariant( v );
                continue;
            }
            if( records[r].vars[j].nameIndex == fldToIdx[ "championMin" ] ) {
                int v = records[r].vars[j].data.at(0).toInt();
                v += 2;
                records[r].vars[j].data[0] = QVariant( v );
                continue;
            }
            // now we are either in a field for name or champion
            //
            if( nflds.contains( fieldName ) ) {
                int pos = backInt.indexIn( fieldName );
                if( pos > 0 ) {
                    npool = backInt.cap(0).toInt();
                    if( fieldName.startsWith( "nameChampion" ) ) {
                        np[ npool ].name = records[r].vars[j].data[0].toInt();
                        np[ npool ].nameIdx = j;
                    } else if( fieldName.startsWith( "weightChampion" ) ) {
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
                    if( fieldName.startsWith( "name" ) ) {
                        cp[ npool ].name = records[r].vars[j].data[0].toInt();
                        cp[ npool ].nameIdx = j;
                    } else if( fieldName.startsWith( "weight" ) ) {
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
            /*
            if( records[r].vars[j].nameIndex == spawnMin &&
                    records[r].vars[j].data.size() > 0 ) {
                int v = records[r].vars[j].data.at(0).toInt();
                records[r].vars[j].data[0] = QVariant( v + 5 );
            } else if( records[r].vars[j].nameIndex == championMin &&
                   records[r].vars[j].data.size() > 0 ) {
                int v = records[r].vars[j].data.at(0).toInt();
                records[r].vars[j].data[0] = QVariant( v + 2 );
              */

        }
        // out of the record, have enough data to re-weight things?
        for( int i = 1; i < 16; ++i ) {
            if( np[i].name == -1 ) {
                continue;
            }
            records[r].vars[ np[i].weightIdx ].data[0] = QVariant( 100 );
            records[r].vars[ np[i].levelVarIdx ].data[0] = QVariant( lvchanger[ np[i].levelVar ] );

           // cout << string( np[i].name ) << "\t" << np[i].weight << string( np[i].levelVar ) <<  endl;
           // cout << "UPGRADED TO " << 100 << "\t" << string( lvchanger[ np[i].levelVar ] ) << endl;
        }
        for( int i = 1; i < 16; ++i ) {
            if( cp[i].name == -1 ) {
                continue;
            }
            cout << "DEBUG weight " << cp[i].weight << " " << cp[i].weightIdx << endl;
            if( cp[i].weight != -1 ) {
                records[r].vars[ cp[i].weightIdx ].data[0] = QVariant( 100 );
            }
            cout << "DEBUG minPC " << cp[i].minPClevel << " " << cp[i].minPClevelIdx << endl;
            if( cp[i].minPClevel != -1 ) {
                int lvl = records[r].vars[cp[i].minPClevelIdx ].data.at( 0 ).toInt();
                lvl += ( lvl + 1 ) / 10;
                records[r].vars[cp[i].minPClevelIdx ].data[ 0 ] = QVariant( lvl );

            }

        }
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
