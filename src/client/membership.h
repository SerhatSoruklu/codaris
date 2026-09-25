#ifndef CODARIS_MEMBERSHIP_H
#define CODARIS_MEMBERSHIP_H

/* C controls account operations; browser bridges transport requests and render
 * server responses. PostgreSQL owns durable account and learning state. */
#define MEMBER_TOPIC_COUNT 22
static unsigned explored_topics;

EM_JS(void, member_ready, (void), {
    document.querySelectorAll('[data-account-fields], [data-account-button], [data-preview-fields], [data-preview-button]').forEach(e => e.disabled = false);
    const fields = document.getElementById('join-fields');
    if (fields) {
        fields.disabled = false;
        fields.querySelector('button[type="submit"]').disabled = false;
    }
    document.body.dataset.memberReady = 'true';
    document.dispatchEvent(new Event('codaris-member-ready'));
})
EM_JS(void, member_tab_view, (int index, int focus), {
    const tabs = document.querySelectorAll('[role="tab"][data-member-tab]');
    tabs.forEach((tab, i) => {
        tab.setAttribute('aria-selected', String(i === index));
        tab.tabIndex = i === index ? 0 : -1;
        document.getElementById(tab.getAttribute('aria-controls')).hidden = i !== index;
        if (i === index && focus) tab.focus();
    });
})
EM_JS(void, member_topic_view, (int index, int selected), {
    const button = document.querySelector('[data-topic="' + index + '"]');
    if (button) {
        button.setAttribute('aria-pressed', String(!!selected));
        button.textContent = selected ? 'Read — undo' : 'Mark as read';
    }
})
EM_JS(void, member_result, (const char *key, const char *message, int is_error), {
    const name = UTF8ToString(key);
    const output = document.getElementById(name + '-feedback');
    if (output) {
        output.textContent = UTF8ToString(message);
        output.classList.add('account-notice');
        output.dataset.tone = is_error ? 'error' : 'success';
        output.setAttribute('role', is_error ? 'alert' : 'status');
    }
})
EMSCRIPTEN_KEEPALIVE void codaris_member_tab(int index, int focus) {
    if (index >= 0 && index < 5) member_tab_view(index, focus);
}
EMSCRIPTEN_KEEPALIVE void codaris_member_key(int index, const char *key) {
    if (index < 0 || index >= 5) return;
    if (!strcmp(key, "ArrowRight")) index = (index + 1) % 5;
    else if (!strcmp(key, "ArrowLeft")) index = (index + 4) % 5;
    else if (!strcmp(key, "Home")) index = 0;
    else if (!strcmp(key, "End")) index = 4;
    else return;
    codaris_member_tab(index, 1);
}
static int pending_topic=-1;
EM_JS(void, member_progress_request, (int topic,int read), {
    fetch('/api/progress',{method:'POST',credentials:'same-origin',headers:{'Content-Type':'application/json'},
        body:JSON.stringify({topic:String(topic),read:String(read)})})
      .then(response=>Module.ccall('codaris_progress_response',null,['number'],[response.status]))
      .catch(()=>Module.ccall('codaris_progress_response',null,['number'],[0]));
})
EMSCRIPTEN_KEEPALIVE void codaris_member_topic(int index) {
    if(index<0 || index>=MEMBER_TOPIC_COUNT || pending_topic>=0)return;
    pending_topic=index;
    member_progress_request(index,!(explored_topics & (1u<<index)));
}
EMSCRIPTEN_KEEPALIVE void codaris_progress_loaded(unsigned mask) {
    explored_topics=mask;
    unsigned count=0;
    for(int i=0;i<MEMBER_TOPIC_COUNT;i++) { int selected=(mask>>i)&1u;member_topic_view(i,selected);count+=(unsigned)selected; }
    char message[64];int n=snprintf(message,sizeof(message),"%u of %u topics read",count,(unsigned)MEMBER_TOPIC_COUNT);
    if(n>0 && (size_t)n<sizeof(message))view_text("learning-progress",message);
}
EMSCRIPTEN_KEEPALIVE void codaris_progress_response(int status) {
    if(pending_topic<0)return;
    if(status==200)codaris_progress_loaded(explored_topics^(1u<<pending_topic));
    else view_text("learning-progress", "Progress could not be saved. Sign in and try again.");
    pending_topic=-1;
}
/* DOM access and JSON serialization are browser transport; C chooses operations
 * and owns navigation / success handling. Passwords are never persisted. */
