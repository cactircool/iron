#ifndef EA7A3CEB_5D62_45C6_B26D_9C436221FE54
#define EA7A3CEB_5D62_45C6_B26D_9C436221FE54

#include "Type.hpp"

namespace iron {

    struct Label : public Type {
        Label(Node *node) : Type(node) {}
    };

}

#endif /* EA7A3CEB_5D62_45C6_B26D_9C436221FE54 */
