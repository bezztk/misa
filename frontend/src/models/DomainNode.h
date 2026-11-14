#ifndef FRONTEND_DOMAINNODE_H
#define FRONTEND_DOMAINNODE_H

#include <QString>

#include <optional>

struct DomainNode {
    int id;
    int domainId;
    std::optional<int> parentId;
    QString name;
};

#endif // FRONTEND_DOMAINNODE_H
