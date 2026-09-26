#ifndef CODARIS_CREDENTIAL_RENDER_H
#define CODARIS_CREDENTIAL_RENDER_H

#include <stddef.h>

typedef struct {
    const char *display_name;
    const char *role;
    const char *membership_number;
    const char *verification_id;
    const char *status;
    const char *issued_at;
    const char *verification_url;
    const unsigned char *avatar_rgba;
    size_t avatar_size;
} CodarisCredential;

int codaris_credential_svg(const CodarisCredential *credential, int back, char **output,
                           size_t *output_size);
int codaris_credential_pdf(const CodarisCredential *credential, const char *font_path,
                           unsigned char **output, size_t *output_size);

#endif
