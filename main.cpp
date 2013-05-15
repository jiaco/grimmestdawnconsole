#include <QCoreApplication>

#include "arz.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    ARZ arz;

    arz.doit();

    return a.exec();
}
