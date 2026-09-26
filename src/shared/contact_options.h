#ifndef CODARIS_CONTACT_OPTIONS_H
#define CODARIS_CONTACT_OPTIONS_H

#include <stddef.h>
#include <string.h>

static const char *const codaris_contact_topics[] = {
    "Membership/application",
    "Account sign-in or verification",
    "Password/email changes",
    "Learning library/content",
    "Research or technical correction",
    "Security/privacy concern",
    "Accessibility",
    "Website bug",
    "Partnership/collaboration",
    "Press/media",
    "Community",
    "Report impersonation/abuse",
    "Data access/deletion",
    "General question",
    "Other",
};

static inline int codaris_contact_topic_allowed(const char *topic) {
    for (size_t i = 0; i < sizeof(codaris_contact_topics) / sizeof(codaris_contact_topics[0]); ++i)
        if (!strcmp(topic, codaris_contact_topics[i]))
            return 1;
    return 0;
}

#endif
