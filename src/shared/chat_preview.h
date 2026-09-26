#ifndef CODARIS_CHAT_PREVIEW_H
#define CODARIS_CHAT_PREVIEW_H
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define CHAT_LIMIT 100
#define CHAT_TEXT 2001
static const char *chat_countries[50] = {
 "Argentina", "Australia", "Austria", "Bangladesh", "Belgium", "Brazil", "Canada", "Chile", "China", "Colombia",
 "Czechia", "Denmark", "Egypt", "Finland", "France", "Germany", "Ghana", "Greece", "India", "Indonesia",
 "Ireland", "Israel", "Italy", "Japan", "Kenya", "Malaysia", "Mexico", "Morocco", "Netherlands", "New Zealand",
 "Nigeria", "Norway", "Pakistan", "Philippines", "Poland", "Portugal", "Romania", "Saudi Arabia", "Singapore",
 "Somalia", "South Africa", "South Korea", "Spain", "Sweden", "Switzerland", "Türkiye", "Ukraine", "United Arab Emirates", "United Kingdom", "United States"
};
typedef struct { int room; char name[481], text[CHAT_TEXT], file[121]; } ChatMessage;
typedef struct {
 int room, count, blocked[3], reported[3], report_target;
 double recent[5]; int sent;
 ChatMessage messages[CHAT_LIMIT];
} ChatState;
static void chat_reset(ChatState *s) { memset(s, 0, sizeof(*s)); s->room=-1; }
static void chat_normalize(const char *input, char *out, size_t capacity) {
 size_t n=0;
 for (; *input && n+1<capacity; ++input) {
  unsigned char c=(unsigned char)*input;
  if (isspace(c)) { if(n && out[n-1]!=' ')out[n++]=' '; }
  else if(isalnum(c) || c>=128) {
   c=(unsigned char)tolower(c);
   if(c=='0')c='o'; else if(c=='1')c='i'; else if(c=='3')c='e'; else if(c=='4')c='a'; else if(c=='5')c='s'; else if(c=='7')c='t';
   out[n++]=(char)c;
  }
 }
 if(n && out[n-1]==' ')--n;
 out[n]='\0';
}
static int chat_offensive(const char *text) {
 char folded[CHAT_TEXT]; chat_normalize(text,folded,sizeof(folded));
 const char *words[]={"fuck","fucking","shit","bitch","asshole","bastard","kill yourself"};
 for(size_t i=0;i<sizeof(words)/sizeof(words[0]);++i) {
  const char *p=folded;
  while((p=strstr(p,words[i]))) {
   size_t len=strlen(words[i]);
   if((p==folded || p[-1]==' ') && (p[len]=='\0' || p[len]==' '))return 1;
   ++p;
  }
 }
 return 0;
}
static const char *chat_attachment(const char *name,const char *mime,double bytes) {
 if(!name || !mime || !*name || strlen(name)>120)return "Use an attachment name of 1–120 UTF-8 bytes.";
 if(bytes<=0 || bytes>5*1024*1024)return "Choose a non-empty file up to 5 MiB.";
 for(const unsigned char *p=(const unsigned char *)name;*p;++p)if(*p<32 || *p==127 || *p=='/' || *p=='\\')return "This attachment name is not supported.";
 if(chat_offensive(name))return "Please rename the attachment using respectful language.";
 const char *dot=strrchr(name,'.'); if(!dot)return "Use a PNG, JPEG, WebP, PDF or TXT file.";
 char ext[8];size_t n=strlen(dot);if(n>=sizeof(ext))return "This attachment type is not supported.";
 for(size_t i=0;i<=n;++i)ext[i]=(char)tolower((unsigned char)dot[i]);
 if((!strcmp(mime,"image/png")&&!strcmp(ext,".png")) || (!strcmp(mime,"image/jpeg")&&(!strcmp(ext,".jpg")||!strcmp(ext,".jpeg"))) || (!strcmp(mime,"image/webp")&&!strcmp(ext,".webp")) || (!strcmp(mime,"application/pdf")&&!strcmp(ext,".pdf")) || (!strcmp(mime,"text/plain")&&!strcmp(ext,".txt")))return NULL;
 return "File type and extension must match PNG, JPEG, WebP, PDF or TXT.";
}
static const char *chat_send(ChatState *s,const char *name,const char *text,const char *file,double now) {
 if(s->room<0 || s->room>=50)return "Choose a country room before sending.";
 if(s->count>=CHAT_LIMIT)return "This preview holds 100 messages. Reload to start a new preview.";
 if(!name || !*name || strlen(name)>480)return "Set a display name in Profile before chatting.";
 if(!text || strlen(text)>=CHAT_TEXT)return "Keep the message within 2,000 UTF-8 bytes.";
 if(!file || strlen(file)>120)return "The attachment name is too long.";
 int content=0,letters=0,caps=0,repeat=0;unsigned char previous=0;
 for(const unsigned char *p=(const unsigned char *)text;*p;++p){
  if(!isspace(*p))content=1;
  if((*p<32 && *p!='\n' && *p!='\t') || *p==127)return "Remove control characters from the message.";
  if(isalpha(*p)){++letters;if(isupper(*p))++caps;}
  repeat=(*p==previous)?repeat+1:1; previous=*p;
  if(repeat>12)return "Please avoid repeated-character spam.";
 }
 if(!content && !*file)return "Write a message or choose an attachment.";
 if(chat_offensive(text)||chat_offensive(name)||chat_offensive(file))return "Please keep messages, display names and attachments respectful.";
 if(letters>=15 && caps*100/letters>85)return "Please avoid shouting in all capitals.";
 if(s->sent && now-s->recent[(s->sent-1)%5]<2000)return "Please wait 2 seconds between messages.";
 if(s->sent>=5 && now-s->recent[s->sent%5]<30000)return "Slow down: up to 5 messages every 30 seconds.";
 char folded[CHAT_TEXT];chat_normalize(text,folded,sizeof(folded));
 for(int i=s->count-1;i>=0 && i>=s->count-5;--i){
  char old[CHAT_TEXT];chat_normalize(s->messages[i].text,old,sizeof(old));
  if((content && !strcmp(old,folded)) || (!content && !strcmp(s->messages[i].file,file)))return "That repeats a recent message. Add something new.";
 }
 ChatMessage *m=&s->messages[s->count++];m->room=s->room;
 snprintf(m->name,sizeof(m->name),"%s",name);snprintf(m->text,sizeof(m->text),"%s",text);snprintf(m->file,sizeof(m->file),"%s",file);
 s->recent[s->sent%5]=now;++s->sent;return NULL;
}
#endif