EM_JS(void, member_request, (const char *operation, const char *endpoint), {
    const kind = UTF8ToString(operation), path = UTF8ToString(endpoint);
    const form = document.querySelector('[data-account-form="' + kind + '"]');
    const payload = {};
    if (form) new FormData(form).forEach((value, key) => { payload[key] = value; });
    if (form) {
        const canvas=form.querySelector('#avatar-preview');
        if(canvas) {
            payload.avatar="";
            if(!canvas.hidden) {
                const context=canvas.getContext('2d');
                if(!context) { Module.ccall('codaris_account_response',null,['string','number','string','number'],[kind,0,'Unable to read the profile picture.',0]);return; }
                const pixels=context.getImageData(0,0,100,100).data;
                let raw=""; for(let i=0;i<pixels.length;i++)raw+=String.fromCharCode(pixels[i]);
                payload.avatar=btoa(raw);
            }
        }
    }
    if (kind === 'verify' || kind === 'reset') payload.token = window.codarisActionToken || "";
    const options = {credentials: 'same-origin', cache: 'no-store'};
    if (kind !== 'me') {
        options.method = 'POST'; options.headers = {'Content-Type': 'application/json'};
        options.body = JSON.stringify(payload);
    }
    if (form) form.querySelectorAll('button').forEach(e => e.disabled = true);
    fetch(path, options).then(async response => {
        const contentType = response.headers.get('content-type') || "";
        if (!contentType.toLowerCase().includes('application/json')) {
            throw new Error('The account service returned an unexpected response. Please try again later.');
        }
        const data = await response.json();
        if (kind === 'me' && response.ok) {
            Module.ccall('codaris_progress_loaded', null, ['number'], [data.progress || 0]);
            const raw=atob(data.avatar || "");
            document.querySelectorAll('#avatar-preview, #member-card-avatar').forEach(canvas=>{
                const context=canvas.getContext('2d'); if(!context)return;
                canvas.hidden=!raw;
                if(raw){const pixels=new Uint8ClampedArray(raw.length);for(let i=0;i<raw.length;i++)pixels[i]=raw.charCodeAt(i);context.putImageData(new ImageData(pixels,100,100),0,0);}
                else context.clearRect(0,0,100,100);
            });
            document.querySelectorAll('[data-avatar-fallback]').forEach(e=>e.hidden=!!raw);
            const mapping = {'profile-name':'name','profile-country':'country','profile-role':'role',
                'profile-linkedin':'linkedin','profile-github':'github','profile-website':'website'};
            Object.entries(mapping).forEach(([id, key]) => {
                const element = document.getElementById(id); if (element) element.value = data[key] || "";
            });
            const text = {'member-display-name':'name','membership-id':'membership_id',
                'account-email':'email','application-reason':'motivation'};
            Object.entries(text).forEach(([id,key]) => { const e=document.getElementById(id); if(e)e.textContent=data[key] || ""; });
        }
        Module.ccall('codaris_account_response', null, ['string','number','string','number'],
            [kind, response.status, data.message || "", data.email_verified ? 1 : 0]);
    }).catch(error => Module.ccall('codaris_account_response', null,
        ['string','number','string','number'], [kind,0,
            error instanceof TypeError ? 'Unable to reach the account service. Please try again.' :
            (error.message || 'The account service returned an unexpected response. Please try again later.'),0]))
      .finally(() => {
        if (form) { form.querySelectorAll('button').forEach(e => e.disabled = false);
            form.querySelectorAll('input[type="password"]').forEach(e => e.value = ""); }
      });
})
EM_JS(void, member_navigate, (const char *path), { location.assign(UTF8ToString(path)); })
EM_JS(void, member_authenticated, (int verified), {
    const panel = document.getElementById('account-content'); if(panel) panel.hidden=false;
    const resend = document.getElementById('resend-verification'); if(resend)resend.hidden=!!verified;
})
EM_JS(int, member_form_matches, (const char *kind), {
    const form=document.querySelector('[data-account-form="'+UTF8ToString(kind)+'"]');
    if(!form)return 1;
    const password=form.elements.namedItem('password'), confirm=form.elements.namedItem('confirmation');
    return !confirm || (password && password.value===confirm.value);
})
EMSCRIPTEN_KEEPALIVE void codaris_account_submit(const char *kind) {
    static const char *operations[]={"register","login","profile","password","email","recover","verify","reset","resend","logout","me"};
    if(!member_form_matches(kind)){member_result(kind,"Passwords do not match.",1);return;}
    for(size_t i=0;i<sizeof(operations)/sizeof(operations[0]);i++) if(!strcmp(kind,operations[i])) {
        char path[40];int n=snprintf(path,sizeof(path),"/api/%s",kind);
        if(n>0 && (size_t)n<sizeof(path))member_request(kind,path);
        return;
    }
}
EM_JS(void, member_recovery_invalid, (const char *message), {
    const input = document.getElementById('recovery-email');
    const error = document.getElementById('recovery-email-error');
    if (input) { input.setAttribute('aria-invalid', 'true'); input.focus(); }
    if (error) error.textContent = UTF8ToString(message);
})
EM_JS(int, member_verification_link, (void), {
    return !!document.getElementById('verification-title') && !!window.codarisActionToken;
})
EM_JS(void, member_verification_view, (int status), {
    const title = document.getElementById('verification-title');
    if (!title) return;
    title.textContent = status === 200 ? 'Email verified' : status === -1 ? 'Verifying your email…' : 'Unable to verify email';
    document.getElementById('verification-description').textContent = status === 200
      ? 'Your membership is active. Sign in to start building and learning.'
      : status === -1 ? 'Please wait while we confirm your verification link.'
      : 'The link may have expired or already been used. Sign in to check your verification status or request a new link.';
    document.getElementById('verification-retry').hidden = status !== 0 && status < 500;
})
EMSCRIPTEN_KEEPALIVE void codaris_account_response(const char *kind,int status,const char *message,int verified) {
    if(!strcmp(kind,"me")) {
        if(status==401){member_navigate("/login/");return;}
        if(status==200){member_authenticated(verified);view_text("email-verification",verified?"Email verified · membership active":"Email not verified · check your inbox");}
        else member_result("account",message,1);
        return;
    }
    member_result(kind,message,status < 200 || status >= 300);
    if (!strcmp(kind,"verify")) member_verification_view(status);
    if (!strcmp(kind,"recover") && status == 400) member_recovery_invalid(message);
    if(status<200 || status>=300)return;
    if(!strcmp(kind,"login"))member_navigate("/dashboard/");
    else if(!strcmp(kind,"logout") || !strcmp(kind,"password"))member_navigate("/login/");
    else if(!strcmp(kind,"profile") || !strcmp(kind,"email"))codaris_account_submit("me");
}
/* Browser file metadata limits decoding; server storage accepts only an exact
 * 100x100 RGBA pixel buffer, never a file format or metadata. */
