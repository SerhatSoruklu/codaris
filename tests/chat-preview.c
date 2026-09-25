#include "../src/shared/chat_preview.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 ChatState s;chat_reset(&s);
 assert(sizeof(chat_countries)/sizeof(chat_countries[0])==50);
 for(int i=0;i<50;++i){assert(chat_countries[i] && *chat_countries[i]);for(int j=0;j<i;++j)assert(strcmp(chat_countries[i],chat_countries[j]));}
 assert(chat_send(&s,"Alex","Hello","",0));assert(s.count==0);
 s.room=0;
 assert(chat_send(&s,"Alex"," \n\t","",0));
 assert(chat_send(&s,"Alex","f.u.c.k","",0));
 assert(chat_send(&s,"Alex","sh1t","",0));
 assert(!chat_offensive("class assignment and assembly"));
 assert(chat_send(&s,"Alex","AAAAAAAAAAAAAAAAAAAA","",0));
 assert(chat_send(&s,"Alex","THIS IS A VERY LOUD MESSAGE","",0));
 assert(!chat_send(&s,"Alex","Hello everyone 👋","",10000));
 assert(chat_send(&s,"Alex","Something else","",11000));
 s.room=1;assert(chat_send(&s,"Alex","Something else","",11500));
 assert(chat_send(&s,"Alex","Hello everyone 👋","",13000));
 assert(!chat_send(&s,"Alex","A distinct question","",13000));
 assert(s.messages[0].room==0 && s.messages[1].room==1);
 assert(!chat_send(&s,"Alex","Message three","",16000));
 assert(!chat_send(&s,"Alex","Message four","",19000));
 assert(!chat_send(&s,"Alex","Message five","",22000));
 assert(chat_send(&s,"Alex","Message six","",25000));
 assert(!chat_send(&s,"Alex","Message six","",41000));
 assert(!chat_attachment("diagram.PNG","image/png",100));
 assert(!chat_attachment("notes.txt","text/plain",100));
 assert(!chat_attachment("guide.pdf","application/pdf",100));
 assert(chat_attachment("app.exe","application/octet-stream",100));
 assert(chat_attachment("image.png","image/jpeg",100));
 assert(chat_attachment("../secret.txt","text/plain",100));
 assert(chat_attachment("large.png","image/png",5*1024*1024+1));
 assert(chat_attachment("empty.txt","text/plain",0));
 assert(chat_attachment("bad\nname.txt","text/plain",1));
 char oversized[CHAT_TEXT+1];memset(oversized,'a',sizeof(oversized)-1);oversized[sizeof(oversized)-1]=0;
 assert(chat_send(&s,"Alex",oversized,"",50000));
 chat_reset(&s);s.room=0;
 assert(!chat_send(&s,"Alex","","diagram.png",0));
 assert(chat_send(&s,"Alex","","diagram.png",3000));
 s.count=CHAT_LIMIT;assert(chat_send(&s,"Alex","Limit","",90000));
 puts("PASS: rooms, limits, duplicate/spam/offensive checks, attachment policy and capacity");
 return 0;
}
