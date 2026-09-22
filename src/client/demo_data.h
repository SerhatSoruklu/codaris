#ifndef CODARIS_DEMO_DATA_H
#define CODARIS_DEMO_DATA_H

#include <stddef.h>

/* Illustrative planning fixtures, NOT sourced statistics or actual membership.
 * Future API responses should populate the same view model after validation. */
typedef struct {
    const char *name;
    const char *code;
    unsigned developers;
    unsigned supporters;
    const char *status;
} Country;

static const Country countries[] = {
    {"United States", "US", 4400000, 2840, "Active"},
    {"India", "IN", 5800000, 3120, "Active"},
    {"China", "CN", 4200000, 420, "Emerging"},
    {"United Kingdom", "GB", 900000, 960, "Active"},
    {"Germany", "DE", 1000000, 740, "Active"},
    {"France", "FR", 650000, 430, "Active"},
    {"Japan", "JP", 1200000, 210, "Emerging"},
    {"Brazil", "BR", 800000, 620, "Active"},
    {"Canada", "CA", 550000, 510, "Active"},
    {"Australia", "AU", 350000, 280, "Active"},
    {"Turkey", "TR", 300000, 340, "Active"},
    {"Nigeria", "NG", 150000, 180, "Emerging"},
    {"South Africa", "ZA", 120000, 120, "Emerging"},
    {"Pakistan", "PK", 350000, 240, "Emerging"},
    {"Indonesia", "ID", 600000, 260, "Emerging"},
    {"Mexico", "MX", 400000, 190, "Emerging"},
    {"Netherlands", "NL", 320000, 220, "Active"},
    {"Poland", "PL", 450000, 310, "Active"},
    {"Ukraine", "UA", 300000, 230, "Active"},
    {"Singapore", "SG", 150000, 100, "Emerging"},
    {"United Arab Emirates (UAE)", "AE", 100000, 70, "Emerging"},
    {"Saudi Arabia", "SA", 120000, 0, "Target"},
    {"Somalia", "SO", 5000, 0, "Target"}
};
static const size_t country_count = sizeof(countries) / sizeof(countries[0]);
static const unsigned demo_communities = 38;
#endif
