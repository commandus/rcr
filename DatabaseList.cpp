//
// Created by andrei on 19.04.24.
//

#include "DatabaseList.h"
#include <google/protobuf/util/json_util.h>
#include "utilstring.h"

static google::protobuf::util::JsonParseOptions jsonParseOptions;
static google::protobuf::util::JsonPrintOptions jsonPrintOptions;

DatabaseList::DatabaseList(
    const std::string &directoryName
)
{
    rcr::EndPointResponse list;
    fileName = directoryName + "/endpoints.json";
    std::string s = file2string(fileName);
    google::protobuf::util::JsonStringToMessage(s, &list, jsonParseOptions);
}

bool DatabaseList::save()
{
    std::string s;
    auto r = google::protobuf::util::MessageToJsonString(list, &s, jsonPrintOptions);
    return r.ok();
}