EMSCRIPTEN_KEEPALIVE int codaris_member_photo(const char *mime, double size, int width, int height) {
    if (strcmp(mime, "image/jpeg") && strcmp(mime, "image/png") && strcmp(mime, "image/webp")) {
        view_text("avatar-status", "Choose a JPEG, PNG or WebP image."); return 0;
    }
    if (size <= 0 || size > 5 * 1024 * 1024) {
        view_text("avatar-status", "Choose a non-empty image no larger than 5 MiB."); return 0;
    }
    if (width < 0 || height < 0 || width > 8192 || height > 8192) {
        view_text("avatar-status", "The image could not be read or exceeds 8192 pixels per side."); return 0;
    }
    if (width && height) view_text("avatar-status", "Picture selected. Save your profile to keep it.");
    return 1;
}
EMSCRIPTEN_KEEPALIVE void codaris_member_photo_removed(void) {
    view_text("avatar-status", "Picture removed. Save your profile to keep this change.");
}
EM_JS(int, member_is_account_page, (void), { return !!document.querySelector('[data-account-form], #account-content'); })
EM_JS(int, member_is_dashboard, (void), { return !!document.getElementById('account-content'); })
static void membership_init(void) {
    if(!member_is_account_page())return;
    member_ready();
    if(member_is_dashboard())codaris_account_submit("me");
    if(member_verification_link()) { member_verification_view(-1); codaris_account_submit("verify"); }
}
#endif
