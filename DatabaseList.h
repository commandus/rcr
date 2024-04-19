#ifndef RCR_DATABASELIST_H
#define RCR_DATABASELIST_H

#include "gen/rcr.pb-odb.hxx"
#include "gen/odb-views-odb.hxx"

class DatabaseList {
private:
    std::string fileName;
public:
    rcr::EndPointResponse list;
    explicit DatabaseList(
        const std::string &directoryName
    );
    bool save();
};


#endif
