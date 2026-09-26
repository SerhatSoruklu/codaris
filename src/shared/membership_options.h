#ifndef CODARIS_MEMBERSHIP_OPTIONS_H
#define CODARIS_MEMBERSHIP_OPTIONS_H

#include <stddef.h>
#include <string.h>

/* Country names are the persisted API values. ISO codes are used only to
 * select the locally bundled flag artwork. Keep existing values unchanged. */
typedef struct {
    const char *name;
    const char *code;
} MembershipCountry;

static const MembershipCountry membership_countries[] = {
    {"United States", "us"},
    {"India", "in"},
    {"China", "cn"},
    {"United Kingdom", "gb"},
    {"Germany", "de"},
    {"France", "fr"},
    {"Japan", "jp"},
    {"Brazil", "br"},
    {"Canada", "ca"},
    {"Australia", "au"},
    {"Turkey", "tr"},
    {"Nigeria", "ng"},
    {"South Africa", "za"},
    {"Pakistan", "pk"},
    {"Indonesia", "id"},
    {"Mexico", "mx"},
    {"Netherlands", "nl"},
    {"Poland", "pl"},
    {"Ukraine", "ua"},
    {"Singapore", "sg"},
    {"United Arab Emirates (UAE)", "ae"},
    {"Saudi Arabia", "sa"},
    {"Somalia", "so"},
    {"Afghanistan", "af"},
    {"Albania", "al"},
    {"Algeria", "dz"},
    {"Argentina", "ar"},
    {"Austria", "at"},
    {"Bangladesh", "bd"},
    {"Belgium", "be"},
    {"Bolivia", "bo"},
    {"Bulgaria", "bg"},
    {"Cambodia", "kh"},
    {"Chile", "cl"},
    {"Colombia", "co"},
    {"Croatia", "hr"},
    {"Czechia", "cz"},
    {"Denmark", "dk"},
    {"Ecuador", "ec"},
    {"Egypt", "eg"},
    {"Ethiopia", "et"},
    {"Finland", "fi"},
    {"Ghana", "gh"},
    {"Greece", "gr"},
    {"Hungary", "hu"},
    {"Iceland", "is"},
    {"Iran", "ir"},
    {"Iraq", "iq"},
    {"Ireland", "ie"},
    {"Israel", "il"},
    {"Italy", "it"},
    {"Jamaica", "jm"},
    {"Jordan", "jo"},
    {"Kenya", "ke"},
    {"Malaysia", "my"},
    {"Morocco", "ma"},
    {"Nepal", "np"},
    {"New Zealand", "nz"},
    {"Norway", "no"},
    {"Peru", "pe"},
    {"Philippines", "ph"},
    {"Portugal", "pt"},
    {"Qatar", "qa"},
    {"Romania", "ro"},
    {"Serbia", "rs"},
    {"South Korea", "kr"},
    {"Spain", "es"},
    {"Sri Lanka", "lk"},
    {"Sweden", "se"},
    {"Switzerland", "ch"},
    {"Taiwan", "tw"},
    {"Thailand", "th"},
    {"Vietnam", "vn"},
};
static const size_t membership_country_count =
    sizeof(membership_countries) / sizeof(membership_countries[0]);

typedef struct {
    const char *value;
    const char *label;
} MembershipRole;

/* “Community Organiser” deliberately keeps its original stored/API value. */
static const MembershipRole membership_roles[] = {
    {"Developer / Engineer", "Developer / Engineer"},
    {"AI / ML Engineer", "AI / ML Engineer"},
    {"Security Engineer / Researcher", "Security Engineer / Researcher"},
    {"Researcher", "Researcher"},
    {"Systems / Infrastructure Engineer", "Systems / Infrastructure Engineer"},
    {"Technical Founder / Entrepreneur", "Technical Founder / Entrepreneur"},
    {"Open-source Maintainer / Contributor", "Open-source Maintainer / Contributor"},
    {"Product / UX Designer", "Product / UX Designer"},
    {"Educator / Technical Writer", "Educator / Technical Writer"},
    {"Community organiser", "Community Organiser"},
    {"Student", "Student"},
    {"Other", "Other"},
};
static const size_t membership_role_count = sizeof(membership_roles) / sizeof(membership_roles[0]);

static inline int membership_country_allowed(const char *name) {
    if (!name) return 0;
    if (!strcmp(name, "Other / not listed") || !strcmp(name, "Türkiye")) return 1;
    for (size_t i = 0; i < membership_country_count; ++i)
        if (!strcmp(name, membership_countries[i].name)) return 1;
    return 0;
}

static inline int membership_role_allowed(const char *value) {
    if (!value) return 0;
    for (size_t i = 0; i < membership_role_count; ++i)
        if (!strcmp(value, membership_roles[i].value)) return 1;
    return 0;
}

#endif
